#!/usr/bin/env python3
"""
Time Domain FDTD Results Visualization Tool
For displaying point, plane, volume time domain simulation results
"""

import numpy as np
import json
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from mpl_toolkits.mplot3d import Axes3D
import os
from pathlib import Path

try:
    import h5py
    HDF5_AVAILABLE = True
except ImportError:
    HDF5_AVAILABLE = False
    print("警告: HDF5库未安装，部分功能受限")

class TimeDomainVisualizer:
    """时域结果可视化器"""
    
    def __init__(self, results_dir="output_time_domain"):
        self.results_dir = results_dir
        self.time_data = None
        self.results = {}
        self.config = {}
        
    def load_results(self):
        """加载时域仿真结果"""
        print("📊 Loading time domain simulation results...")
        
        # 加载配置文件
        report_file = f"{self.results_dir}/time_domain_report.json"
        if os.path.exists(report_file):
            with open(report_file, 'r') as f:
                self.config = json.load(f)
                print(f"  Simulation config: {self.config['simulation_config']['frequency_GHz']} GHz, "
                      f"{self.config['simulation_config']['total_time_ns']} ns")
        
        # 加载HDF5数据
        h5_file = f"{self.results_dir}/time_domain_results.h5"
        if HDF5_AVAILABLE and os.path.exists(h5_file):
            with h5py.File(h5_file, 'r') as f:
                self.time_data = f['time'][:]
                
                # 加载所有数据集
                def load_group(name, obj):
                    if isinstance(obj, h5py.Dataset) and isinstance(obj[()], np.ndarray):
                        # 只加载数组数据，跳过标量
                        if obj.shape != ():
                            self.results[name] = obj[:]
                
                f.visititems(load_group)
                
            print(f"  Loaded data: {len(self.results)} datasets, "
                  f"time steps: {len(self.time_data)}")
        else:
            print("  HDF5 file not available")
            return False
            
        return True
    
    def visualize_point_monitors(self):
        """可视化点监测结果"""
        print("\n📈 Visualizing point monitoring results...")
        
        # 找到所有点监测数据
        point_data = {}
        for key, data in self.results.items():
            if 'point' in key and len(data.shape) == 2 and data.shape[1] == 1:  # [time, 1 point]
                point_data[key] = data
        
        if not point_data:
            print("  No point monitoring data found")
            return
        
        # 创建子图
        n_points = len(set([k.split('_')[1] for k in point_data.keys()]))
        fig, axes = plt.subplots(n_points, 3, figsize=(15, 4*n_points))
        if n_points == 1:
            axes = axes.reshape(1, -1)
        
        time_ns = self.time_data * 1e9
        
        for i, point_idx in enumerate(sorted(set([k.split('_')[1] for k in point_data.keys()]))):
            for j, component in enumerate(['Ex', 'Ey', 'Ez']):
                key = f'point_{point_idx}_{component}'
                if key in point_data:
                    ax = axes[i, j]
                    
                    # 绘制时域波形
                    field_data = point_data[key][:, 0]  # 第一个点的数据
                    ax.plot(time_ns, field_data, 'b-', linewidth=1, alpha=0.8)
                    
                    # 计算包络（使用Hilbert变换）
                    try:
                        from scipy.signal import hilbert
                        analytic_signal = hilbert(field_data)
                        amplitude_envelope = np.abs(analytic_signal)
                        ax.plot(time_ns, amplitude_envelope, 'r--', alpha=0.6, label='包络')
                        ax.legend()
                    except ImportError:
                        pass
                    
                    ax.set_xlabel('Time (ns)')
                    ax.set_ylabel(f'{component} (V/m)')
                    ax.set_title(f'Point {int(point_idx)+1} - {component}')
                    ax.grid(True, alpha=0.3)
                    
                    # 添加统计信息
                    max_val = np.max(np.abs(field_data))
                    ax.text(0.02, 0.98, f'Peak: {max_val:.3f} V/m', 
                           transform=ax.transAxes, verticalalignment='top',
                           bbox=dict(boxstyle='round', facecolor='lightblue', alpha=0.8))
        
        plt.tight_layout()
        plt.savefig(f"{self.results_dir}/point_monitors_time_domain.png", dpi=300, bbox_inches='tight')
        plt.show(block=False)  # Non-blocking show
        plt.pause(0.1)  # Brief pause to allow rendering
        
        print(f"  Point monitoring time domain waveforms saved: point_monitors_time_domain.png")
    
    def visualize_plane_snapshots(self, time_indices=None):
        """可视化平面场的快照"""
        print("\n🌐 Visualizing plane field snapshots...")
        
        # 找到平面数据
        plane_data = {}
        for key, data in self.results.items():
            if 'plane' in key and len(data.shape) == 3:  # [time, y, x] or [time, z, x]
                plane_data[key] = data
        
        if not plane_data:
            print("  No plane monitoring data found")
            return
        
        # 选择时间快照
        if time_indices is None:
            # 选择几个关键时间点 - 限制为5个
            time_indices = [0, 25, 50, 75, 103]  # 手动设置合理的索引
        
        # 为每个平面创建快照
        for plane_key in plane_data.keys():
            data = plane_data[plane_key]
            component = plane_key.split('_')[2]  # Ex, Ey, Ez
            
            # 选择时间快照 - 限制为5个
            time_indices = [0, 25, 50, 75, 103]
            
            fig, axes = plt.subplots(2, 3, figsize=(18, 10))
            axes = axes.flatten()
            
            for i, t_idx in enumerate(time_indices):
                if t_idx >= data.shape[0]:  # 检查数据的时间维度
                    continue
                    
                ax = axes[i]
                
                # 获取当前时间的场分布
                field_snapshot = data[t_idx, :, :]
                
                # 创建彩色图
                im = ax.imshow(field_snapshot.T, 
                              extent=[-1.5, 1.5, -0.55, 0.55],  # 根据坐标范围
                              aspect='auto', 
                              cmap='RdBu_r',
                              vmin=-np.max(np.abs(field_snapshot)),
                              vmax=np.max(np.abs(field_snapshot)))
                
                ax.set_xlabel('Position (m)')
                ax.set_ylabel('Position (m)')
                ax.set_title(f'{plane_key} - t={self.time_data[t_idx]*1e9:.2f} ns')
                
                # 添加颜色条
                cbar = plt.colorbar(im, ax=ax)
                cbar.set_label(f'{component} (V/m)')
                
                # 添加网格
                ax.grid(True, alpha=0.3)
            
            # 移除未使用的子图
            for i in range(len(time_indices), len(axes)):
                fig.delaxes(axes[i])
            
            plt.tight_layout()
            plt.savefig(f"{self.results_dir}/plane_{plane_key.replace('/', '_')}_snapshots.png", dpi=300, bbox_inches='tight')
            plt.show()
            
            print(f"  Plane {plane_key} snapshots saved")
    
    def create_plane_animation(self, plane_key='plane_x0_Ez', fps=10):
        """创建平面场的动画"""
        print(f"\n🎬 Creating {plane_key} animation...")
        
        if plane_key not in self.results:
            print(f"  {plane_key} data not found")
            return
        
        data = self.results[plane_key]
        n_frames = min(len(self.time_data), data.shape[0])
        
        fig, ax = plt.subplots(figsize=(10, 6))
        
        # 初始帧
        im = ax.imshow(data[0, :, :].T, 
                      extent=[-1.5, 1.5, -0.55, 0.55],
                      aspect='auto', 
                      cmap='RdBu_r',
                      vmin=-np.max(np.abs(data)),
                      vmax=np.max(np.abs(data)))
        
        ax.set_xlabel('Position (m)')
        ax.set_ylabel('Position (m)')
        ax.set_title(f'{plane_key} - Time: 0.00 ns')
        
        cbar = plt.colorbar(im, ax=ax)
        component = plane_key.split('_')[2]
        cbar.set_label(f'{component} (V/m)')
        
        def animate(frame):
            im.set_array(data[frame, :, :].T)
            ax.set_title(f'{plane_key} - Time: {self.time_data[frame]*1e9:.2f} ns')
            return [im]
        
        anim = animation.FuncAnimation(fig, animate, frames=n_frames, 
                                     interval=1000//fps, blit=True)
        
        # 保存动画
        anim_file = f"{self.results_dir}/{plane_key}_animation.gif"
        anim.save(anim_file, writer='pillow', fps=fps)
        plt.show()
        
        print(f"  Animation saved: {plane_key}_animation.gif")
    
    def visualize_volume_isosurfaces(self, time_step=None, isovalues=None):
        """可视化体积场的等值面"""
        if time_step is None:
            time_step = len(self.time_data) // 2  # 中间时间点
        
        # 确保时间步在有效范围内
        if time_step >= len(self.time_data):
            time_step = len(self.time_data) - 1
        
        print(f"\n🎯 Visualizing volume field isosurfaces (t={self.time_data[time_step]*1e9:.2f} ns)...")
        
        # 检查时间步范围
        if time_step >= len(self.time_data):
            time_step = len(self.time_data) - 1
            print(f"  Adjusted time step to {time_step} (max available)")
        
        # 找到体积数据
        volume_data = {}
        for key, data in self.results.items():
            if 'volume' in key and len(data.shape) == 4:  # [time, x, y, z]
                volume_data[key] = data
        
        if not volume_data:
            print("  No volume monitoring data found")
            return
        
        # 使用matplotlib的3D可视化
        for key, data in volume_data.items():
            component = key.split('_')[1]  # Ex, Ey, Ez
            
            # 获取指定时间步的数据
            vol_snapshot = data[time_step, :, :, :]
            
            # 创建3D图
            fig = plt.figure(figsize=(12, 8))
            ax = fig.add_subplot(111, projection='3d')
            
            # 创建坐标网格
            nx, ny, nz = vol_snapshot.shape
            x = np.linspace(-1.0, 1.0, nx)
            y = np.linspace(-1.0, 1.0, ny)
            z = np.linspace(-0.3, 0.8, nz)
            
            # 选择等值面值
            if isovalues is None:
                max_val = np.max(np.abs(vol_snapshot))
                isovalues = [-max_val*0.7, -max_val*0.3, 0, max_val*0.3, max_val*0.7]
            
            # 绘制等值面（使用多个切片代替真正的等值面）
            colors = ['red', 'orange', 'green', 'blue', 'purple']
            
            for i, (isoval, color) in enumerate(zip(isovalues, colors)):
                if isoval != 0:
                    # 找到接近等值面的点
                    threshold = abs(isoval)
                    mask = np.abs(vol_snapshot) > threshold
                    
                    # 随机选择一些点来显示
                    points = np.where(mask)
                    if len(points[0]) > 0:
                        # 只显示部分点以避免图形过于密集
                        n_show = min(1000, len(points[0]))
                        indices = np.random.choice(len(points[0]), n_show, replace=False)
                        
                        ax.scatter(points[0][indices], points[1][indices], points[2][indices], 
                                 c=color, s=1, alpha=0.6, 
                                 label=f'{component}={isoval:.3f} V/m')
            
            ax.set_xlabel('X (m)')
            ax.set_ylabel('Y (m)')
            ax.set_zlabel('Z (m)')
            ax.set_title(f'{key} Isosurfaces - t={self.time_data[time_step]*1e9:.2f} ns')
            ax.legend()
            
            plt.tight_layout()
            plt.savefig(f"{self.results_dir}/volume_{key}_isosurfaces_t{time_step}.png", 
                       dpi=300, bbox_inches='tight')
            plt.show()
            
            print(f"  Volume {key} isosurfaces saved")
    
    def generate_field_statistics(self):
        """生成场统计信息"""
        print("\n📈 Generating field statistics...")
        
        stats = {}
        
        for key, data in self.results.items():
            if len(data.shape) >= 2:  # 至少有时间和空间维度
                # 计算统计信息
                max_val = np.max(np.abs(data))
                mean_val = np.mean(data)
                std_val = np.std(data)
                
                # 时间最大值（每个空间点的峰值）
                if len(data.shape) == 2:  # 点监测
                    time_max = np.max(np.abs(data), axis=0)
                elif len(data.shape) == 3:  # 平面监测
                    time_max = np.max(np.abs(data), axis=0)
                elif len(data.shape) == 4:  # 体积监测
                    time_max = np.max(np.abs(data), axis=0)
                
                spatial_max = np.max(time_max)
                spatial_mean = np.mean(time_max)
                
                stats[key] = {
                    'max_amplitude': max_val,
                    'mean_amplitude': abs(mean_val),
                    'std_amplitude': std_val,
                    'spatial_peak_max': spatial_max,
                    'spatial_peak_mean': spatial_mean,
                    'dynamic_range': max_val / (std_val + 1e-12)
                }
        
        # 保存统计信息
        stats_file = f"{self.results_dir}/field_statistics.json"
        with open(stats_file, 'w') as f:
            json.dump(stats, f, indent=2)
        
        # 打印摘要
        print("  Field Statistics Summary:")
        for key, stat in stats.items():
            print(f"    {key}:")
            print(f"      Peak Amplitude: {stat['max_amplitude']:.3f} V/m")
            print(f"      Spatial Average Peak: {stat['spatial_peak_mean']:.3f} V/m")
            print(f"      Dynamic Range: {stat['dynamic_range']:.1f}")
        
        print(f"  Detailed statistics saved: field_statistics.json")
        
        return stats
    
    def create_comprehensive_report(self):
        """创建综合可视化报告"""
        print("\n📊 Creating comprehensive visualization report...")
        
        # 1. 点监测时域波形
        self.visualize_point_monitors()
        
        # 2. 平面场快照
        self.visualize_plane_snapshots()
        
        # 3. 创建动画（选择一个有代表性的平面）
        if any('plane_x0' in key for key in self.results.keys()):
            self.create_plane_animation('plane_x0_Ez')
        
        # 4. 体积场等值面
        self.visualize_volume_isosurfaces()
        
        # 5. 生成统计信息
        self.generate_field_statistics()
        
        print(f"\n✅ Comprehensive visualization report completed!")
        print(f"  All figures saved in: {self.results_dir}/")
        print(f"  Main output files:")
        print(f"    - point_monitors_time_domain.png")
        print(f"    - plane_*_snapshots.png")
        print(f"    - *_animation.gif")
        print(f"    - volume_*_isosurfaces.png")
        print(f"    - field_statistics.json")

def main():
    """主函数"""
    print("🎨 Time Domain FDTD Results Visualization Tool")
    print("=" * 50)
    
    # 创建可视化器
    visualizer = TimeDomainVisualizer()
    
    # 加载结果
    if visualizer.load_results():
        # 创建综合报告
        visualizer.create_comprehensive_report()
    else:
        print("❌ Unable to load result data")

if __name__ == "__main__":
    main()