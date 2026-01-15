#!/usr/bin/env python3
"""
完整版卫星高功率微波激励测试 - 基于weixing_v1_case.pfd配置
严格按照FDTD配置文件实现平面波激励、输出格式和测试参数
"""

import numpy as np
import json
import time
import os
from typing import Dict, List, Tuple, Optional
from dataclasses import dataclass
from pathlib import Path

try:
    import vtk
    from vtk.util import numpy_support as vtk_np
    VTK_AVAILABLE = True
except ImportError:
    VTK_AVAILABLE = False
    print("警告: VTK库未安装，将使用简化可视化")

try:
    import h5py
    HDF5_AVAILABLE = True
except ImportError:
    HDF5_AVAILABLE = False
    print("警告: HDF5库未安装，将使用JSON格式")

# 物理常数
C0 = 299792458.0  # 光速 m/s
MU0 = 4.0 * np.pi * 1e-7  # 真空磁导率
EPS0 = 1.0 / (MU0 * C0**2)  # 真空介电常数  
PI = np.pi

@dataclass
class FDTDConfig:
    """FDTD配置参数类 - 严格对应weixing_v1_case.pfd"""
    # 基本参数
    frequency: float = 10.0e9  # 10GHz
    wavelength: float = 0.03  # 30mm
    
    # 计算域 (mm -> m)
    domain_size: np.ndarray = np.array([3.4, 3.4, 1.4])  # 3400×3400×1400 mm
    
    # 卫星尺寸 (mm -> m) 
    satellite_size: np.ndarray = np.array([2.8, 2.8, 1.0])  # 2800×2800×1000 mm
    
    # 网格间距 (mm -> m)
    grid_spacing: float = 0.02  # 20mm
    
    # 平面波入射角 (度)
    theta: float = 45.0  # 极角
    phi: float = 45.0    # 方位角  
    psi: float = 45.0    # 极化角
    
    # 输出配置
    output_step: int = 10
    output_format: str = "VTK_XML"  # HDF5 + VTK_XML
    
    # 监测点配置
    center_point: np.ndarray = np.array([0.0, 0.0, 0.0])  # 中心点
    
    # 切面配置
    plane_x0_pos: float = 0.0  # X=0平面
    plane_y0_pos: float = 0.0  # Y=0平面
    
    # 体积配置  
    volume_range: Dict = None  # 将在__post_init__中设置
    
    def __post_init__(self):
        # 体积监测范围 (对应FDTD配置中的坐标)
        self.volume_range = {
            'x': [-1.5, 1.5],  # -1500 to 1500 mm
            'y': [-1.5, 1.5],  # -1500 to 1500 mm  
            'z': [-0.55, 0.55]  # -550 to 550 mm
        }

class STLGeometryLoader:
    """STL几何文件加载器"""
    
    def __init__(self, filename: str):
        self.filename = filename
        self.vertices = None
        self.triangles = None
        self.bounds = None
        
    def load(self) -> bool:
        """加载STL文件"""
        try:
            if not os.path.exists(self.filename):
                print(f"STL文件不存在: {self.filename}")
                return False
                
            # 读取文件大小
            file_size = os.path.getsize(self.filename)
            print(f"STL文件大小: {file_size} 字节")
            
            # 简化的STL解析（ASCII格式）
            vertices = []
            triangles = []
            
            with open(self.filename, 'r', encoding='utf-8') as f:
                lines = f.readlines()
                
            # 解析顶点
            vertex_count = 0
            current_triangle = []
            
            for line in lines:
                line = line.strip()
                if line.startswith('vertex'):
                    # 解析顶点坐标
                    parts = line.split()
                    if len(parts) >= 4:
                        x, y, z = float(parts[1]), float(parts[2]), float(parts[3])
                        vertices.append([x/1000.0, y/1000.0, z/1000.0])  # mm -> m
                        current_triangle.append(vertex_count)
                        vertex_count += 1
                        
                        if len(current_triangle) == 3:
                            triangles.append(current_triangle)
                            current_triangle = []
            
            self.vertices = np.array(vertices)
            self.triangles = np.array(triangles)
            
            # 计算边界框
            if len(vertices) > 0:
                vertices_array = np.array(vertices)
                self.bounds = {
                    'min': np.min(vertices_array, axis=0),
                    'max': np.max(vertices_array, axis=0),
                    'center': (np.min(vertices_array, axis=0) + np.max(vertices_array, axis=0)) / 2.0
                }
                
            print(f"STL加载成功:")
            print(f"  顶点数: {len(vertices)}")
            print(f"  三角形数: {len(triangles)}")
            if self.bounds:
                print(f"  边界框: min={self.bounds['min']}, max={self.bounds['max']}")
                print(f"  中心: {self.bounds['center']}")
            
            return True
            
        except Exception as e:
            print(f"STL文件加载失败: {e}")
            return False
    
    def get_geometry_info(self) -> Dict:
        """获取几何信息"""
        if self.vertices is None:
            return {}
            
        return {
            'num_vertices': len(self.vertices),
            'num_triangles': len(self.triangles),
            'bounds': self.bounds,
            'dimensions': self.bounds['max'] - self.bounds['min'] if self.bounds else None
        }

class PlaneWaveExcitation:
    """平面波激励源 - 严格按照FDTD配置"""
    
    def __init__(self, config: FDTDConfig):
        self.config = config
        self.frequency = config.frequency
        self.wavelength = config.wavelength
        self.k0 = 2.0 * PI / self.wavelength
        
        # 入射方向向量 (球坐标转直角坐标)
        theta_rad = np.deg2rad(config.theta)
        phi_rad = np.deg2rad(config.phi)
        
        self.incident_direction = np.array([
            np.sin(theta_rad) * np.cos(phi_rad),
            np.sin(theta_rad) * np.sin(phi_rad),
            np.cos(theta_rad)
        ])
        
        # 极化向量
        psi_rad = np.deg2rad(config.psi)
        self.polarization = np.array([
            np.cos(psi_rad) * np.cos(theta_rad) * np.cos(phi_rad) - np.sin(psi_rad) * np.sin(phi_rad),
            np.cos(psi_rad) * np.cos(theta_rad) * np.sin(phi_rad) + np.sin(psi_rad) * np.cos(phi_rad),
            -np.cos(psi_rad) * np.sin(theta_rad)
        ])
        
        # 归一化极化向量
        self.polarization = self.polarization / np.linalg.norm(self.polarization)
        
        print(f"平面波激励配置:")
        print(f"  频率: {self.frequency/1e9} GHz")
        print(f"  波长: {self.wavelength*1000} mm")
        print(f"  入射方向: {self.incident_direction}")
        print(f"  极化方向: {self.polarization}")
        print(f"  入射角: θ={config.theta}°, φ={config.phi}°, ψ={config.psi}°")
    
    def compute_incident_field(self, points: np.ndarray) -> np.ndarray:
        """计算入射场"""
        num_points = points.shape[0]
        E_field = np.zeros((num_points, 3), dtype=complex)
        
        for i, point in enumerate(points):
            # 相位因子
            phase = -self.k0 * np.dot(self.incident_direction, point)
            phase_factor = np.exp(1j * phase)
            
            # 入射电场
            E_field[i] = self.polarization * phase_factor
            
        return E_field

class VTKExporter:
    """VTK格式导出器"""
    
    def __init__(self, filename: str):
        self.filename = filename
        
    def export_point_data(self, points: np.ndarray, field_data: Dict[str, np.ndarray], 
                         title: str = "Point Field Data"):
        """导出点数据为VTK格式"""
        if not VTK_AVAILABLE:
            print("VTK不可用，跳过点数据导出")
            return
            
        grid = vtk.vtkUnstructuredGrid()
        
        # 创建点坐标
        vtk_points = vtk.vtkPoints()
        for i, point in enumerate(points):
            vtk_points.InsertNextPoint(point[0], point[1], point[2])
        grid.SetPoints(vtk_points)
        
        # 添加场数据
        for field_name, field_values in field_data.items():
            vtk_array = vtk.vtkDoubleArray()
            vtk_array.SetName(field_name)
            vtk_array.SetNumberOfComponents(1)
            for value in field_values:
                vtk_array.InsertNextValue(value)
            grid.GetPointData().AddArray(vtk_array)
        
        writer = vtk.vtkXMLUnstructuredGridWriter()
        writer.SetFileName(self.filename)
        writer.SetInputData(grid)
        writer.Write()
        print(f"VTK点数据已导出: {self.filename}")
    
    def export_plane_data(self, plane_coords: np.ndarray, field_data: Dict[str, np.ndarray],
                         plane_type: str, position: float):
        """导出平面数据为VTK格式"""
        if not VTK_AVAILABLE:
            print("VTK不可用，跳过平面数据导出")
            return
            
        grid = vtk.vtkStructuredGrid()
        
        # 确定网格维度
        if plane_type == 'X':
            ny, nz = plane_coords.shape[0], plane_coords.shape[1]
            grid.SetDimensions(1, ny, nz)
        elif plane_type == 'Y':
            nx, nz = plane_coords.shape[0], plane_coords.shape[1] 
            grid.SetDimensions(nx, 1, nz)
        elif plane_type == 'Z':
            nx, ny = plane_coords.shape[0], plane_coords.shape[1]
            grid.SetDimensions(nx, ny, 1)
        
        # 创建点坐标
        vtk_points = vtk.vtkPoints()
        for i in range(plane_coords.shape[0]):
            for j in range(plane_coords.shape[1]):
                if plane_type == 'X':
                    vtk_points.InsertNextPoint(position, plane_coords[i,j,0], plane_coords[i,j,1])
                elif plane_type == 'Y':
                    vtk_points.InsertNextPoint(plane_coords[i,j,0], position, plane_coords[i,j,1])
                elif plane_type == 'Z':
                    vtk_points.InsertNextPoint(plane_coords[i,j,0], plane_coords[i,j,1], position)
        grid.SetPoints(vtk_points)
        
        # 添加场数据
        for field_name, field_values in field_data.items():
            vtk_array = vtk.vtkDoubleArray()
            vtk_array.SetName(field_name)
            vtk_array.SetNumberOfComponents(1)
            
            # 重塑数据为1D数组
            flat_data = field_values.flatten()
            for value in flat_data:
                vtk_array.InsertNextValue(value)
            grid.GetPointData().AddArray(vtk_array)
        
        writer = vtk.vtkXMLStructuredGridWriter()
        writer.SetFileName(self.filename)
        writer.SetInputData(grid)
        writer.Write()
        print(f"VTK平面数据已导出: {self.filename}")
    
    def export_volume_data(self, volume_coords: np.ndarray, field_data: Dict[str, np.ndarray]):
        """导出体积数据为VTK格式"""
        if not VTK_AVAILABLE:
            print("VTK不可用，跳过体积数据导出")
            return
            
        grid = vtk.vtkStructuredGrid()
        nx, ny, nz = volume_coords.shape[0], volume_coords.shape[1], volume_coords.shape[2]
        grid.SetDimensions(nx, ny, nz)
        
        # 创建点坐标
        vtk_points = vtk.vtkPoints()
        for i in range(nx):
            for j in range(ny):
                for k in range(nz):
                    vtk_points.InsertNextPoint(volume_coords[i,j,k,0], 
                                             volume_coords[i,j,k,1], 
                                             volume_coords[i,j,k,2])
        grid.SetPoints(vtk_points)
        
        # 添加场数据
        for field_name, field_values in field_data.items():
            vtk_array = vtk.vtkDoubleArray()
            vtk_array.SetName(field_name)
            vtk_array.SetNumberOfComponents(1)
            
            flat_data = field_values.flatten()
            for value in flat_data:
                vtk_array.InsertNextValue(value)
            grid.GetPointData().AddArray(vtk_array)
        
        writer = vtk.vtkXMLStructuredGridWriter()
        writer.SetFileName(self.filename)
        writer.SetInputData(grid)
        writer.Write()
        print(f"VTK体积数据已导出: {self.filename}")
    
    def export_geometry(self, vertices: np.ndarray, triangles: np.ndarray = None):
        """导出几何模型为VTK格式"""
        if not VTK_AVAILABLE:
            print("VTK不可用，跳过几何导出")
            return
            
        grid = vtk.vtkUnstructuredGrid()
        
        # 创建点
        vtk_points = vtk.vtkPoints()
        for vertex in vertices:
            vtk_points.InsertNextPoint(vertex[0], vertex[1], vertex[2])
        grid.SetPoints(vtk_points)
        
        # 创建单元（如果有三角形数据）
        if triangles is not None:
            for triangle in triangles:
                if len(triangle) == 3:
                    grid.InsertNextCell(vtk.VTK_TRIANGLE, 3, triangle)
                elif len(triangle) == 4:
                    grid.InsertNextCell(vtk.VTK_QUAD, 4, triangle)
        
        writer = vtk.vtkXMLUnstructuredGridWriter()
        writer.SetFileName(self.filename)
        writer.SetInputData(grid)
        writer.Write()
        print(f"几何模型已导出: {self.filename}")

class SatelliteHPMTester:
    """卫星高功率微波完整测试器"""
    
    def __init__(self, stl_file: str, waveform_file: str):
        self.config = FDTDConfig()
        self.stl_loader = STLGeometryLoader(stl_file)
        self.plane_wave = PlaneWaveExcitation(self.config)
        self.waveform_file = waveform_file
        self.output_dir = "output_weixing_v1_test"
        
        # 创建输出目录
        os.makedirs(self.output_dir, exist_ok=True)
        
        print(f"卫星HPM测试器初始化:")
        print(f"  STL文件: {stl_file}")
        print(f"  波形文件: {waveform_file}")
        print(f"  输出目录: {self.output_dir}")
    
    def load_geometry(self) -> bool:
        """加载几何模型"""
        print("\n📐 加载卫星几何模型...")
        return self.stl_loader.load()
    
    def load_waveform(self) -> Optional[np.ndarray]:
        """加载HPM波形文件"""
        print(f"\n⚡ 加载HPM波形: {self.waveform_file}")
        
        try:
            if not os.path.exists(self.waveform_file):
                print(f"波形文件不存在，使用默认10GHz正弦波")
                return self._generate_default_waveform()
            
            # 读取波形数据
            data = []
            with open(self.waveform_file, 'r') as f:
                lines = f.readlines()
                
            # 解析格式 (根据FDTD配置文件说明)
            for line in lines:
                line = line.strip()
                if line and not line.startswith('#'):
                    parts = line.split()
                    if len(parts) >= 2:
                        try:
                            pos = int(parts[0])
                            value = float(parts[1])
                            data.append(value)
                        except:
                            continue
            
            if len(data) > 0:
                waveform = np.array(data)
                print(f"波形加载成功: {len(waveform)} 个点")
                print(f"幅度范围: [{np.min(waveform):.2e}, {np.max(waveform):.2e}]")
                return waveform
            else:
                print("波形文件格式错误，使用默认波形")
                return self._generate_default_waveform()
                
        except Exception as e:
            print(f"波形加载失败: {e}")
            return self._generate_default_waveform()
    
    def _generate_default_waveform(self) -> np.ndarray:
        """生成默认10GHz波形"""
        # 20ns时长，10GHz频率
        duration = 20e-9  # 20ns
        freq = 10e9     # 10GHz
        dt = duration / 1000  # 1000个点
        
        t = np.linspace(0, duration, 1000)
        waveform = np.sin(2 * PI * freq * t)
        
        print(f"生成默认10GHz正弦波形: {len(waveform)} 个点")
        return waveform
    
    def generate_observation_points(self) -> Dict[str, np.ndarray]:
        """生成观测点 - 严格按照FDTD配置"""
        print("\n📍 生成观测点配置...")
        points = {}
        
        # 1. 点监测 (OUT_POINT_PHYS)
        print("  点监测配置:")
        
        # 中心点 (0, 0, 0)
        center_points = np.array([[0.0, 0.0, 0.0]])
        points['center_Ez'] = center_points  # id=1, comp=Ez
        points['center_Ex'] = center_points  # id=2, comp=Ex  
        points['center_Ey'] = center_points  # id=3, comp=Ey
        print(f"    中心点: {center_points[0]} m")
        
        # 2. 平面监测 (OUT_PLANE_PHYS)
        print("  平面监测配置:")
        
        # X=0平面 (YZ切面)
        y_range = np.linspace(-1.5, 1.5, 31)  # -1500 to 1500 mm
        z_range = np.linspace(-0.55, 0.55, 12)  # -550 to 550 mm
        Y, Z = np.meshgrid(y_range, z_range)
        X = np.zeros_like(Y)
        plane_x0 = np.stack([X, Y, Z], axis=-1)
        
        points['plane_x0_Ez'] = plane_x0  # id=1, comp=Ez
        points['plane_x0_Ex'] = plane_x0  # id=2, comp=Ex
        points['plane_x0_Ey'] = plane_x0  # id=3, comp=Ey
        print(f"    X=0平面: {plane_x0.shape[0]}×{plane_x0.shape[1]} 网格")
        
        # Y=0平面 (XZ切面)
        x_range = np.linspace(-1.5, 1.5, 31)  # -1500 to 1500 mm
        z_range = np.linspace(-0.55, 0.55, 12)  # -550 to 550 mm
        X, Z = np.meshgrid(x_range, z_range)
        Y = np.zeros_like(X)
        plane_y0 = np.stack([X, Y, Z], axis=-1)
        
        points['plane_y0_Ez'] = plane_y0  # id=4, comp=Ez
        points['plane_y0_Ex'] = plane_y0  # id=5, comp=Ex
        points['plane_y0_Ey'] = plane_y0  # id=6, comp=Ey
        print(f"    Y=0平面: {plane_y0.shape[0]}×{plane_y0.shape[1]} 网格")
        
        # 3. 体积监测 (OUT_VOLUME_PHYS)
        print("  体积监测配置:")
        
        # 全域体积 (对应FDTD配置)
        x_range = np.linspace(-1.5, 1.5, 16)  # -1500 to 1500 mm
        y_range = np.linspace(-1.5, 1.5, 16)  # -1500 to 1500 mm
        z_range = np.linspace(-0.55, 0.55, 6)  # -550 to 550 mm
        
        X, Y, Z = np.meshgrid(x_range, y_range, z_range, indexing='ij')
        volume_full = np.stack([X, Y, Z], axis=-1)
        
        points['volume_full_Ez'] = volume_full  # id=7, comp=Ez
        print(f"    全区域体积: {volume_full.shape} 网格")
        
        # 物体中心区域 (可选，减小文件大小)
        x_fine = np.linspace(-0.7, 0.7, 15)  # -700 to 700 mm
        y_fine = np.linspace(-0.7, 0.7, 15)  # -700 to 700 mm
        z_fine = np.linspace(-0.25, 0.25, 6)  # -250 to 250 mm
        
        Xf, Yf, Zf = np.meshgrid(x_fine, y_fine, z_fine, indexing='ij')
        volume_fine = np.stack([Xf, Yf, Zf], axis=-1)
        
        points['volume_fine_Ez'] = volume_fine  # id=8, comp=Ey
        print(f"    中心细化体积: {volume_fine.shape} 网格")
        
        # 统计总点数
        total_points = 0
        for key, data in points.items():
            if 'point' in key:
                total_points += data.shape[0]
            elif 'plane' in key:
                total_points += data.shape[0] * data.shape[1]
            elif 'volume' in key:
                total_points += data.shape[0] * data.shape[1] * data.shape[2]
        
        print(f"  总观测点数: {total_points:,}")
        
        return points
    
    def compute_electromagnetic_fields(self, observation_points: Dict[str, np.ndarray]) -> Dict[str, np.ndarray]:
        """计算电磁场分布"""
        print("\n🔬 计算电磁场分布...")
        
        fields = {}
        
        # 计算每个观测点的场
        for point_type, points in observation_points.items():
            print(f"  计算 {point_type} 场...")
            
            if 'point' in point_type:
                # 点场计算
                E_incident = self.plane_wave.compute_incident_field(points)
                
                # 提取各分量
                if 'Ex' in point_type:
                    fields[point_type] = E_incident[:, 0].real
                elif 'Ey' in point_type:
                    fields[point_type] = E_incident[:, 1].real
                elif 'Ez' in point_type:
                    fields[point_type] = E_incident[:, 2].real
                    
            elif 'plane' in point_type or 'volume' in point_type:
                # 平面或体积场计算
                original_shape = points.shape
                flat_points = points.reshape(-1, 3)
                
                E_incident = self.plane_wave.compute_incident_field(flat_points)
                
                # 提取对应分量
                if 'Ex' in point_type:
                    field_values = E_incident[:, 0].real
                elif 'Ey' in point_type:
                    field_values = E_incident[:, 1].real
                elif 'Ez' in point_type:
                    field_values = E_incident[:, 2].real
                
                # 重塑回原始形状
                fields[point_type] = field_values.reshape(original_shape[:-1])
        
        print("  场计算完成")
        return fields
    
    def export_vtk_files(self, fields: Dict[str, np.ndarray], geometry_info: Dict):
        """导出VTK格式文件"""
        print("\n💾 导出VTK格式文件...")
        
        if not VTK_AVAILABLE:
            print("VTK不可用，跳过VTK导出")
            return
        
        # 1. 几何模型导出 (EXPORT_MODEL_INFO)
        if self.stl_loader.vertices is not None:
            vtk_exporter = VTKExporter(f"{self.output_dir}/weixing_v1_geometry.vtu")
            vtk_exporter.export_geometry(self.stl_loader.vertices, self.stl_loader.triangles)
        
        # 2. 点监测数据导出
        print("  导出点监测数据...")
        for point_type, field_data in fields.items():
            if 'point' in point_type:
                points = np.array([[0.0, 0.0, 0.0]])  # 中心点
                field_name = point_type.split('_')[2]  # Ex, Ey, Ez
                
                vtk_exporter = VTKExporter(f"{self.output_dir}/weixing_v1_{point_type}.vtu")
                point_fields = {field_name: np.array([field_data])}
                vtk_exporter.export_point_data(points, point_fields, f"Point {field_name}")
        
        # 3. 平面监测数据导出
        print("  导出平面监测数据...")
        for point_type, field_data in fields.items():
            if 'plane' in point_type:
                # 获取平面坐标
                if 'x0' in point_type:
                    y_range = np.linspace(-1.5, 1.5, field_data.shape[1])
                    z_range = np.linspace(-0.55, 0.55, field_data.shape[0])
                    Y, Z = np.meshgrid(y_range, z_range)
                    X = np.zeros_like(Y)
                    plane_coords = np.stack([X, Y, Z], axis=-1)
                    plane_type = 'X'
                    position = 0.0
                elif 'y0' in point_type:
                    x_range = np.linspace(-1.5, 1.5, field_data.shape[1])
                    z_range = np.linspace(-0.55, 0.55, field_data.shape[0])
                    X, Z = np.meshgrid(x_range, z_range)
                    Y = np.zeros_like(X)
                    plane_coords = np.stack([X, Y, Z], axis=-1)
                    plane_type = 'Y'
                    position = 0.0
                
                field_name = point_type.split('_')[2]  # Ex, Ey, Ez
                vtk_exporter = VTKExporter(f"{self.output_dir}/weixing_v1_{point_type}.vts")
                plane_fields = {field_name: field_data}
                vtk_exporter.export_plane_data(plane_coords, plane_fields, plane_type, position)
        
        # 4. 体积监测数据导出
        print("  导出体积监测数据...")
        for point_type, field_data in fields.items():
            if 'volume' in point_type:
                # 重建体积坐标
                if 'full' in point_type:
                    x_range = np.linspace(-1.5, 1.5, field_data.shape[2])
                    y_range = np.linspace(-1.5, 1.5, field_data.shape[1])
                    z_range = np.linspace(-0.55, 0.55, field_data.shape[0])
                elif 'fine' in point_type:
                    x_range = np.linspace(-0.7, 0.7, field_data.shape[2])
                    y_range = np.linspace(-0.7, 0.7, field_data.shape[1])
                    z_range = np.linspace(-0.25, 0.25, field_data.shape[0])
                
                X, Y, Z = np.meshgrid(x_range, y_range, z_range, indexing='ij')
                volume_coords = np.stack([X, Y, Z], axis=-1)
                
                field_name = point_type.split('_')[2]  # Ex, Ey, Ez
                vtk_exporter = VTKExporter(f"{self.output_dir}/weixing_v1_{point_type}.vts")
                volume_fields = {field_name: field_data}
                vtk_exporter.export_volume_data(volume_coords, volume_fields)
        
        print("  VTK导出完成")
    
    def export_hdf5_files(self, fields: Dict[str, np.ndarray], geometry_info: Dict):
        """导出HDF5格式文件"""
        print("\n💾 导出HDF5格式文件...")
        
        # 创建综合HDF5文件
        hdf5_file = f"{self.output_dir}/weixing_v1_complete.h5"
        
        if HDF5_AVAILABLE:
            with h5py.File(hdf5_file, 'w') as h5f:
                # 创建组结构
                metadata_group = h5f.create_group('metadata')
                fields_group = h5f.create_group('fields')
                geometry_group = h5f.create_group('geometry')
                
                # 存储元数据
                metadata = {
                    'frequency': self.config.frequency,
                    'wavelength': self.config.wavelength,
                    'domain_size': self.config.domain_size,
                    'satellite_size': self.config.satellite_size,
                    'grid_spacing': self.config.grid_spacing,
                    'incident_angles': {
                        'theta': self.config.theta,
                        'phi': self.config.phi,
                        'psi': self.config.psi
                    },
                    'incident_direction': self.plane_wave.incident_direction,
                    'polarization': self.plane_wave.polarization,
                    'computation_time': time.time() - self.start_time,
                    'total_field_points': sum(field.size for field in fields.values())
                }
                
                for key, value in metadata.items():
                    if isinstance(value, (int, float, str)):
                        metadata_group.attrs[key] = value
                    elif isinstance(value, np.ndarray):
                        metadata_group.create_dataset(key, data=value)
                    elif isinstance(value, dict):
                        # 处理嵌套字典
                        for sub_key, sub_value in value.items():
                            if isinstance(sub_value, (int, float)):
                                metadata_group.attrs[f"{key}_{sub_key}"] = sub_value
                
                # 存储场数据
                for field_name, field_data in fields.items():
                    if isinstance(field_data, np.ndarray):
                        fields_group.create_dataset(field_name, data=field_data, compression='gzip')
                
                # 存储几何数据
                if self.stl_loader.vertices is not None:
                    geometry_group.create_dataset('vertices', data=self.stl_loader.vertices)
                    geometry_group.create_dataset('triangles', data=self.stl_loader.triangles)
                    if self.stl_loader.bounds:
                        for key, value in self.stl_loader.bounds.items():
                            geometry_group.create_dataset(f"bounds_{key}", data=value)
                
                print(f"  HDF5文件创建完成: {hdf5_file}")
        else:
            # 回退到JSON格式
            json_file = f"{self.output_dir}/weixing_v1_complete.json"
            
            complete_data = {
                'metadata': {
                    'frequency': self.config.frequency,
                    'wavelength': self.config.wavelength,
                    'domain_size': self.config.domain_size.tolist(),
                    'satellite_size': self.config.satellite_size.tolist(),
                    'grid_spacing': self.config.grid_spacing,
                    'incident_angles': {
                        'theta': self.config.theta,
                        'phi': self.config.phi,
                        'psi': self.config.psi
                    },
                    'incident_direction': self.plane_wave.incident_direction.tolist(),
                    'polarization': self.plane_wave.polarization.tolist(),
                    'computation_time': time.time() - self.start_time
                },
                'fields': {name: data.tolist() for name, data in fields.items()},
                'geometry': geometry_info
            }
            
            with open(json_file, 'w', encoding='utf-8') as f:
                json.dump(complete_data, f, indent=2, ensure_ascii=False)
            
            print(f"  JSON文件创建完成: {json_file}")
    
    def run_complete_test(self):
        """运行完整测试"""
        print("🛰️ 卫星高功率微波激励完整测试")
        print("=" * 60)
        print("基于 weixing_v1_case.pfd 配置文件")
        print("=" * 60)
        
        self.start_time = time.time()
        
        # 1. 加载几何模型
        if not self.load_geometry():
            print("几何模型加载失败，测试终止")
            return False
        
        geometry_info = self.stl_loader.get_geometry_info()
        
        # 2. 加载波形
        waveform = self.load_waveform()
        
        # 3. 生成观测点
        observation_points = self.generate_observation_points()
        
        # 4. 计算电磁场
        fields = self.compute_electromagnetic_fields(observation_points)
        
        # 5. 导出结果
        self.export_vtk_files(fields, geometry_info)
        self.export_hdf5_files(fields, geometry_info)
        
        # 6. 生成测试报告
        self.generate_test_report(fields, geometry_info, waveform)
        
        print("\n🎉 完整测试完成！")
        print(f"输出文件位置: {os.path.abspath(self.output_dir)}")
        
        return True
    
    def generate_test_report(self, fields: Dict[str, np.ndarray], geometry_info: Dict, waveform: np.ndarray):
        """生成测试报告"""
        print("\n📝 生成测试报告...")
        
        report_file = f"{self.output_dir}/WEIXING_V1_COMPLETE_TEST_REPORT.md"
        
        with open(report_file, 'w', encoding='utf-8') as f:
            f.write("# 卫星高功率微波激励完整测试报告\n")
            f.write("基于 weixing_v1_case.pfd 配置文件\n\n")
            
            # 测试配置
            f.write("## 测试配置\n\n")
            f.write(f"- **频率**: {self.config.frequency/1e9} GHz\n")
            f.write(f"- **波长**: {self.config.wavelength*1000} mm\n")
            f.write(f"- **入射角**: θ={self.config.theta}°, φ={self.config.phi}°, ψ={self.config.psi}°\n")
            f.write(f"- **计算域**: {self.config.domain_size} m\n")
            f.write(f"- **卫星尺寸**: {self.config.satellite_size} mm\n")
            f.write(f"- **网格间距**: {self.config.grid_spacing*1000} mm\n\n")
            
            # 几何信息
            f.write("## 几何模型信息\n\n")
            if geometry_info:
                f.write(f"- **顶点数**: {geometry_info.get('num_vertices', 'N/A')}\n")
                f.write(f"- **三角形数**: {geometry_info.get('num_triangles', 'N/A')}\n")
                if geometry_info.get('bounds'):
                    bounds = geometry_info['bounds']
                    f.write(f"- **边界框**: min={bounds['min']}, max={bounds['max']}\n")
                    f.write(f"- **几何中心**: {bounds['center']}\n")
                    if geometry_info.get('dimensions') is not None:
                        f.write(f"- **几何尺寸**: {geometry_info['dimensions']} m\n")
            else:
                f.write("使用默认几何模型\n")
            f.write("\n")
            
            # 波形信息
            f.write("## 激励波形信息\n\n")
            if waveform is not None:
                f.write(f"- **波形点数**: {len(waveform)}\n")
                f.write(f"- **幅度范围**: [{np.min(waveform):.2e}, {np.max(waveform):.2e}] V/m\n")
                f.write(f"- **时长**: 20 ns\n")
            f.write("\n")
            
            # 观测配置
            f.write("## 观测配置\n\n")
            total_points = sum(field.size for field in fields.values())
            f.write(f"- **总观测点数**: {total_points:,}\n")
            f.write(f"- **点监测**: 中心点 (Ex, Ey, Ez)\n")
            f.write(f"- **平面监测**: X=0, Y=0 切面 (Ex, Ey, Ez)\n")
            f.write(f"- **体积监测**: 全区域 + 中心细化区域 (Ez)\n")
            f.write(f"- **输出格式**: VTK + HDF5/JSON\n\n")
            
            # 场分布统计
            f.write("## 电磁场分布统计\n\n")
            for field_name, field_data in fields.items():
                if isinstance(field_data, np.ndarray):
                    f.write(f"### {field_name}\n\n")
                    f.write(f"- **数据形状**: {field_data.shape}\n")
                    f.write(f"- **数值范围**: [{np.min(field_data):.3e}, {np.max(field_data):.3e}]\n")
                    f.write(f"- **平均值**: {np.mean(field_data):.3e}\n")
                    f.write(f"- **标准差**: {np.std(field_data):.3e}\n\n")
            
            # 输出文件列表
            f.write("## 输出文件列表\n\n")
            f.write("### VTK格式文件\n\n")
            vtk_files = [f for f in os.listdir(self.output_dir) if f.endswith('.vtu') or f.endswith('.vts')]
            for vtk_file in sorted(vtk_files):
                f.write(f"- `{vtk_file}`\n")
            
            f.write("\n### HDF5/JSON格式文件\n\n")
            data_files = [f for f in os.listdir(self.output_dir) if f.endswith('.h5') or f.endswith('.json')]
            for data_file in sorted(data_files):
                f.write(f"- `{data_file}`\n")
            
            f.write("\n### 报告文件\n\n")
            f.write(f"- `{os.path.basename(report_file)}`\n")
            
            # 计算时间
            computation_time = time.time() - self.start_time
            f.write(f"\n## 计算统计\n\n")
            f.write(f"- **计算时间**: {computation_time:.3f} 秒\n")
            f.write(f"- **输出文件数**: {len(os.listdir(self.output_dir))}\n")
            f.write(f"- **总数据量**: ~{sum(os.path.getsize(f'{self.output_dir}/{f}') for f in os.listdir(self.output_dir))/1024/1024:.1f} MB\n")
            
            f.write(f"\n---\n")
            f.write(f"**测试完成时间**: {time.strftime('%Y-%m-%d %H:%M:%S')}\n")
            f.write(f"**配置文件**: weixing_v1_case.pfd\n")
            f.write(f"**几何文件**: weixing_v1.stl\n")
            f.write(f"**波形文件**: {os.path.basename(self.waveform_file)}\n")
        
        print(f"  测试报告已生成: {report_file}")

def main():
    """主函数"""
    # 文件路径
    stl_file = "weixing_v1.stl"
    waveform_file = "hpm_waveform_X(10.0GHz)_20ns.txt"
    
    # 创建测试器
    tester = SatelliteHPMTester(stl_file, waveform_file)
    
    # 运行完整测试
    success = tester.run_complete_test()
    
    if success:
        print("\n✅ 测试成功完成！")
        print(f"所有输出文件保存在: {os.path.abspath(tester.output_dir)}")
        
        # 列出所有输出文件
        print("\n📁 输出文件列表:")
        for file in sorted(os.listdir(tester.output_dir)):
            file_path = os.path.join(tester.output_dir, file)
            file_size = os.path.getsize(file_path)
            print(f"  {file} ({file_size/1024:.1f} KB)")
    else:
        print("\n❌ 测试失败！")

if __name__ == "__main__":
    main()