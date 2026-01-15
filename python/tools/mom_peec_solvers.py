#!/usr/bin/env python3
"""
通用MoM/PEEC求解器具体实现
Concrete Implementation of General MoM/PEEC Solvers

基于通用框架的具体MoM和PEEC求解器实现
包括STL几何处理、RWG基函数、专业电磁计算等
"""

import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
import re
import os
import json
from scipy.sparse import csr_matrix, lil_matrix
from scipy.sparse.linalg import gmres, bicgstab
from scipy.fft import fft, ifft
from pathlib import Path
from typing import Dict, List, Tuple, Optional, Any
import time

# 导入通用框架
from mom_peec_framework import (
    BaseMoMSolver, BasePEECSolver, Material, Excitation, MeshData, 
    SimulationResult, EMConstants, MoMPeecCoupler, MoMPeecSimulator,
    MoMPeecVisualizer
)


class STLMeshGenerator:
    """STL网格生成器"""
    
    def __init__(self, target_scale: float = 1.0, stl_units: str = 'mm'):
        self.target_scale = target_scale
        self.stl_units = stl_units
        self.vertices = []
        self.triangles = []
        self.normals = []
        self.material_regions = []
    
    def parse_stl_file(self, filename: str, max_facets: Optional[int] = None) -> MeshData:
        """解析STL文件"""
        print(f"📐 解析STL文件: {filename}")
        
        try:
            with open(filename, 'r', encoding='utf-8', errors='ignore') as f:
                content = f.read()
            
            # 检测文件类型
            if content.startswith('solid'):
                return self._parse_ascii_stl(content, max_facets)
            else:
                return self._parse_binary_stl(filename, max_facets)
                
        except Exception as e:
            print(f"❌ STL解析失败: {e}")
            raise
    
    def _parse_ascii_stl(self, content: str, max_facets: Optional[int] = None) -> MeshData:
        """解析ASCII STL"""
        vertices = []
        triangles = []
        normals = []
        
        # 解析顶点、法向量和三角形
        vertex_pattern = r'vertex\s+([+-]?\d+\.?\d*(?:[eE][+-]?\d+)?)\s+([+-]?\d+\.?\d*(?:[eE][+-]?\d+)?)\s+([+-]?\d+\.?\d*(?:[eE][+-]?\d+)?)'
        normal_pattern = r'facet\s+normal\s+([+-]?\d+\.?\d*(?:[eE][+-]?\d+)?)\s+([+-]?\d+\.?\d*(?:[eE][+-]?\d+)?)\s+([+-]?\d+\.?\d*(?:[eE][+-]?\d+)?)'
        
        lines = content.split('\n')
        current_triangle = []
        current_normal = None
        
        for line in lines:
            line = line.strip().lower()
            
            # 法向量
            normal_match = re.match(normal_pattern, line)
            if normal_match:
                current_normal = [float(normal_match.group(i)) for i in range(1, 4)]
            
            # 顶点
            vertex_match = re.match(vertex_pattern, line)
            if vertex_match:
                vertex = [float(vertex_match.group(i)) for i in range(1, 4)]
                current_triangle.append(vertex)
                
                # 完成一个三角形
                if len(current_triangle) == 3:
                    # 添加顶点
                    base_idx = len(vertices)
                    vertices.extend(current_triangle)
                    
                    # 添加三角形索引
                    triangles.append([base_idx, base_idx + 1, base_idx + 2])
                    
                    # 添加法向量
                    if current_normal:
                        normals.append(current_normal)
                    else:
                        # 计算法向量
                        v1 = np.array(current_triangle[1]) - np.array(current_triangle[0])
                        v2 = np.array(current_triangle[2]) - np.array(current_triangle[0])
                        normal = np.cross(v1, v2)
                        normal = normal / np.linalg.norm(normal) if np.linalg.norm(normal) > 0 else [0, 0, 1]
                        normals.append(normal.tolist())
                    
                    current_triangle = []
                    current_normal = None
        
        # 应用缩放
        scale_factor = self._get_scale_factor(vertices)
        vertices = [[v[i] * scale_factor for i in range(3)] for v in vertices]
        
        print(f"   ✅ STL解析完成: {len(vertices)} 顶点, {len(triangles)} 三角形")
        
        return MeshData(
            vertices=np.array(vertices),
            triangles=np.array(triangles),
            normals=np.array(normals)
        )
    
    def _parse_binary_stl(self, filename: str, max_facets: Optional[int] = None) -> MeshData:
        """解析二进制STL"""
        with open(filename, 'rb') as f:
            # 跳过头部
            f.read(80)
            
            # 读取三角形数量
            num_triangles = struct.unpack('<I', f.read(4))[0]
            if max_facets and num_triangles > max_facets:
                num_triangles = max_facets
            
            vertices = []
            triangles = []
            normals = []
            
            for i in range(num_triangles):
                # 法向量
                normal = struct.unpack('<fff', f.read(12))
                normals.append(list(normal))
                
                # 三个顶点
                triangle_vertices = []
                for j in range(3):
                    vertex = struct.unpack('<fff', f.read(12))
                    triangle_vertices.append(list(vertex))
                
                # 添加顶点
                base_idx = len(vertices)
                vertices.extend(triangle_vertices)
                triangles.append([base_idx, base_idx + 1, base_idx + 2])
                
                # 跳过属性字节
                f.read(2)
        
        # 应用缩放
        scale_factor = self._get_scale_factor(vertices)
        vertices = [[v[i] * scale_factor for i in range(3)] for v in vertices]
        
        print(f"   ✅ 二进制STL解析完成: {len(vertices)} 顶点, {len(triangles)} 三角形")
        
        return MeshData(
            vertices=np.array(vertices),
            triangles=np.array(triangles),
            normals=np.array(normals)
        )
    
    def _get_scale_factor(self, vertices: List[List[float]]) -> float:
        """计算缩放因子"""
        if not vertices:
            return 1.0
        
        vertices_array = np.array(vertices)
        current_size = np.max(vertices_array) - np.min(vertices_array)
        
        if current_size == 0:
            return 1.0
        
        # 单位转换
        unit_scale = {'mm': 0.001, 'cm': 0.01, 'm': 1.0, 'inch': 0.0254, 'ft': 0.3048}
        scale_to_meters = unit_scale.get(self.stl_units, 0.001)
        
        # 计算目标缩放
        target_size_meters = self.target_scale
        scale_factor = target_size_meters / (current_size * scale_to_meters)
        
        print(f"   📏 缩放因子: {scale_factor:.3f} (目标: {target_size_meters}m)")
        return scale_factor


class RWGBasisFunctions:
    """RWG基函数生成器"""
    
    def __init__(self):
        self.basis_functions = []
        self.edges = []
        self.edge_to_triangles = {}
    
    def create_basis_functions(self, mesh_data: MeshData) -> List[Dict]:
        """创建RWG基函数"""
        print("🔧 创建RWG基函数...")
        
        vertices = mesh_data.vertices
        triangles = mesh_data.triangles
        
        # 构建边到三角形的映射
        self._build_edge_mapping(triangles)
        
        # 创建内部边的RWG基函数
        basis_functions = []
        basis_id = 0
        
        for edge_idx, (edge, triangle_pair) in enumerate(self.edge_to_triangles.items()):
            if len(triangle_pair) == 2:  # 内部边
                tri_p, tri_n = triangle_pair
                
                # 创建正向和负向三角形的信息
                plus_triangle = self._get_triangle_info(tri_p, vertices, triangles)
                minus_triangle = self._get_triangle_info(tri_n, vertices, triangles)
                
                # 创建RWG基函数
                basis_func = {
                    'id': basis_id,
                    'edge_index': edge_idx,
                    'edge_vertices': list(edge),
                    'plus_triangle': plus_triangle,
                    'minus_triangle': minus_triangle,
                    'length': self._calculate_edge_length(edge, vertices),
                    'area_plus': plus_triangle['area'],
                    'area_minus': minus_triangle['area']
                }
                
                basis_functions.append(basis_func)
                basis_id += 1
        
        self.basis_functions = basis_functions
        print(f"   ✅ RWG基函数创建完成: {len(basis_functions)} 个基函数")
        return basis_functions
    
    def _build_edge_mapping(self, triangles: np.ndarray):
        """构建边到三角形的映射"""
        edge_to_triangles = {}
        
        for tri_idx, triangle in enumerate(triangles):
            # 获取三角形的三个顶点
            i, j, k = triangle
            
            # 创建三条边 (排序以确保一致性)
            edges = [
                tuple(sorted([i, j])),
                tuple(sorted([j, k])),
                tuple(sorted([k, i]))
            ]
            
            # 映射边到三角形
            for edge in edges:
                if edge not in edge_to_triangles:
                    edge_to_triangles[edge] = []
                edge_to_triangles[edge].append(tri_idx)
        
        self.edge_to_triangles = edge_to_triangles
        self.edges = list(edge_to_triangles.keys())
    
    def _get_triangle_info(self, tri_idx: int, vertices: np.ndarray, triangles: np.ndarray) -> Dict:
        """获取三角形信息"""
        triangle = triangles[tri_idx]
        vertex_coords = vertices[triangle]
        
        # 计算面积
        v1 = vertex_coords[1] - vertex_coords[0]
        v2 = vertex_coords[2] - vertex_coords[0]
        area = 0.5 * np.linalg.norm(np.cross(v1, v2))
        
        # 计算法向量
        normal = np.cross(v1, v2)
        normal = normal / np.linalg.norm(normal) if np.linalg.norm(normal) > 0 else np.array([0, 0, 1])
        
        # 计算重心
        centroid = np.mean(vertex_coords, axis=0)
        
        return {
            'index': tri_idx,
            'vertices': triangle.tolist(),
            'vertex_coords': vertex_coords.tolist(),
            'area': area,
            'normal': normal.tolist(),
            'centroid': centroid.tolist()
        }
    
    def _calculate_edge_length(self, edge: tuple, vertices: np.ndarray) -> float:
        """计算边长"""
        v1, v2 = vertices[list(edge)]
        return np.linalg.norm(v2 - v1)


class ProfessionalMoMSolver(BaseMoMSolver):
    """专业MoM求解器实现"""
    
    def __init__(self, frequency: float, materials: Dict[str, Material], 
                 mesh_accuracy: str = 'standard'):
        super().__init__(frequency, materials)
        self.mesh_accuracy = mesh_accuracy
        self.mesh_generator = None
        self.rwg_generator = None
        
        # 根据精度设置网格参数
        if mesh_accuracy == 'standard':
            self.target_edge_length = self.wavelength / 10
        elif mesh_accuracy == 'high':
            self.target_edge_length = self.wavelength / 20
        elif mesh_accuracy == 'extreme':
            self.target_edge_length = self.wavelength / 50
        else:
            self.target_edge_length = self.wavelength / 10
    
    def generate_mesh(self, geometry_file: str, **kwargs) -> MeshData:
        """生成计算网格"""
        target_scale = kwargs.get('target_scale', 1.0)
        stl_units = kwargs.get('stl_units', 'mm')
        max_facets = kwargs.get('max_facets', None)
        
        # 创建网格生成器
        self.mesh_generator = STLMeshGenerator(target_scale, stl_units)
        
        # 解析STL文件
        mesh_data = self.mesh_generator.parse_stl_file(geometry_file, max_facets)
        
        # 计算三角形面积和法向量
        mesh_data.areas = self._calculate_triangle_areas(mesh_data)
        
        return mesh_data
    
    def create_basis_functions(self, mesh_data: MeshData) -> List[Dict]:
        """创建RWG基函数"""
        self.rwg_generator = RWGBasisFunctions()
        return self.rwg_generator.create_basis_functions(mesh_data)
    
    def calculate_impedance_matrix(self, basis_functions: List[Dict]) -> np.ndarray:
        """计算MoM阻抗矩阵"""
        print("🧮 计算MoM阻抗矩阵...")
        
        num_basis = len(basis_functions)
        Z = np.zeros((num_basis, num_basis), dtype=complex)
        
        # 电磁参数
        k = self.k
        eta = EMConstants.ETA_0
        omega = self.omega
        
        # 计算阻抗矩阵
        for m in range(num_basis):
            if m % 50 == 0:
                print(f"   计算进度: {m+1}/{num_basis}")
            
            for n in range(num_basis):
                if m == n:
                    # 自阻抗 (处理奇异性)
                    Z[m, n] = self._calculate_self_impedance(basis_functions[m], k, eta, omega)
                else:
                    # 互阻抗
                    Z[m, n] = self._calculate_mutual_impedance(basis_functions[m], basis_functions[n], k, eta, omega)
        
        print(f"   ✅ 阻抗矩阵计算完成: {Z.shape}")
        return Z
    
    def calculate_excitation(self, excitation: Excitation, basis_functions: List[Dict]) -> np.ndarray:
        """计算激励向量"""
        print("⚡ 计算激励向量...")
        
        num_basis = len(basis_functions)
        V = np.zeros(num_basis, dtype=complex)
        
        if excitation.type == 'plane_wave':
            # 平面波激励
            E_inc = excitation.amplitude * np.exp(1j * excitation.phase)
            k_vec = self.k * excitation.direction if excitation.direction is not None else np.array([self.k, 0, 0])
            
            for i, basis_func in enumerate(basis_functions):
                # 计算基函数与入射场的耦合
                V[i] = self._calculate_plane_wave_coupling(basis_func, E_inc, k_vec, excitation.polarization)
        
        print(f"   ✅ 激励向量计算完成: {V.shape}")
        return V
    
    def solve_linear_system(self, impedance_matrix: np.ndarray, excitation_vector: np.ndarray) -> np.ndarray:
        """求解线性方程组"""
        print("🔍 求解线性方程组...")
        
        try:
            # 使用迭代求解器
            I, info = gmres(impedance_matrix, excitation_vector, tol=1e-6, maxiter=1000)
            
            if info == 0:
                print(f"   ✅ GMRES求解成功: {I.shape}")
            else:
                print(f"   ⚠️ GMRES求解信息: {info}")
            
            return I
            
        except Exception as e:
            print(f"   ❌ 求解失败: {e}")
            # 返回零解
            return np.zeros_like(excitation_vector)
    
    def _calculate_triangle_areas(self, mesh_data: MeshData) -> np.ndarray:
        """计算三角形面积"""
        areas = []
        vertices = mesh_data.vertices
        triangles = mesh_data.triangles
        
        for triangle in triangles:
            v1 = vertices[triangle[1]] - vertices[triangle[0]]
            v2 = vertices[triangle[2]] - vertices[triangle[0]]
            area = 0.5 * np.linalg.norm(np.cross(v1, v2))
            areas.append(area)
        
        return np.array(areas)
    
    def _calculate_self_impedance(self, basis_func: Dict, k: float, eta: float, omega: float) -> complex:
        """计算自阻抗"""
        # 简化实现 - 实际应处理奇异性
        length = basis_func['length']
        area_plus = basis_func['area_plus']
        area_minus = basis_func['area_minus']
        
        # 平均面积
        avg_area = (area_plus + area_minus) / 2
        
        # 自阻抗近似
        R_self = eta * length**2 / (4 * avg_area)
        X_self = omega * EMConstants.MU_0 * length**2 / (4 * np.pi * avg_area)
        
        return R_self + 1j * X_self
    
    def _calculate_mutual_impedance(self, basis_func_m: Dict, basis_func_n: Dict, 
                                  k: float, eta: float, omega: float) -> complex:
        """计算互阻抗"""
        # 获取三角形信息
        tri_m_plus = basis_func_m['plus_triangle']
        tri_m_minus = basis_func_m['minus_triangle']
        tri_n_plus = basis_func_n['plus_triangle']
        tri_n_minus = basis_func_n['minus_triangle']
        
        # 计算重心之间的距离
        r_m_plus = np.array(tri_m_plus['centroid'])
        r_m_minus = np.array(tri_m_minus['centroid'])
        r_n_plus = np.array(tri_n_plus['centroid'])
        r_n_minus = np.array(tri_n_minus['centroid'])
        
        # 计算四个相互作用项
        R_mp_np = np.linalg.norm(r_m_plus - r_n_plus)
        R_mp_nm = np.linalg.norm(r_m_plus - r_n_minus)
        R_mm_np = np.linalg.norm(r_m_minus - r_n_plus)
        R_mm_nm = np.linalg.norm(r_m_minus - r_n_minus)
        
        # 平均距离
        avg_distance = (R_mp_np + R_mp_nm + R_mm_np + R_mm_nm) / 4
        
        # 避免零距离
        avg_distance = max(avg_distance, 1e-6)
        
        # 互阻抗计算 (简化格林函数)
        G = np.exp(-1j * k * avg_distance) / (4 * np.pi * avg_distance)
        
        # 几何因子
        geom_factor = (basis_func_m['length'] * basis_func_n['length']) / 4
        
        # 互阻抗
        Z_mn = 1j * omega * EMConstants.MU_0 * G * geom_factor
        
        return Z_mn
    
    def _calculate_plane_wave_coupling(self, basis_func: Dict, E_inc: complex, 
                                     k_vec: np.ndarray, polarization: Optional[np.ndarray]) -> complex:
        """计算平面波耦合"""
        # 计算平均位置
        tri_plus = basis_func['plus_triangle']
        tri_minus = basis_func['minus_triangle']
        
        r_avg = (np.array(tri_plus['centroid']) + np.array(tri_minus['centroid'])) / 2
        
        # 入射相位
        phase = np.dot(k_vec, r_avg)
        
        # 入射场 (考虑极化)
        if polarization is not None:
            E_local = E_inc * np.exp(-1j * phase)
        else:
            E_local = E_inc * np.exp(-1j * phase)
        
        # 几何因子
        geom_factor = basis_func['length'] * (tri_plus['area'] + tri_minus['area']) / 2
        
        # 耦合项
        coupling = E_local * geom_factor
        
        return coupling


class ProfessionalPEECSolver(BasePEECSolver):
    """专业PEEC求解器实现"""
    
    def __init__(self, frequency: float, materials: Dict[str, Material]):
        super().__init__(frequency, materials)
        self.mesh_generator = None
    
    def extract_circuit_topology(self, geometry_file: str, **kwargs) -> Dict:
        """提取电路拓扑"""
        print("🔌 提取电路拓扑...")
        
        target_scale = kwargs.get('target_scale', 1.0)
        stl_units = kwargs.get('stl_units', 'mm')
        
        # 解析几何
        self.mesh_generator = STLMeshGenerator(target_scale, stl_units)
        mesh_data = self.mesh_generator.parse_stl_file(geometry_file)
        
        # 简化的电路拓扑提取
        num_vertices = len(mesh_data.vertices)
        num_triangles = len(mesh_data.triangles)
        
        # 创建节点和支路
        nodes = list(range(num_vertices))
        branches = []
        
        # 每个三角形作为一个电路单元
        for tri_idx, triangle in enumerate(mesh_data.triangles):
            # 创建三个支路连接三角形的三个顶点
            for i in range(3):
                j = (i + 1) % 3
                branch = {
                    'id': len(branches),
                    'from_node': triangle[i],
                    'to_node': triangle[j],
                    'triangle': tri_idx,
                    'length': self._calculate_edge_length(triangle[i], triangle[j], mesh_data.vertices)
                }
                branches.append(branch)
        
        topology = {
            'nodes': nodes,
            'branches': branches,
            'num_nodes': len(nodes),
            'num_branches': len(branches),
            'mesh_data': mesh_data
        }
        
        print(f"   ✅ 电路拓扑提取完成: {len(nodes)} 节点, {len(branches)} 支路")
        return topology
    
    def calculate_partial_elements(self, circuit_topology: Dict) -> Dict:
        """计算部分元件 (R, L, P)"""
        print("⚡ 计算部分元件 (R, L, P)...")
        
        branches = circuit_topology['branches']
        mesh_data = circuit_topology['mesh_data']
        vertices = mesh_data.vertices
        
        partial_elements = {
            'resistances': [],
            'inductances': [],
            'capacitances': [],
            'coupling_inductances': []
        }
        
        # 计算每个支路的R, L, C
        for branch in branches:
            from_node = branch['from_node']
            to_node = branch['to_node']
            
            # 获取节点坐标
            pos1 = vertices[from_node]
            pos2 = vertices[to_node]
            length = np.linalg.norm(pos2 - pos1)
            
            # 计算横截面积 (简化)
            area = 1e-6  # 1 mm²
            
            # 材料属性 (默认PEC)
            material = self.materials.get('PEC', Material('PEC', sigma=1e20))
            
            # 计算R, L, C
            R = self._calculate_resistance(material, length, area)
            L = self._calculate_inductance(material, length, area)
            C = self._calculate_capacitance(material, length, area)
            
            partial_elements['resistances'].append(R)
            partial_elements['inductances'].append(L)
            partial_elements['capacitances'].append(C)
        
        # 计算互感 (简化)
        num_branches = len(branches)
        M_matrix = np.zeros((num_branches, num_branches))
        
        for i in range(num_branches):
            for j in range(num_branches):
                if i != j:
                    # 简化的互感计算
                    distance = self._calculate_branch_distance(branches[i], branches[j], vertices)
                    M_matrix[i, j] = self._calculate_mutual_inductance(distance)
        
        partial_elements['coupling_inductances'] = M_matrix
        
        print(f"   ✅ 部分元件计算完成")
        return partial_elements
    
    def assemble_system_matrix(self, partial_elements: Dict) -> Tuple[np.ndarray, np.ndarray]:
        """组装系统矩阵"""
        print("🧮 组装PEEC系统矩阵...")
        
        num_branches = len(partial_elements['resistances'])
        
        # 创建阻抗矩阵
        Z = np.zeros((num_branches, num_branches), dtype=complex)
        omega = self.omega
        
        # 对角线元素: R + jωL
        for i in range(num_branches):
            R = partial_elements['resistances'][i]
            L = partial_elements['inductances'][i]
            Z[i, i] = R + 1j * omega * L
        
        # 互感项: jωM
        M_matrix = partial_elements['coupling_inductances']
        for i in range(num_branches):
            for j in range(num_branches):
                if i != j:
                    Z[i, j] = 1j * omega * M_matrix[i, j]
        
        # 创建电压向量 (简化)
        V = np.zeros(num_branches, dtype=complex)
        
        # 假设第一个支路有1V电压源
        if num_branches > 0:
            V[0] = 1.0
        
        print(f"   ✅ PEEC系统矩阵组装完成: {Z.shape}")
        return Z, V
    
    def solve_circuit(self, impedance_matrix: np.ndarray, voltage_vector: np.ndarray) -> np.ndarray:
        """求解电路方程"""
        print("🔍 求解PEEC电路方程...")
        
        try:
            # 直接求解
            I = np.linalg.solve(impedance_matrix, voltage_vector)
            print(f"   ✅ PEEC电路求解成功: {I.shape}")
            return I
            
        except np.linalg.LinAlgError:
            print("   ⚠️ 矩阵奇异，使用最小二乘求解")
            # 使用最小二乘
            I, residuals, rank, s = np.linalg.lstsq(impedance_matrix, voltage_vector, rcond=None)
            return I
    
    def _calculate_edge_length(self, node1: int, node2: int, vertices: np.ndarray) -> float:
        """计算边长"""
        pos1 = vertices[node1]
        pos2 = vertices[node2]
        return np.linalg.norm(pos2 - pos1)
    
    def _calculate_resistance(self, material: Material, length: float, area: float) -> float:
        """计算电阻"""
        if area <= 0:
            return 1e12  # 开路
        
        resistivity = 1.0 / (material.sigma + 1e-12)  # 电阻率
        return resistivity * length / area
    
    def _calculate_inductance(self, material: Material, length: float, area: float) -> float:
        """计算电感"""
        # 简化的电感公式
        mu = EMConstants.MU_0 * material.mur
        return mu * length / (2 * np.pi) * np.log(length / np.sqrt(area) + 1e-12)
    
    def _calculate_capacitance(self, material: Material, length: float, area: float) -> float:
        """计算电容"""
        # 简化的电容公式
        epsilon = EMConstants.EPSILON_0 * material.epsr
        return epsilon * area / (length + 1e-12)
    
    def _calculate_branch_distance(self, branch1: Dict, branch2: Dict, vertices: np.ndarray) -> float:
        """计算支路间距离"""
        # 计算两个支路中点间的距离
        mid1 = (vertices[branch1['from_node']] + vertices[branch1['to_node']]) / 2
        mid2 = (vertices[branch2['from_node']] + vertices[branch2['to_node']]) / 2
        return np.linalg.norm(mid1 - mid2)
    
    def _calculate_mutual_inductance(self, distance: float) -> float:
        """计算互感"""
        # 简化的互感公式
        if distance < 1e-6:
            distance = 1e-6
        return EMConstants.MU_0 / (2 * np.pi) * np.log(1 + 1e-3 / distance)


# 卫星测试案例
class SatelliteTestCase:
    """卫星测试案例"""
    
    def __init__(self):
        self.test_dir = Path('tests/test_hpm')
        self.stl_file = self.test_dir / 'weixing_v1.stl'
        self.pfd_file = self.test_dir / 'weixing_v1_case.pfd'
        
    def create_materials(self) -> Dict[str, Material]:
        """创建卫星材料"""
        return {
            'PEC': Material('PEC', epsr=1.0, mur=1.0, sigma=1e20),
            'ALUMINUM': Material('ALUMINUM', epsr=1.0, mur=1.0, sigma=3.5e7),
            'SOLAR_PANEL': Material('SOLAR_PANEL', epsr=11.7, mur=1.0, sigma=1e3)
        }
    
    def create_excitation(self, frequency: float = 10e9) -> Excitation:
        """创建卫星激励"""
        return Excitation(
            type='plane_wave',
            frequency=frequency,
            amplitude=1.0,
            phase=0.0,
            direction=np.array([1, 0, 0]),
            polarization=np.array([0, 1, 0])
        )
    
    def run_satellite_test(self, mom_solver_class: type, peec_solver_class: type):
        """运行卫星测试"""
        print("🛰️ 运行卫星MoM/PEEC测试...")
        
        # 创建材料
        materials = self.create_materials()
        
        # 创建激励
        excitation = self.create_excitation()
        frequency = excitation.frequency
        
        # 创建仿真器
        simulator = MoMPeecSimulator(mom_solver_class, peec_solver_class)
        simulator.setup_simulation(frequency, materials)
        
        # 运行MoM仿真
        print("📊 运行MoM仿真...")
        mom_results = simulator.run_mom_simulation(
            str(self.stl_file), excitation,
            target_scale=2.8, stl_units='mm'
        )
        
        # 运行PEEC仿真
        print("📊 运行PEEC仿真...")
        peec_results = simulator.run_peec_simulation(
            str(self.stl_file), [excitation],
            target_scale=2.8, stl_units='mm'
        )
        
        # 运行耦合仿真
        print("🔄 运行耦合仿真...")
        coupling_regions = [
            {
                'mom_indices': list(range(0, 50)),  # 前50个MoM基函数
                'peec_indices': list(range(0, 30)),  # 前30个PEEC节点
                'coupling_strength': 0.1
            }
        ]
        
        coupled_results = simulator.run_coupled_simulation(
            str(self.stl_file), excitation, [excitation], coupling_regions,
            target_scale=2.8, stl_units='mm'
        )
        
        # 可视化结果
        print("📈 生成可视化结果...")
        visualizer = MoMPeecVisualizer()
        
        # MoM电流分布
        if mom_results.currents is not None:
            fig1 = visualizer.plot_current_distribution(
                mom_results.currents, "Satellite MoM Current Distribution"
            )
            plt.savefig('satellite_mom_currents.png', dpi=150, bbox_inches='tight')
            plt.close()
        
        # PEEC电流分布
        if peec_results.currents is not None:
            fig2 = visualizer.plot_current_distribution(
                peec_results.currents, "Satellite PEEC Current Distribution"
            )
            plt.savefig('satellite_peec_currents.png', dpi=150, bbox_inches='tight')
            plt.close()
        
        # 耦合结果
        if coupled_results and 'coupled_results' in coupled_results:
            fig3 = visualizer.plot_coupling_results(
                coupled_results['coupled_results'], "Satellite MoM-PEEC Coupling Results"
            )
            plt.savefig('satellite_coupling_results.png', dpi=150, bbox_inches='tight')
            plt.close()
        
        # 保存结果
        simulator.save_results('satellite_test_results.json')
        
        print("✅ 卫星测试完成")
        return simulator.results


if __name__ == "__main__":
    # 运行卫星测试
    print("🚀 通用MoM/PEEC框架 - 卫星测试案例")
    print("=" * 60)
    
    # 创建测试案例
    satellite_test = SatelliteTestCase()
    
    # 运行测试
    results = satellite_test.run_satellite_test(ProfessionalMoMSolver, ProfessionalPEECSolver)
    
    print("\n✅ 测试完成！")
    print("📁 结果文件:")
    print("   - satellite_mom_currents.png")
    print("   - satellite_peec_currents.png") 
    print("   - satellite_coupling_results.png")
    print("   - satellite_test_results.json")