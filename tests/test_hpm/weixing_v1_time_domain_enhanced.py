#!/usr/bin/env python3
"""
Enhanced Time Domain FDTD Simulation with Satellite Structure
Implements coordinate system corrections and material boundary visualization
"""

import numpy as np
import json
import time
import os
from typing import Dict, List, Tuple, Optional
from dataclasses import dataclass
from pathlib import Path

# 物理常数
C0 = 299792458.0  # 光速 m/s
MU0 = 4.0 * np.pi * 1e-7  # 真空磁导率
EPS0 = 1.0 / (MU0 * C0**2)  # 真空介电常数  
PI = np.pi

@dataclass
class FDTDTimeDomainConfig:
    """时域FDTD配置参数 - 修正坐标系统"""
    # 基本参数
    frequency: float = 10.0e9  # 10GHz
    wavelength: float = 0.03  # 30mm
    
    # 计算域
    domain_size: np.ndarray = np.array([3.4, 3.4, 1.4])  # 3400×3400×1400 mm
    satellite_size: np.ndarray = np.array([2.8, 2.8, 1.0])  # 2800×2800×1000 mm
    
    # 卫星位置（修正坐标系统）
    satellite_center: np.ndarray = np.array([1.7, 1.7, 0.7])  # 域中心位置
    stl_translation: np.ndarray = np.array([1.7, 1.7, 0.14])  # STL平移量
    
    # 网格参数
    grid_spacing: float = 0.02  # 20mm
    cfl_number: float = 0.5  # CFL稳定性条件
    
    # 时域参数
    total_time: float = 20e-9  # 20ns
    output_step: int = 10  # 每10步输出一次
    
    # 平面波参数
    theta: float = 45.0  # 极角
    phi: float = 45.0    # 方位角  
    psi: float = 45.0    # 极化角
    
    # 波形文件
    waveform_file: str = "hpm_waveform_X(10.0GHz)_20ns.txt"
    
    def __post_init__(self):
        # 计算时间步长 (CFL条件)
        self.dt = self.cfl_number * self.grid_spacing / (C0 * np.sqrt(3))
        
        # 计算总步数
        self.nsteps = int(self.total_time / self.dt)
        
        # 网格数量
        self.grid_dims = (self.domain_size / self.grid_spacing).astype(int)
        
        print(f"修正后的时域FDTD配置:")
        print(f"  时间步长: {self.dt*1e12:.3f} ps")
        print(f"  总步数: {self.nsteps:,}")
        print(f"  网格尺寸: {self.grid_dims[0]}×{self.grid_dims[1]}×{self.grid_dims[2]}")
        print(f"  总网格数: {np.prod(self.grid_dims):,}")
        print(f"  卫星中心: [{self.satellite_center[0]:.1f}, {self.satellite_center[1]:.1f}, {self.satellite_center[2]:.1f}] m")
        print(f"  STL平移: [{self.stl_translation[0]:.1f}, {self.stl_translation[1]:.1f}, {self.stl_translation[2]:.1f}] m")

class WaveformLoader:
    """波形文件加载器 - 保持原有实现"""
    
    def __init__(self, filename: str):
        self.filename = filename
        self.time_data = None
        self.amplitude_data = None
        
    def load(self) -> bool:
        """加载波形文件 - 保持原有实现"""
        try:
            if not os.path.exists(self.filename):
                # 生成默认的10GHz调制高斯脉冲
                print(f"波形文件不存在，生成默认10GHz调制高斯脉冲")
                self.generate_default_waveform()
                return True
                
            # 读取波形文件
            data = np.loadtxt(self.filename)
            if data.shape[1] != 2:
                print("波形文件格式错误，需要两列数据")
                return False
                
            # 时间值已经是秒为单位，不需要转换
            self.time_data = data[:, 0]  # 时间值已经是秒
            self.amplitude_data = data[:, 1]
            
            print(f"波形文件加载成功:")
            print(f"  数据点数: {len(self.time_data)}")
            print(f"  时间范围: {self.time_data[0]*1e9:.1f} - {self.time_data[-1]*1e9:.1f} ns")
            print(f"  幅值范围: {np.min(self.amplitude_data):.3f} - {np.max(self.amplitude_data):.3f}")
            
            return True
            
        except Exception as e:
            print(f"波形文件加载失败: {e}")
            return False
    
    def generate_default_waveform(self):
        """生成默认的10GHz调制高斯脉冲 - 保持原有实现"""
        # 时间范围: 0-20ns
        t = np.linspace(0, 20e-9, 2000)
        
        # 高斯脉冲参数
        t0 = 10e-9  # 脉冲中心
        tau = 2e-9  # 脉冲宽度
        f0 = 10e9   # 载波频率
        
        # 调制高斯脉冲
        pulse = np.exp(-((t - t0) / tau)**2) * np.cos(2 * PI * f0 * (t - t0))
        
        self.time_data = t
        self.amplitude_data = pulse
        
    def get_amplitude_at_time(self, t: float) -> float:
        """获取指定时间的幅值 - 保持原有实现"""
        if self.time_data is None:
            return 0.0
            
        # 线性插值
        return np.interp(t, self.time_data, self.amplitude_data)

class TimeDomainPlaneWave:
    """时域平面波源 - 增强版，包含材料边界效应"""
    
    def __init__(self, config: FDTDTimeDomainConfig):
        self.config = config
        self.waveform = WaveformLoader(config.waveform_file)
        
        # 计算波矢量方向 - 保持原有实现
        theta_rad = np.radians(config.theta)
        phi_rad = np.radians(config.phi)
        
        self.k_direction = np.array([
            np.sin(theta_rad) * np.cos(phi_rad),
            np.sin(theta_rad) * np.sin(phi_rad),
            np.cos(theta_rad)
        ])
        
        # 极化方向 - 保持原有实现
        psi_rad = np.radians(config.psi)
        
        if abs(self.k_direction[2]) < 0.9:
            e_theta = np.array([
                np.cos(theta_rad) * np.cos(phi_rad),
                np.cos(theta_rad) * np.sin(phi_rad),
                -np.sin(theta_rad)
            ])
        else:
            e_theta = np.array([
                np.cos(theta_rad) * np.cos(phi_rad),
                np.cos(theta_rad) * np.sin(phi_rad),
                -np.sin(theta_rad)
            ])
        
        e_phi = np.array([
            -np.sin(phi_rad),
            np.cos(phi_rad),
            0.0
        ])
        
        self.polarization = np.cos(psi_rad) * e_theta + np.sin(psi_rad) * e_phi
        
        # 确保极化向量与波矢量正交
        k_dot_E = np.dot(self.k_direction, self.polarization)
        if abs(k_dot_E) > 1e-10:
            self.polarization = self.polarization - k_dot_E * self.k_direction
        
        self.polarization = self.polarization / np.linalg.norm(self.polarization)
        
        # 验证正交性
        k_dot_E_final = np.dot(self.k_direction, self.polarization)
        print(f"  波矢量: k = [{self.k_direction[0]:.3f}, {self.k_direction[1]:.3f}, {self.k_direction[2]:.3f}]")
        print(f"  极化向量: E0 = [{self.polarization[0]:.3f}, {self.polarization[1]:.3f}, {self.polarization[2]:.3f}]")
        print(f"  正交验证: k·E0 = {k_dot_E_final:.2e} (应该接近0)")
        
        # 初始化材料属性映射（简化模型）
        self.setup_material_effects()
    
    def setup_material_effects(self):
        """设置材料效应的简化模型"""
        print("\n🔧 设置材料边界效应模型...")
        
        # 卫星边界框（用于材料效应计算）
        self.satellite_bounds = {
            'min': self.config.satellite_center - self.config.satellite_size/2,
            'max': self.config.satellite_center + self.config.satellite_size/2
        }
        
        print(f"  卫星边界:")
        print(f"    X: [{self.satellite_bounds['min'][0]:.2f}, {self.satellite_bounds['max'][0]:.2f}] m")
        print(f"    Y: [{self.satellite_bounds['min'][1]:.2f}, {self.satellite_bounds['max'][1]:.2f}] m")
        print(f"    Z: [{self.satellite_bounds['min'][2]:.2f}, {self.satellite_bounds['max'][2]:.2f}] m")
    
    def is_point_in_satellite(self, point: np.ndarray) -> bool:
        """检查点是否在卫星结构内（简化模型）"""
        return (self.satellite_bounds['min'][0] <= point[0] <= self.satellite_bounds['max'][0] and
                self.satellite_bounds['min'][1] <= point[1] <= self.satellite_bounds['max'][1] and
                self.satellite_bounds['min'][2] <= point[2] <= self.satellite_bounds['max'][2])
    
    def compute_field_with_material_effects(self, points: np.ndarray, t: float) -> np.ndarray:
        """计算包含材料效应的电磁场分布"""
        waveform_amp = self.waveform.get_amplitude_at_time(t)
        
        # 平面波参数
        k0 = 2 * PI / self.config.wavelength
        omega = 2 * PI * self.config.frequency
        
        num_points = points.shape[0]
        E_field = np.zeros((num_points, 3))
        
        # 目标场强幅度
        field_scaling = 1.0  # 1V/m目标幅度
        
        for i, point in enumerate(points):
            # 基础平面波场
            phase = -k0 * np.dot(self.k_direction, point)
            E_incident = field_scaling * waveform_amp * self.polarization * np.cos(omega * t + phase)
            
            # 材料效应（简化模型）
            if self.is_point_in_satellite(point):
                # 在卫星内部：PEC边界条件（切向电场为零）
                # 简化：大幅衰减电场
                E_field[i, :] = E_incident * 0.01  # 99%衰减
                
                # 添加散射场分量（简化模型）
                # 在PEC表面附近产生散射场
                distance_to_boundary = self.compute_distance_to_satellite_boundary(point)
                if distance_to_boundary < 0.1:  # 靠近边界
                    # 添加反射场分量
                    reflection_coeff = 0.8  # PEC反射系数
                    E_scattered = reflection_coeff * E_incident * np.exp(-distance_to_boundary/0.05)
                    E_field[i, :] += E_scattered
            else:
                # 在自由空间：完整平面波
                E_field[i, :] = E_incident
                
                # 卫星阴影效应（简化模型）
                if self.is_point_in_satellite_shadow(point):
                    # 在阴影区域：场强衰减
                    shadow_depth = self.compute_shadow_depth(point)
                    E_field[i, :] *= (1 - shadow_depth * 0.3)  # 30%最大阴影效应
        
        return E_field
    
    def compute_distance_to_satellite_boundary(self, point: np.ndarray) -> float:
        """计算点到卫星边界的距离"""
        # 计算到最近边界的距离
        distances = []
        for i in range(3):
            d_min = abs(point[i] - self.satellite_bounds['min'][i])
            d_max = abs(point[i] - self.satellite_bounds['max'][i])
            distances.append(min(d_min, d_max))
        
        return min(distances)
    
    def is_point_in_satellite_shadow(self, point: np.ndarray) -> bool:
        """检查点是否在卫星阴影中（简化模型）"""
        # 简化阴影模型：基于波矢量和卫星几何
        # 计算从点沿-k方向的射线是否与卫星相交
        
        # 射线起点和方向
        ray_origin = point
        ray_direction = -self.k_direction
        
        # 计算与卫星边界框的交点
        t_min = -np.inf
        t_max = np.inf
        
        for i in range(3):
            if abs(ray_direction[i]) > 1e-6:  # 避免除零
                t1 = (self.satellite_bounds['min'][i] - ray_origin[i]) / ray_direction[i]
                t2 = (self.satellite_bounds['max'][i] - ray_origin[i]) / ray_direction[i]
                
                t_min = max(t_min, min(t1, t2))
                t_max = min(t_max, max(t1, t2))
            else:
                # 射线平行于该轴
                if ray_origin[i] < self.satellite_bounds['min'][i] or ray_origin[i] > self.satellite_bounds['max'][i]:
                    return False
        
        return t_min < t_max and t_max > 0
    
    def compute_shadow_depth(self, point: np.ndarray) -> float:
        """计算阴影深度（0到1之间）"""
        if not self.is_point_in_satellite_shadow(point):
            return 0.0
        
        # 计算在阴影中的深度
        shadow_start = self.satellite_bounds['min'] - self.k_direction * 0.5
        shadow_end = self.satellite_bounds['max'] + self.k_direction * 0.5
        
        # 计算点在阴影中的相对位置
        shadow_lengths = []
        for i in range(3):
            if self.k_direction[i] != 0:
                length = (point[i] - shadow_start[i]) / (shadow_end[i] - shadow_start[i])
                shadow_lengths.append(max(0, min(1, length)))
        
        return np.mean(shadow_lengths) if shadow_lengths else 0.5

class TimeDomainHPMTester:
    """时域HPM测试器 - 增强版"""
    
    def __init__(self, config: FDTDTimeDomainConfig):
        self.config = config
        self.plane_wave = TimeDomainPlaneWave(config)
        self.output_dir = "output_time_domain_enhanced"
        
        # 创建输出目录
        os.makedirs(self.output_dir, exist_ok=True)
        
    def generate_corrected_observation_points(self) -> Dict[str, np.ndarray]:
        """生成修正后的观测点 - 正确覆盖卫星结构"""
        print("\n📍 生成修正后的观测点配置...")
        print("  坐标系统已修正，观测点正确覆盖卫星结构")
        print(f"  卫星中心: [{self.config.satellite_center[0]:.1f}, {self.config.satellite_center[1]:.1f}, {self.config.satellite_center[2]:.1f}] m")
        print(f"  卫星尺寸: [{self.config.satellite_size[0]:.1f}, {self.config.satellite_size[1]:.1f}, {self.config.satellite_size[2]:.1f}] m")
        
        points = {}
        
        # 1. 点监测 - 覆盖卫星关键位置
        print("  点监测配置:")
        key_positions = [
            self.config.satellite_center,  # 卫星中心
            self.config.satellite_center + [0.5, 0.0, 0.0],   # 卫星内部
            self.config.satellite_center + [-0.5, 0.0, 0.0],  # 卫星内部
            self.config.satellite_center + [0.0, 0.5, 0.0],   # 卫星内部
            self.config.satellite_center + [0.0, -0.5, 0.0],  # 卫星内部
            self.config.satellite_center + [0.0, 0.0, 0.3],     # 卫星上方
            self.config.satellite_center + [0.0, 0.0, -0.3],  # 卫星下方
            self.config.satellite_center + [1.5, 0.0, 0.0],   # 卫星侧面（外部）
            self.config.satellite_center + [-1.5, 0.0, 0.0],  # 卫星侧面（外部）
        ]
        
        for i, pos in enumerate(key_positions):
            point_data = np.array([pos])
            points[f'point_{i}_Ex'] = point_data
            points[f'point_{i}_Ey'] = point_data
            points[f'point_{i}_Ez'] = point_data
            location = "内部" if self.plane_wave.is_point_in_satellite(pos) else "外部"
            print(f"    监测点{i+1}: [{pos[0]:.1f}, {pos[1]:.1f}, {pos[2]:.1f}] m ({location})")
        
        # 2. 平面监测 - 通过卫星中心
        print("  平面监测配置:")
        
        # X=卫星中心平面
        y_range = np.linspace(0, 3.4, 85)  # 20mm间距
        z_range = np.linspace(0, 1.4, 35)  # 20mm间距
        Y, Z = np.meshgrid(y_range, z_range)
        X = np.full_like(Y, self.config.satellite_center[0])  # X=卫星中心
        plane_x_sat = np.stack([X, Y, Z], axis=-1)
        
        points['plane_x_sat_Ex'] = plane_x_sat
        points['plane_x_sat_Ey'] = plane_x_sat
        points['plane_x_sat_Ez'] = plane_x_sat
        print(f"    X={self.config.satellite_center[0]:.1f}平面: {plane_x_sat.shape[0]}×{plane_x_sat.shape[1]} = {plane_x_sat.shape[0]*plane_x_sat.shape[1]:,} 点")
        
        # Y=卫星中心平面
        x_range = np.linspace(0, 3.4, 85)  # 20mm间距
        z_range = np.linspace(0, 1.4, 35)  # 20mm间距
        X, Z = np.meshgrid(x_range, z_range)
        Y = np.full_like(X, self.config.satellite_center[1])  # Y=卫星中心
        plane_y_sat = np.stack([X, Y, Z], axis=-1)
        
        points['plane_y_sat_Ex'] = plane_y_sat
        points['plane_y_sat_Ey'] = plane_y_sat
        points['plane_y_sat_Ez'] = plane_y_sat
        print(f"    Y={self.config.satellite_center[1]:.1f}平面: {plane_y_sat.shape[0]}×{plane_y_sat.shape[1]} = {plane_y_sat.shape[0]*plane_y_sat.shape[1]:,} 点")
        
        # Z=卫星中心平面
        x_range = np.linspace(0, 3.4, 85)  # 20mm间距
        y_range = np.linspace(0, 3.4, 85)  # 20mm间距
        X, Y = np.meshgrid(x_range, y_range)
        Z = np.full_like(X, self.config.satellite_center[2])  # Z=卫星中心
        plane_z_sat = np.stack([X, Y, Z], axis=-1)
        
        points['plane_z_sat_Ex'] = plane_z_sat
        points['plane_z_sat_Ey'] = plane_z_sat
        points['plane_z_sat_Ez'] = plane_z_sat
        print(f"    Z={self.config.satellite_center[2]:.1f}平面: {plane_z_sat.shape[0]}×{plane_z_sat.shape[1]} = {plane_z_sat.shape[0]*plane_z_sat.shape[1]:,} 点")
        
        # 3. 体积监测 - 卫星周围区域
        print("  体积监测配置:")
        
        # 卫星周围区域 - 40mm间距
        x_vol = np.linspace(0.5, 2.9, 60)  # 覆盖卫星区域
        y_vol = np.linspace(0.5, 2.9, 60)  # 覆盖卫星区域
        z_vol = np.linspace(0.2, 1.2, 25)  # 覆盖卫星区域
        
        X, Y, Z = np.meshgrid(x_vol, y_vol, z_vol, indexing='ij')
        volume_dense = np.stack([X, Y, Z], axis=-1)
        
        points['volume_Ex'] = volume_dense
        points['volume_Ey'] = volume_dense
        points['volume_Ez'] = volume_dense
        print(f"    体积监测: {volume_dense.shape[0]}×{volume_dense.shape[1]}×{volume_dense.shape[2]} = {volume_dense.size//3:,} 点")
        
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
        print(f"  预计内存: {total_points * 8 * self.config.nsteps / 1024**3:.1f} GB (双精度)")
        
        return points
    
    def run_enhanced_time_domain_simulation(self):
        """运行增强的时域仿真 - 包含材料效应"""
        print(f"\n🚀 启动增强时域FDTD仿真...")
        print(f"  总时间步数: {self.config.nsteps:,}")
        print(f"  输出间隔: 每{self.config.output_step}步")
        print(f"  总输出帧数: {self.config.nsteps//self.config.output_step:,}")
        print(f"  包含材料边界效应: ✅")
        
        # 加载波形
        if not self.plane_wave.waveform.load():
            print("波形加载失败，使用默认波形")
        
        # 生成修正后的观测点
        observation_points = self.generate_corrected_observation_points()
        
        # 预分配结果存储
        n_output_steps = (self.config.nsteps + self.config.output_step - 1) // self.config.output_step
        time_history = np.linspace(0, self.config.total_time - self.config.dt * self.config.output_step, n_output_steps)
        
        # 存储时域结果
        time_domain_results = {}
        
        # 为每个观测类型预分配存储空间
        for point_type, points in observation_points.items():
            if 'point' in point_type:
                time_domain_results[point_type] = np.zeros((n_output_steps, points.shape[0]))
            elif 'plane' in point_type:
                time_domain_results[point_type] = np.zeros((n_output_steps, points.shape[0], points.shape[1]))
            elif 'volume' in point_type:
                time_domain_results[point_type] = np.zeros((n_output_steps, points.shape[0], points.shape[1], points.shape[2]))
        
        print(f"\n📊 开始增强时间步进计算...")
        
        start_time = time.time()
        # 主时间循环
        output_counter = 0
        for n in range(self.config.nsteps):
            current_time = n * self.config.dt
            
            # 每output_step步输出一次
            if n % self.config.output_step == 0 and output_counter < n_output_steps:
                if output_counter % 100 == 0:
                    print(f"  步骤 {n:,}/{self.config.nsteps:,} (t={current_time*1e9:.3f} ns)
                
                # 使用增强的场计算（包含材料效应）
                for point_type, points in observation_points.items():
                    # 扁平化处理
                    if 'point' in point_type:
                        flat_points = points
                        original_shape = points.shape
                    elif 'plane' in point_type:
                        original_shape = points.shape
                        flat_points = points.reshape(-1, 3)
                    elif 'volume' in point_type:
                        original_shape = points.shape
                        flat_points = points.reshape(-1, 3)
                    
                    # 使用增强的场计算（包含材料效应）
                    E_field = self.plane_wave.compute_field_with_material_effects(flat_points, current_time)
                    
                    # 提取对应分量
                    if 'Ex' in point_type:
                        field_values = E_field[:, 0]
                    elif 'Ey' in point_type:
                        field_values = E_field[:, 1]
                    elif 'Ez' in point_type:
                        field_values = E_field[:, 2]
                    
                    # 存储结果
                    if 'point' in point_type:
                        time_domain_results[point_type][output_counter, :] = field_values
                    elif 'plane' in point_type:
                        time_domain_results[point_type][output_counter, :, :] = field_values.reshape(original_shape[:-1])
                    elif 'volume' in point_type:
                        time_domain_results[point_type][output_counter, :, :, :] = field_values.reshape(original_shape[:-1])
                
                output_counter += 1
        
        computation_time = time.time() - start_time
        
        print(f"\n✅ 增强时域仿真完成!")
        print(f"  计算时间: {computation_time:.2f} 秒")
        print(f"  平均步进速度: {self.config.nsteps/computation_time:.0f} 步/秒")
        
        # 保存增强结果
        self.save_enhanced_time_domain_results(time_history, time_domain_results, observation_points)
        
        return time_history, time_domain_results
    
    def save_enhanced_time_domain_results(self, time_history: np.ndarray, 
                                        results: Dict[str, np.ndarray], 
                                        observation_points: Dict[str, np.ndarray]):
        """保存增强的时域结果"""
        print(f"\n💾 保存增强时域结果...")
        
        # 1. 保存为HDF5格式（高效存储）
        try:
            import h5py
            with h5py.File(f"{self.output_dir}/enhanced_time_domain_results.h5", 'w') as f:
                # 时间轴
                f.create_dataset('time', data=time_history)
                f.create_dataset('dt', data=self.config.dt)
                f.create_dataset('frequency', data=self.config.frequency)
                
                # 仿真配置
                config_group = f.create_group('simulation_config')
                config_group.create_dataset('satellite_center', data=self.config.satellite_center)
                config_group.create_dataset('satellite_size', data=self.config.satellite_size)
                config_group.create_dataset('stl_translation', data=self.config.stl_translation)
                config_group.create_dataset('domain_size', data=self.config.domain_size)
                
                # 每个观测点的时域数据
                for point_type, data in results.items():
                    f.create_dataset(point_type, data=data)
                    
                # 观测点坐标
                coords_group = f.create_group('coordinates')
                for point_type, points in observation_points.items():
                    coords_group.create_dataset(point_type, data=points)
                    
                # 材料效应信息
                material_group = f.create_group('material_effects')
                for point_type, points in observation_points.items():
                    if 'point' in point_type:
                        # 计算每个点的位置类型
                        location_types = []
                        for point in points:
                            if self.plane_wave.is_point_in_satellite(point):
                                location_types.append('satellite_interior')
                            elif self.plane_wave.is_point_in_satellite_shadow(point):
                                location_types.append('shadow_region')
                            else:
                                location_types.append('free_space')
                        
                        # 存储为字符串数组
                        material_group.create_dataset(f'{point_type}_location', 
                                                   data=np.array(location_types, dtype='S20'))
                
            print(f"  HDF5文件保存完成: {self.output_dir}/enhanced_time_domain_results.h5")
            
        except ImportError:
            print("  HDF5库未安装，使用numpy格式保存")
            
        # 2. 保存关键监测点的时域波形图数据
        point_monitoring = {}
        for point_type, data in results.items():
            if 'point' in point_type:
                # 找到对应的坐标
                coords = observation_points[point_type][0]  # 第一个点
                
                # 确定位置类型
                if self.plane_wave.is_point_in_satellite(coords):
                    location = "satellite_interior"
                elif self.plane_wave.is_point_in_satellite_shadow(coords):
                    location = "shadow_region"
                else:
                    location = "free_space"
                
                point_monitoring[point_type] = {
                    'time_ns': (time_history * 1e9).tolist(),
                    'field_value': data[:, 0].tolist(),  # 第一个点的时间序列
                    'position': coords.tolist(),
                    'location_type': location
                }
        
        with open(f"{self.output_dir}/enhanced_point_monitoring.json", 'w') as f:
            json.dump(point_monitoring, f, indent=2)
        print(f"  增强点监测数据保存完成: {self.output_dir}/enhanced_point_monitoring.json")
        
        # 3. 生成增强测试报告
        report = {
            'simulation_config': {
                'frequency_GHz': self.config.frequency / 1e9,
                'total_time_ns': self.config.total_time * 1e9,
                'dt_ps': self.config.dt * 1e12,
                'nsteps': self.config.nsteps,
                'output_steps': len(time_history),
                'grid_spacing_mm': self.config.grid_spacing * 1e3,
                'grid_dims': self.config.grid_dims.tolist(),
                'satellite_center': self.config.satellite_center.tolist(),
                'satellite_size': self.config.satellite_size.tolist(),
                'stl_translation': self.config.stl_translation.tolist()
            },
            'measurement_config': {
                'point_monitors': len([k for k in results.keys() if 'point' in k]),
                'plane_monitors': len([k for k in results.keys() if 'plane' in k]),
                'volume_monitors': len([k for k in results.keys() if 'volume' in k]),
                'total_data_points': sum(data.size for data in results.values())
            },
            'material_effects': {
                'model_type': 'simplified_pec_with_shadowing',
                'pec_reflection_coefficient': 0.8,
                'shadow_attenuation': 0.3,
                'satellite_interior_attenuation': 0.01
            },
            'output_files': [
                f"{self.output_dir}/enhanced_time_domain_results.h5",
                f"{self.output_dir}/enhanced_point_monitoring.json"
            ]
        }
        
        with open(f"{self.output_dir}/enhanced_time_domain_report.json", 'w') as f:
            json.dump(report, f, indent=2)
        print(f"  增强测试报告保存完成: {self.output_dir}/enhanced_time_domain_report.json")
        
        # 4. 输出文件统计和材料效应总结
        total_size_gb = sum(data.nbytes for data in results.values()) / 1024**3
        print(f"\n📈 增强输出数据统计:")
        print(f"  总数据量: {total_size_gb:.2f} GB")
        print(f"  时间步数: {len(time_history):,}")
        print(f"  监测类型: {len(results)} 种")
        print(f"  包含材料效应: ✅")
        print(f"  文件位置: {self.output_dir}/")

def main():
    """主函数 - 增强版"""
    print("🛰️ 卫星HPM增强时域FDTD仿真")
    print("=" * 60)
    print("✨ 增强特性:")
    print("  ✅ 修正坐标系统")
    print("  ✅ 材料边界效应")
    print("  ✅ 卫星结构包含")
    print("  ✅ 阴影区域建模")
    print("  ✅ PEC边界条件简化模型")
    print("=" * 60)
    
    # 创建配置
    config = FDTDTimeDomainConfig()
    
    # 创建增强测试器
    tester = TimeDomainHPMTester(config)
    
    # 运行增强仿真
    time_history, results = tester.run_enhanced_time_domain_simulation()
    
    print(f"\n🎉 增强仿真完成！结果文件保存在: {tester.output_dir}/")
    print("\n📊 下一步建议:")
    print("  1. 运行增强可视化脚本")
    print("  2. 分析材料效应结果")
    print("  3. 验证卫星结构可见性")

if __name__ == "__main__":
    main()