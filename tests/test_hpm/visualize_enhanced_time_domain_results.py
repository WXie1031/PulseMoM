#!/usr/bin/env python3
"""
Enhanced Time Domain FDTD Results Visualization Tool
Shows satellite structure, material boundaries, and field interactions
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

class EnhancedTimeDomainVisualizer:
    """增强时域结果可视化器 - 显示卫星结构和材料效应"""
    
    def __init__(self, results_dir="output_time_domain_enhanced"):
        self.results_dir = results_dir
        self.time_data = None
        self.results = {}
        self.config = {}
        self.satellite_center = None
        self.satellite_size = None
        
    def load_enhanced_results(self):
        """加载增强的时域仿真结果"""
        print("📊 Loading enhanced time domain simulation results...")
        
        # 加载配置文件
        report_file = f"{self.results_dir}/enhanced_time_domain_report.json"
        if os.path.exists(report_file):
            with open(report_file, 'r') as f:
                self.config = json.load(f)
                print(f"  Enhanced simulation config loaded")
                
                # 提取卫星配置
                sim_config = self.config.get('simulation_config', {})
                self.satellite_center = np.array(sim_config.get('satellite_center', [1.7, 1.7, 0.7]))
                self.satellite_size = np.array(sim_config.get('satellite_size', [2.8, 2.8, 1.0]))
                print(f"  Satellite center: {self.satellite_center}")
                print(f"  Satellite size: {self.satellite_size}")
        
        # 加载HDF5数据
        h5_file = f"{self.results_dir}/enhanced_time_domain_results.h5"
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
                
            print(f"  Loaded enhanced data: {len(self.results)} datasets, "
                  f"time steps: {len(self.time_data)}")
            
            # 加载材料效应信息
            try:
                with h5py.File(h5_file, 'r') as f:
                    if 'material_effects' in f:
                        print("  Material effects data found")
            except:
                pass
        else:
            print("  Enhanced HDF5 file not available")
            return False
            
        return True
    
    def plot_satellite_boundary(self, ax, center=None, size=None, alpha=0.3, color='red', label='Satellite'):
        """绘制卫星边界框"""
        if center is None:
            center = self.satellite_center
        if size is None:
            size = self.satellite_size
            
        # 计算边界
        min_coords = center - size/2
        max_coords = center + size/2
        
        # 绘制边界框
        # X-Y平面 (Z=常数)
        z_levels = [min_coords[2], max_coords[2]]
        for z in z_levels:
            rect = plt.Rectangle((min_coords[0], min_coords[1]), 
                                size[0], size[1], 
                                fill=False, color=color, alpha=alpha, linewidth=2)
            ax.add_patch(rect)
            if z == z_levels[0]:
                ax.text(center[0], center[1], min_coords[2]-0.1, label, 
                       color=color, fontsize=10, ha='center')
        
        return ax
    
    def visualize_enhanced_point_monitors(self):
        """可视化增强的点监测结果 - 显示位置类型"""
        print("\n📈 Visualizing enhanced point monitoring results...")
        
        # 找到所有点监测数据
        point_data = {}
        location_types = {}
        
        for key, data in self.results.items():
            if 'point' in key and len(data.shape) == 2 and data.shape[1] == 1:
                point_data[key] = data
                # 尝试获取位置类型
                if hasattr(self, 'location_info') and key in self.location_info:
                    location_types[key] = self.location_info[key]
        
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
                    ax.plot(time_ns, field_data, 'b-', linewidth=1, alpha=0.8, label='Field')
                    
                    # 计算包络（使用Hilbert变换）
                    try:
                        from scipy.signal import hilbert
                        analytic_signal = hilbert(field_data)
                        amplitude_envelope = np.abs(analytic_signal)
                        ax.plot(time_ns, amplitude_envelope, 'r--', alpha=0.6, label='Envelope')
                        ax.legend()
                    except ImportError:
                        pass
                    
                    # 添加位置信息
                    if key in location_types:
                        loc_type = location_types[key]
                        color_map = {
                            'satellite_interior': 'red',
                            'shadow_region': 'orange', 
                            'free_space': 'green'
                        }
                        color = color_map.get(loc_type, 'black')
                        ax.text(0.02, 0.98, f'Location: {loc_type}', 
                               transform=ax.transAxes, verticalalignment='top',
                               bbox=dict(boxstyle='round', facecolor=color, alpha=0.3),
                               color='white' if color in ['red', 'green'] else 'black')
                    
                    ax.set_xlabel('Time (ns)')
                    ax.set_ylabel(f'{component} (V/m)')
                    ax.set_title(f'Point {int(point_idx)+1} - {component}')
                    ax.grid(True, alpha=0.3)
                    
                    # 添加统计信息
                    max_val = np.max(np.abs(field_data))
                    ax.text(0.02, 0.85, f'Peak: {max_val:.3f} V/m', 
                           transform=ax.transAxes, verticalalignment='top',
                           bbox=dict(boxstyle='round', facecolor='lightblue', alpha=0.8))
        
        plt.tight_layout()
        plt.savefig(f"{self.results_dir}/enhanced_point_monitors_time_domain.png", dpi=300, bbox_inches='tight')
        plt.show(block=False)
        plt.pause(0.1)
        
        print(f"  Enhanced point monitoring waveforms saved: enhanced_point_monitors_time_domain.png")
    
    def visualize_enhanced_plane_snapshots(self, time_indices=None):
        """可视化增强的平面场快照 - 显示卫星结构"""
        print("\n🌐 Visualizing enhanced plane field snapshots...")
        
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
            # 选择几个关键时间点
            time_indices = [0, len(self.time_data)//4, len(self.time_data)//2, 3*len(self.time_data)//4, len(self.time_data)-1]
        
        # 为每个平面创建快照
        for plane_key in plane_data.keys():
            data = plane_data[plane_key]
            component = plane_key.split('_')[3]  # Ex, Ey, Ez
            
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
                              extent=[0, 3.4, 0, 1.4],  # 根据坐标范围
                              aspect='auto', 
                              cmap='RdBu_r',
                              vmin=-np.max(np.abs(field_snapshot)),
                              vmax=np.max(np.abs(field_snapshot)))
                
                # 添加卫星边界
                if 'x_sat' in plane_key:
                    # X=卫星中心平面 - 显示Y-Z截面
                    self.plot_satellite_boundary(ax, center=[self.satellite_center[1], self.satellite_center[2]], 
                                               size=[self.satellite_size[1], self.satellite_size[2]])
                elif 'y_sat' in plane_key:
                    # Y=卫星中心平面 - 显示X-Z截面
                    self.plot_satellite_boundary(ax, center=[self.satellite_center[0], self.satellite_center[2]], 
                                               size=[self.satellite_size[0], self.satellite_size[2]])
                elif 'z_sat' in plane_key:
                    # Z=卫星中心平面 - 显示X-Y截面
                    self.plot_satellite_boundary(ax, center=[self.satellite_center[0], self.satellite_center[1]], 
                                               size=[self.satellite_size[0], self.satellite_size[1]])
                
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
            plt.savefig(f"{self.results_dir}/enhanced_plane_{plane_key.replace('/', '_')}_snapshots.png", dpi=300, bbox_inches='tight')
            plt.show(block=False)
            plt.pause(0.1)
            
            print(f"  Enhanced plane {plane_key} snapshots saved")
    
    def create_material_effect_animation(self, plane_key='plane_x_sat_Ez', fps=10):
        """创建材料效应动画 - 显示卫星结构影响"""
        print(f"\n🎬 Creating enhanced {plane_key} animation with material effects...")
        
        if plane_key not in self.results:
            print(f"  {plane_key} data not found")
            return
        
        data = self.results[plane_key]
        n_frames = min(len(self.time_data), data.shape[0])
        
        fig, ax = plt.subplots(figsize=(10, 6))
        
        # 初始帧
        im = ax.imshow(data[0, :, :].T, 
                      extent=[0, 3.4, 0, 1.4],
                      aspect='auto', 
                      cmap='RdBu_r',
                      vmin=-np.max(np.abs(data)),
                      vmax=np.max(np.abs(data)))
        
        # 添加卫星边界
        if 'x_sat' in plane_key:
            self.plot_satellite_boundary(ax, center=[self.satellite_center[1], self.satellite_center[2]], 
                                       size=[self.satellite_size[1], self.satellite_size[2]])
        
        ax.set_xlabel('Position (m)')
        ax.set_ylabel('Position (m)')
        ax.set_title(f'{plane_key} - Time: 0.00 ns')
        
        cbar = plt.colorbar(im, ax=ax)
        component = plane_key.split('_')[3]
        cbar.set_label(f'{component} (V/m)')
        
        def animate(frame):
            im.set_array(data[frame, :, :].T)
            ax.set_title(f'{plane_key} - Time: {self.time_data[frame]*1e9:.2f} ns')
            return [im]
        
        anim = animation.FuncAnimation(fig, animate, frames=n_frames, 
                                     interval=1000//fps, blit=True)
        
        # 保存动画
        anim_file = f"{self.results_dir}/enhanced_{plane_key}_animation.gif"
        anim.save(anim_file, writer='pillow', fps=fps)
        plt.show(block=False)
        plt.pause(0.1)
        
        print(f"  Enhanced animation saved: enhanced_{plane_key}_animation.gif")
    
    def generate_enhanced_field_statistics(self):
        """生成增强的场统计信息 - 区分不同区域"""
        print("\n📈 Generating enhanced field statistics...")
        
        stats = {}
        region_stats = {
            'satellite_interior': [],
            'shadow_region': [],
            'free_space': []
        }
        
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
        stats_file = f"{self.results_dir}/enhanced_field_statistics.json"
        with open(stats_file, 'w') as f:
            json.dump(stats, f, indent=2)
        
        # 打印摘要
        print("  Enhanced Field Statistics Summary:")
        for key, stat in stats.items():
            print(f"    {key}:")
            print(f"      Peak Amplitude: {stat['max_amplitude']:.3f} V/m")
            print(f"      Spatial Average Peak: {stat['spatial_peak_mean']:.3f} V/m")
            print(f"      Dynamic Range: {stat['dynamic_range']:.1f}")
        
        print(f"  Enhanced statistics saved: enhanced_field_statistics.json")
        
        return stats
    
    def create_comprehensive_enhanced_report(self):
        """创建综合增强可视化报告"""
        print("\n📊 Creating comprehensive enhanced visualization report...")
        
        # 1. 增强点监测时域波形
        self.visualize_enhanced_point_monitors()
        
        # 2. 增强平面场快照（显示卫星结构）
        self.visualize_enhanced_plane_snapshots()
        
        # 3. 创建增强动画（显示材料效应）
        satellite_plane_keys = [k for k in self.results.keys() if 'sat' in k and 'plane' in k]
        if satellite_plane_keys:
            self.create_material_effect_animation(satellite_plane_keys[0])
        
        # 4. 生成增强统计信息
        self.generate_enhanced_field_statistics()
        
        print(f"\n✅ Comprehensive enhanced visualization report completed!")
        print(f"  All enhanced figures saved in: {self.results_dir}/")
        print(f"  Enhanced output files:")
        print(f"    - enhanced_point_monitors_time_domain.png")
        print(f"    - enhanced_plane_*_snapshots.png")
        print(f"    - enhanced_*_animation.gif")
        print(f"    - enhanced_field_statistics.json")

def main():
    """主函数 - 增强版"""
    print("🎨 Enhanced Time Domain FDTD Results Visualization Tool")
    print("=" * 60)
    print("✨ 增强特性:")
    print("  ✅ 卫星结构可视化")
    print("  ✅ 材料边界显示")
    print("  ✅ 区域类型标识")
    print("  ✅ 材料效应动画")
    print("=" * 60)
    
    # 创建增强可视化器
    visualizer = EnhancedTimeDomainVisualizer()
    
    # 加载增强结果
    if visualizer.load_enhanced_results():
        # 创建综合增强报告
        visualizer.create_comprehensive_enhanced_report()
    else:
        print("❌ Unable to load enhanced result data")
        print("  请确保已运行增强仿真脚本: weixing_v1_time_domain_enhanced.py")

if __name__ == "__main__":
    main()