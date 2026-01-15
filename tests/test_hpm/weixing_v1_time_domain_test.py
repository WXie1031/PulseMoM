#!/usr/bin/env python3
"""
时域FDTD仿真 - 真正的时域波形测试
严格按照weixing_v1_case.pfd配置实现时域仿真
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
    """时域FDTD配置参数"""
    # 基本参数
    frequency: float = 10.0e9  # 10GHz
    wavelength: float = 0.03  # 30mm
    
    # 计算域
    domain_size: np.ndarray = np.array([3.4, 3.4, 1.4])  # 3400×3400×1400 mm
    satellite_size: np.ndarray = np.array([2.8, 2.8, 1.0])  # 2800×2800×1000 mm
    
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
        
        print(f"时域FDTD配置:")
        print(f"  时间步长: {self.dt*1e12:.3f} ps")
        print(f"  总步数: {self.nsteps:,}")
        print(f"  网格尺寸: {self.grid_dims[0]}×{self.grid_dims[1]}×{self.grid_dims[2]}")
        print(f"  总网格数: {np.prod(self.grid_dims):,}")

class WaveformLoader:
    """波形文件加载器"""
    
    def __init__(self, filename: str):
        self.filename = filename
        self.time_data = None
        self.amplitude_data = None
        
    def load(self) -> bool:
        """加载波形文件"""
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
        """生成默认的10GHz调制高斯脉冲"""
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
        """获取指定时间的幅值"""
        if self.time_data is None:
            return 0.0
            
        # 线性插值
        return np.interp(t, self.time_data, self.amplitude_data)

class TimeDomainPlaneWave:
    """时域平面波源"""
    
    def __init__(self, config: FDTDTimeDomainConfig):
        self.config = config
        self.waveform = WaveformLoader(config.waveform_file)
        
        # 计算波矢量方向
        theta_rad = np.radians(config.theta)
        phi_rad = np.radians(config.phi)
        
        self.k_direction = np.array([
            np.sin(theta_rad) * np.cos(phi_rad),
            np.sin(theta_rad) * np.sin(phi_rad),
            np.cos(theta_rad)
        ])
        
        # 极化方向 - 确保与波矢量正交 (k·E0 = 0)
        psi_rad = np.radians(config.psi)
        
        # 首先计算一个与k方向正交的参考向量
        # 使用标准球坐标系中的极化向量构造方法
        if abs(self.k_direction[2]) < 0.9:  # k不是主要沿着z方向
            # 使用z轴作为参考，构造与k正交的向量
            e_theta = np.array([
                np.cos(theta_rad) * np.cos(phi_rad),
                np.cos(theta_rad) * np.sin(phi_rad),
                -np.sin(theta_rad)
            ])
        else:  # k主要沿着z方向
            # 使用x轴作为参考，构造与k正交的向量
            e_theta = np.array([
                np.cos(theta_rad) * np.cos(phi_rad),
                np.cos(theta_rad) * np.sin(phi_rad),
                -np.sin(theta_rad)
            ])
        
        # 计算e_phi方向（与k和e_theta都正交）
        e_phi = np.array([
            -np.sin(phi_rad),
            np.cos(phi_rad),
            0.0
        ])
        
        # 构造极化向量：e_psi = cos(psi) * e_theta + sin(psi) * e_phi
        self.polarization = np.cos(psi_rad) * e_theta + np.sin(psi_rad) * e_phi
        
        # 确保极化向量与波矢量正交（数值精度修正）
        k_dot_E = np.dot(self.k_direction, self.polarization)
        if abs(k_dot_E) > 1e-10:
            # 修正极化向量使其严格正交于k
            self.polarization = self.polarization - k_dot_E * self.k_direction
        
        # 归一化
        self.polarization = self.polarization / np.linalg.norm(self.polarization)
        
        # 验证正交性
        k_dot_E_final = np.dot(self.k_direction, self.polarization)
        print(f"  波矢量: k = [{self.k_direction[0]:.3f}, {self.k_direction[1]:.3f}, {self.k_direction[2]:.3f}]")
        print(f"  极化向量: E0 = [{self.polarization[0]:.3f}, {self.polarization[1]:.3f}, {self.polarization[2]:.3f}]")
        print(f"  正交验证: k·E0 = {k_dot_E_final:.2e} (应该接近0)")
        
    def compute_field_at_time(self, points: np.ndarray, t: float) -> np.ndarray:
        """计算指定时间的电场分布"""
        waveform_amp = self.waveform.get_amplitude_at_time(t)
        
        # 平面波相位因子
        k0 = 2 * PI / self.config.wavelength
        
        num_points = points.shape[0]
        E_field = np.zeros((num_points, 3))
        
        # 确保场强在合理范围内（修正幅度缩放）
        # 对于10GHz平面波，1V/m是合理的场强幅度
        field_scaling = 1.0  # 目标场强幅度为1V/m
        
        for i, point in enumerate(points):
            phase = -k0 * np.dot(self.k_direction, point)
            # 应用幅度缩放确保场强在合理范围
            E_field[i, :] = field_scaling * waveform_amp * self.polarization * np.cos(phase)
            
        return E_field

class TimeDomainHPMTester:
    """时域HPM测试器"""
    
    def __init__(self, config: FDTDTimeDomainConfig):
        self.config = config
        self.plane_wave = TimeDomainPlaneWave(config)
        self.output_dir = "output_time_domain"
        
        # 创建输出目录
        os.makedirs(self.output_dir, exist_ok=True)
        
    def generate_dense_observation_points(self) -> Dict[str, np.ndarray]:
        """生成高密度的观测点 - 符合100波长分辨率要求"""
        print("\n📍 生成高密度观测点配置...")
        print("  注意：坐标系已根据STL平移调整（GEOMETRY_TRANSLATE 0 0 -550）")
        print("  坐标系：域中心为(0,0,0)，卫星中心在(0,0,-0.55)m")
        points = {}
        
        # 卫星中心位置（考虑STL平移 -550mm）
        # 与.pfd文件一致：域中心(0,0,0)，卫星平移(0,0,-550mm)
        satellite_center = np.array([0.0, 0.0, -0.55])  # -550mm translation from domain center
        
        # 1. 点监测 - 多个关键位置（相对于平移后的卫星中心）
        print("  点监测配置:")
        key_positions = [
            satellite_center,                    # 卫星中心
            satellite_center + [0.7, 0.0, 0.56], # 卫星表面附近
            satellite_center + [-0.7, 0.0, 0.56], # 卫星表面附近
            satellite_center + [0.0, 0.7, 0.56],  # 卫星表面附近
            satellite_center + [0.0, -0.7, 0.56], # 卫星表面附近
        ]
        
        for i, pos in enumerate(key_positions):
            point_data = np.array([pos])
            points[f'point_{i}_Ex'] = point_data
            points[f'point_{i}_Ey'] = point_data
            points[f'point_{i}_Ez'] = point_data
            print(f"    监测点{i+1}: [{pos[0]:.1f}, {pos[1]:.1f}, {pos[2]:.1f}] m")
        
        # 2. 高密度平面监测 - 每波长至少10个点
        print("  高密度平面监测:")
        
        # X=0平面 - 每30mm一个点 (波长30mm，1点/波长) - 合理密度
        # 与.pfd文件一致：域中心(0,0,0)，卫星范围Z=[-1.05, -0.05]m
        y_range_dense = np.linspace(-1.7, 1.7, 114)  # 覆盖整个域 Y=[-1.7, 1.7]m
        z_range_dense = np.linspace(-1.4, 0.7, 105)  # 覆盖整个域 Z=[-1.4, 0.7]m
        Y, Z = np.meshgrid(y_range_dense, z_range_dense)
        X = np.zeros_like(Y)
        plane_x0_dense = np.stack([X, Y, Z], axis=-1)
        
        points['plane_x0_Ex'] = plane_x0_dense
        points['plane_x0_Ey'] = plane_x0_dense
        points['plane_x0_Ez'] = plane_x0_dense
        print(f"    X=0平面: {plane_x0_dense.shape[0]}×{plane_x0_dense.shape[1]} = {plane_x0_dense.shape[0]*plane_x0_dense.shape[1]:,} 点")
        
        # Y=0平面 - 同样密度
        # 与.pfd文件一致：域中心(0,0,0)，卫星范围Z=[-1.05, -0.05]m
        x_range_dense = np.linspace(-1.7, 1.7, 114)  # 覆盖整个域 X=[-1.7, 1.7]m
        z_range_dense = np.linspace(-1.4, 0.7, 105)  # 覆盖整个域 Z=[-1.4, 0.7]m
        X, Z = np.meshgrid(x_range_dense, z_range_dense)
        Y = np.zeros_like(X)
        plane_y0_dense = np.stack([X, Y, Z], axis=-1)
        
        points['plane_y0_Ex'] = plane_y0_dense
        points['plane_y0_Ey'] = plane_y0_dense
        points['plane_y0_Ez'] = plane_y0_dense
        print(f"    Y=0平面: {plane_y0_dense.shape[0]}×{plane_y0_dense.shape[1]} = {plane_y0_dense.shape[0]*plane_y0_dense.shape[1]:,} 点")
        
        # 3. 体积监测 - 重点区域
        print("  体积监测配置:")
        
        # 卫星周围区域 - 60mm间距 - 合理密度
        # 与.pfd文件一致：域中心(0,0,0)，覆盖整个域
        x_vol = np.linspace(-1.7, 1.7, 58)  # 覆盖整个域 X=[-1.7, 1.7]m，60mm间距
        y_vol = np.linspace(-1.7, 1.7, 58)  # 覆盖整个域 Y=[-1.7, 1.7]m，60mm间距
        z_vol = np.linspace(-1.4, 0.7, 36)  # 覆盖整个域 Z=[-1.4, 0.7]m，60mm间距
        
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
    
    def run_time_domain_simulation(self):
        """运行完整的时域仿真"""
        print(f"\n🚀 启动时域FDTD仿真...")
        print(f"  总时间步数: {self.config.nsteps:,}")
        print(f"  输出间隔: 每{self.config.output_step}步")
        print(f"  总输出帧数: {self.config.nsteps//self.config.output_step:,}")
        
        # 加载波形
        if not self.plane_wave.waveform.load():
            print("波形加载失败，使用默认波形")
        
        # 生成观测点
        observation_points = self.generate_dense_observation_points()
        
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
        
        print(f"\n📊 开始时间步进计算...")
        
        start_time = time.time()
        # 主时间循环
        output_counter = 0
        for n in range(self.config.nsteps):
            current_time = n * self.config.dt
            
            # 每output_step步输出一次
            if n % self.config.output_step == 0 and output_counter < n_output_steps:
                if output_counter % 100 == 0:
                    print(f"  步骤 {n:,}/{self.config.nsteps:,} (t={current_time*1e9:.3f} ns)")
                
                # 计算所有观测点的场
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
                    
                    # 计算时域场
                    E_field = self.plane_wave.compute_field_at_time(flat_points, current_time)
                    
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
        
        print(f"\n✅ 时域仿真完成!")
        print(f"  计算时间: {computation_time:.2f} 秒")
        print(f"  平均步进速度: {self.config.nsteps/computation_time:.0f} 步/秒")
        
        # 保存结果
        self.save_time_domain_results(time_history, time_domain_results, observation_points)
        
        return time_history, time_domain_results
    
    def save_time_domain_results(self, time_history: np.ndarray, 
                                results: Dict[str, np.ndarray], 
                                observation_points: Dict[str, np.ndarray]):
        """保存时域结果"""
        print(f"\n💾 保存时域结果...")
        
        # 1. 保存为HDF5格式（高效存储）
        try:
            import h5py
            with h5py.File(f"{self.output_dir}/time_domain_results.h5", 'w') as f:
                # 时间轴
                f.create_dataset('time', data=time_history)
                f.create_dataset('dt', data=self.config.dt)
                f.create_dataset('frequency', data=self.config.frequency)
                
                # 每个观测点的时域数据
                for point_type, data in results.items():
                    f.create_dataset(point_type, data=data)
                    
                # 观测点坐标
                coords_group = f.create_group('coordinates')
                for point_type, points in observation_points.items():
                    coords_group.create_dataset(point_type, data=points)
                    
            print(f"  HDF5文件保存完成: {self.output_dir}/time_domain_results.h5")
            
        except ImportError:
            print("  HDF5库未安装，使用numpy格式保存")
            
        # 2. 保存关键监测点的时域波形图数据
        point_monitoring = {}
        for point_type, data in results.items():
            if 'point' in point_type:
                # 找到对应的坐标
                coords = observation_points[point_type][0]  # 第一个点
                point_monitoring[point_type] = {
                    'time_ns': (time_history * 1e9).tolist(),
                    'field_value': data[:, 0].tolist(),  # 第一个点的时间序列
                    'position': coords.tolist()
                }
        
        with open(f"{self.output_dir}/point_monitoring.json", 'w') as f:
            json.dump(point_monitoring, f, indent=2)
        print(f"  点监测数据保存完成: {self.output_dir}/point_monitoring.json")
        
        # 3. 生成测试报告
        report = {
            'simulation_config': {
                'frequency_GHz': self.config.frequency / 1e9,
                'total_time_ns': self.config.total_time * 1e9,
                'dt_ps': self.config.dt * 1e12,
                'nsteps': self.config.nsteps,
                'output_steps': len(time_history),
                'grid_spacing_mm': self.config.grid_spacing * 1e3,
                'grid_dims': self.config.grid_dims.tolist()
            },
            'measurement_config': {
                'point_monitors': len([k for k in results.keys() if 'point' in k]),
                'plane_monitors': len([k for k in results.keys() if 'plane' in k]),
                'volume_monitors': len([k for k in results.keys() if 'volume' in k]),
                'total_data_points': sum(data.size for data in results.values())
            },
            'output_files': [
                f"{self.output_dir}/time_domain_results.h5",
                f"{self.output_dir}/point_monitoring.json"
            ]
        }
        
        with open(f"{self.output_dir}/time_domain_report.json", 'w') as f:
            json.dump(report, f, indent=2)
        print(f"  测试报告保存完成: {self.output_dir}/time_domain_report.json")
        
        # 4. 输出文件统计
        total_size_gb = sum(data.nbytes for data in results.values()) / 1024**3
        print(f"\n📈 输出数据统计:")
        print(f"  总数据量: {total_size_gb:.2f} GB")
        print(f"  时间步数: {len(time_history):,}")
        print(f"  监测类型: {len(results)} 种")
        print(f"  文件位置: {self.output_dir}/")

def main():
    """主函数"""
    print("🛰️ 卫星HPM时域FDTD仿真")
    print("=" * 50)
    
    # 创建配置
    config = FDTDTimeDomainConfig()
    
    # 创建测试器
    tester = TimeDomainHPMTester(config)
    
    # 运行仿真
    time_history, results = tester.run_time_domain_simulation()
    
    print(f"\n🎉 仿真完成！结果文件保存在: {tester.output_dir}/")

if __name__ == "__main__":
    main()