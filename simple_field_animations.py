#!/usr/bin/env python3
"""
简化电磁场动画生成器
Simplified Electromagnetic Field Animation Generator

处理现有的仿真数据格式并生成动画
Handles existing simulation data format and generates animations
"""

import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from matplotlib.animation import PillowWriter
import json
import os

class SimpleFieldAnimationGenerator:
    """简化电磁场动画生成器"""
    
    def __init__(self, data_file="satellite_mom_peec_simulation_data.json"):
        self.data_file = data_file
        self.simulation_data = None
        self.load_simulation_data()
        
    def load_simulation_data(self):
        """加载仿真数据"""
        try:
            with open(self.data_file, 'r', encoding='utf-8') as f:
                self.simulation_data = json.load(f)
            print(f"✅ 成功加载仿真数据: {self.data_file}")
            
            # 转换复数字段数据
            self.convert_complex_fields()
            
        except Exception as e:
            print(f"❌ 加载仿真数据失败: {e}")
            self.create_synthetic_data()
    
    def convert_complex_fields(self):
        """转换复数字段数据"""
        for field_type in ['incident_fields', 'scattered_fields', 'total_fields']:
            if field_type in self.simulation_data:
                complex_data = []
                for item in self.simulation_data[field_type]:
                    if isinstance(item, dict) and 'real' in item and 'imag' in item:
                        complex_val = complex(item['real'], item['imag'])
                        complex_data.append(complex_val)
                    else:
                        complex_data.append(complex(0, 0))
                self.simulation_data[field_type] = np.array(complex_data)
                print(f"✅ 转换 {field_type}: {len(complex_data)} 个复数点")
    
    def create_synthetic_data(self):
        """创建合成数据用于动画演示"""
        print("📝 创建合成动画数据...")
        
        # 创建观测点网格
        x = np.linspace(-3, 3, 40)
        y = np.linspace(-3, 3, 40)
        z = np.linspace(-2, 2, 20)
        X, Y, Z = np.meshgrid(x, y, z)
        
        # 创建观测点
        obs_points = np.column_stack([X.flatten(), Y.flatten(), Z.flatten()])
        
        # 参数
        freq = 10e9
        omega = 2 * np.pi * freq
        k = omega / 3e8
        
        # 创建时间序列
        time_steps = 20
        time = np.linspace(0, 2*np.pi/omega, time_steps)
        
        # 初始化场数据
        incident_fields = []
        scattered_fields = []
        total_fields = []
        
        # 卫星中心位置
        satellite_center = np.array([0, 0, -0.55])
        
        for t in time:
            # 入射平面波 (沿z方向传播)
            phase = k * obs_points[:, 2] - omega * t
            incident = np.exp(1j * phase)
            
            # 散射场（卫星周围的散射，简化模型）
            scattered = np.zeros_like(incident, dtype=complex)
            
            # 计算到卫星中心的距离
            distances = np.linalg.norm(obs_points - satellite_center, axis=1)
            
            # 在卫星附近创建散射场
            scatter_mask = distances < 2.0  # 2米范围内
            if np.any(scatter_mask):
                # 简化的散射模型
                r = distances[scatter_mask]
                scattered[scatter_mask] = 0.3 * incident[scatter_mask] * np.exp(-r/0.5) * (1 + 0.1j)
            
            # 总场
            total = incident + scattered
            
            # 存储
            incident_fields.append(incident)
            scattered_fields.append(scattered)
            total_fields.append(total)
        
        # 保存到仿真数据
        self.simulation_data = {
            "frequency": freq,
            "wavelength": 2*np.pi/k,
            "observation_points": obs_points.tolist(),
            "incident_fields": incident_fields,
            "scattered_fields": scattered_fields,
            "total_fields": total_fields,
            "time_steps": time_steps,
            "simulation_time": time.tolist()
        }
        
        print(f"✅ 合成数据创建完成: {time_steps} 个时间步")
    
    def create_field_plane_animation(self, plane='xy', field_type='total', output_file=None):
        """创建场平面动画"""
        
        if output_file is None:
            output_file = f"satellite_{field_type}_field_{plane}_plane_animation.gif"
        
        print(f"🎬 创建 {field_type} 场 {plane.upper()} 平面动画...")
        
        # 获取数据
        obs_points = np.array(self.simulation_data['observation_points'])
        
        if field_type == 'incident':
            field_data = self.simulation_data['incident_fields']
        elif field_type == 'scattered':
            field_data = self.simulation_data['scattered_fields']
        else:
            field_data = self.simulation_data['total_fields']
        
        # 检查数据格式
        if isinstance(field_data, list) and len(field_data) > 0 and isinstance(field_data[0], (list, np.ndarray)):
            # 多时间步数据
            time_steps = len(field_data)
            multi_time = True
        else:
            # 单时间步数据，需要创建时间序列
            time_steps = 20
            multi_time = False
            print(f"⚠️  检测到单时间步数据，将创建合成时间序列动画")
        
        # 创建图形
        fig, ax = plt.subplots(figsize=(10, 8))
        
        # 选择平面
        if plane == 'xy':
            plane_mask = np.abs(obs_points[:, 2]) < 0.2
            x_idx, y_idx = 0, 1
            title = f'{field_type.title()} Field (XY Plane)'
        elif plane == 'xz':
            plane_mask = np.abs(obs_points[:, 1]) < 0.2
            x_idx, y_idx = 0, 2
            title = f'{field_type.title()} Field (XZ Plane)'
        else:  # yz
            plane_mask = np.abs(obs_points[:, 0]) < 0.2
            x_idx, y_idx = 1, 2
            title = f'{field_type.title()} Field (YZ Plane)'
        
        # 获取平面数据
        x_coords = obs_points[plane_mask, x_idx]
        y_coords = obs_points[plane_mask, y_idx]
        
        if len(x_coords) == 0:
            print(f"⚠️  {plane.upper()} 平面没有数据点，选择所有点")
            x_coords = obs_points[:, x_idx]
            y_coords = obs_points[:, y_idx]
            plane_mask = np.ones(len(obs_points), dtype=bool)
        
        # 创建网格
        xi = np.linspace(x_coords.min(), x_coords.max(), 30)
        yi = np.linspace(y_coords.min(), y_coords.max(), 30)
        xi, yi = np.meshgrid(xi, yi)
        
        # 动画更新函数
        def update_frame(frame):
            ax.clear()
            
            if multi_time:
                # 使用实际的多时间步数据
                current_field = np.array(field_data[frame % len(field_data)])
            else:
                # 创建合成时间序列
                if isinstance(field_data, np.ndarray):
                    base_field = field_data
                else:
                    base_field = np.array(field_data)
                
                # 添加时间相位
                phase = 2 * np.pi * frame / time_steps
                current_field = base_field * np.exp(1j * phase)
            
            field_values = np.abs(current_field[plane_mask])
            
            # 插值
            from scipy.interpolate import griddata
            try:
                zi = griddata((x_coords, y_coords), field_values, (xi, yi), method='linear')
                zi = np.nan_to_num(zi, nan=0.0)
                
                # 绘制等高线图
                contour = ax.contourf(xi, yi, zi, levels=15, cmap='jet', alpha=0.8)
                
                # 添加标题和标签
                ax.set_title(f'{title} - Phase {frame + 1}/{time_steps}')
                ax.set_xlabel(f'{plane[0].upper()} (m)')
                ax.set_ylabel(f'{plane[1].upper()} (m)')
                
                # 添加卫星轮廓
                self._add_satellite_outline(ax, plane)
                
                # 添加颜色条
                if frame == 0:
                    fig.colorbar(contour, ax=ax, label='Field Magnitude (V/m)')
                
                return contour
            except Exception as e:
                print(f"⚠️  插值失败: {e}")
                ax.text(0.5, 0.5, 'Interpolation Error', transform=ax.transAxes, 
                       ha='center', va='center', fontsize=14)
                return None
        
        # 创建动画
        anim = animation.FuncAnimation(fig, update_frame, frames=time_steps, 
                                     interval=200, blit=False, repeat=True)
        
        # 保存动画
        try:
            writer = PillowWriter(fps=5)
            anim.save(output_file, writer=writer)
            plt.close()
            print(f"✅ 动画创建完成: {output_file}")
            return output_file
        except Exception as e:
            print(f"❌ 动画保存失败: {e}")
            plt.close()
            return None
    
    def create_3d_field_animation(self, field_type='total', output_file=None):
        """创建3D体积场动画"""
        
        if output_file is None:
            output_file = f"satellite_{field_type}_field_3d_volume_animation.gif"
        
        print(f"🎬 创建 {field_type} 场 3D 体积动画...")
        
        # 获取数据
        obs_points = np.array(self.simulation_data['observation_points'])
        
        if field_type == 'incident':
            field_data = self.simulation_data['incident_fields']
        elif field_type == 'scattered':
            field_data = self.simulation_data['scattered_fields']
        else:
            field_data = self.simulation_data['total_fields']
        
        # 检查数据格式
        if isinstance(field_data, list) and len(field_data) > 0 and isinstance(field_data[0], (list, np.ndarray)):
            time_steps = len(field_data)
        else:
            time_steps = 15
            print(f"⚠️  检测到单时间步数据，将创建合成时间序列动画")
        
        # 创建图形 - 使用多个2D投影而不是真正的3D
        fig = plt.figure(figsize=(15, 10))
        
        # 创建子图
        ax1 = fig.add_subplot(2, 3, 1)
        ax2 = fig.add_subplot(2, 3, 2)
        ax3 = fig.add_subplot(2, 3, 3)
        ax4 = fig.add_subplot(2, 3, 4)
        ax5 = fig.add_subplot(2, 3, 5)
        ax6 = fig.add_subplot(2, 3, 6)
        
        axes = [ax1, ax2, ax3, ax4, ax5, ax6]
        
        # 不同平面的配置
        plane_configs = [
            ('xy', 0.0, 'XY Plane (z=0)'),
            ('xz', 0.0, 'XZ Plane (y=0)'),
            ('yz', 0.0, 'YZ Plane (x=0)'),
            ('xy', 0.5, 'XY Plane (z=0.5)'),
            ('xz', 0.5, 'XZ Plane (y=0.5)'),
            ('yz', 0.5, 'YZ Plane (x=0.5)')
        ]
        
        def update_frame(frame):
            # 清除所有子图
            for ax in axes:
                ax.clear()
            
            # 获取当前时间步的场数据
            if isinstance(field_data, list) and len(field_data) > 1:
                current_field = np.array(field_data[frame % len(field_data)])
            else:
                # 创建合成时间序列
                if isinstance(field_data, np.ndarray):
                    base_field = field_data
                else:
                    base_field = np.array(field_data)
                
                phase = 2 * np.pi * frame / time_steps
                current_field = base_field * np.exp(1j * phase)
            
            field_magnitude = np.abs(current_field)
            
            # 为每个平面创建可视化
            for i, (plane, z_value, title) in enumerate(plane_configs):
                ax = axes[i]
                
                # 选择平面
                if plane == 'xy':
                    plane_mask = np.abs(obs_points[:, 2] - z_value) < 0.2
                    x_idx, y_idx = 0, 1
                elif plane == 'xz':
                    plane_mask = np.abs(obs_points[:, 1] - z_value) < 0.2
                    x_idx, y_idx = 0, 2
                else:  # yz
                    plane_mask = np.abs(obs_points[:, 0] - z_value) < 0.2
                    x_idx, y_idx = 1, 2
                
                if np.sum(plane_mask) == 0:
                    ax.text(0.5, 0.5, 'No Data', transform=ax.transAxes, 
                           ha='center', va='center')
                    ax.set_title(f'{title} - No Data')
                    continue
                
                # 获取平面数据
                x_coords = obs_points[plane_mask, x_idx]
                y_coords = obs_points[plane_mask, y_idx]
                field_values = field_magnitude[plane_mask]
                
                # 创建网格和插值
                if len(x_coords) > 3:  # 需要足够的数据点
                    xi = np.linspace(x_coords.min(), x_coords.max(), 20)
                    yi = np.linspace(y_coords.min(), y_coords.max(), 20)
                    xi, yi = np.meshgrid(xi, yi)
                    
                    from scipy.interpolate import griddata
                    try:
                        zi = griddata((x_coords, y_coords), field_values, (xi, yi), method='linear')
                        zi = np.nan_to_num(zi, nan=0.0)
                        
                        # 绘制等高线图
                        contour = ax.contourf(xi, yi, zi, levels=10, cmap='jet', alpha=0.8)
                        ax.set_title(f'{title} - Step {frame + 1}')
                        
                        # 添加卫星轮廓
                        self._add_satellite_outline(ax, plane)
                        
                    except Exception as e:
                        ax.text(0.5, 0.5, 'Interpolation Error', transform=ax.transAxes, 
                               ha='center', va='center')
                        ax.set_title(f'{title} - Error')
                else:
                    ax.text(0.5, 0.5, 'Insufficient Data', transform=ax.transAxes, 
                           ha='center', va='center')
                    ax.set_title(f'{title} - Insufficient Data')
                
                ax.set_xlabel(f'{plane[0].upper()} (m)')
                ax.set_ylabel(f'{plane[1].upper()} (m)')
            
            # 主标题
            fig.suptitle(f'{field_type.title()} Field Multi-Plane Visualization - Step {frame + 1}/{time_steps}')
            
            return []
        
        # 创建动画
        anim = animation.FuncAnimation(fig, update_frame, frames=time_steps, 
                                     interval=300, blit=False, repeat=True)
        
        # 保存动画
        try:
            writer = PillowWriter(fps=3)
            anim.save(output_file, writer=writer)
            plt.close()
            print(f"✅ 3D动画创建完成: {output_file}")
            return output_file
        except Exception as e:
            print(f"❌ 3D动画保存失败: {e}")
            plt.close()
            return None
    
    def _add_satellite_outline(self, ax, plane):
        """添加卫星轮廓"""
        # 简化的卫星轮廓（椭圆表示）
        if plane == 'xy':
            # XY平面轮廓
            theta = np.linspace(0, 2*np.pi, 50)
            x = 1.4 * np.cos(theta)  # 半长轴 1.4m
            y = 1.15 * np.sin(theta)  # 半短轴 1.15m
            ax.plot(x, y, 'k--', linewidth=2, alpha=0.7, label='Satellite')
        elif plane == 'xz':
            # XZ平面轮廓
            theta = np.linspace(0, 2*np.pi, 50)
            x = 1.4 * np.cos(theta)
            z = 0.54 * np.sin(theta)  # 半短轴 0.54m
            ax.plot(x, z, 'k--', linewidth=2, alpha=0.7, label='Satellite')
        else:  # yz
            # YZ平面轮廓
            theta = np.linspace(0, 2*np.pi, 50)
            y = 1.15 * np.cos(theta)
            z = 0.54 * np.sin(theta)
            ax.plot(y, z, 'k--', linewidth=2, alpha=0.7, label='Satellite')
        
        if ax.get_legend() is None:
            ax.legend(loc='upper right')
    
    def create_all_animations(self):
        """创建所有动画"""
        print("🎬 开始生成所有电磁场动画...")
        
        animations_created = []
        
        # 创建平面动画
        for plane in ['xy', 'xz', 'yz']:
            for field_type in ['incident', 'scattered', 'total']:
                try:
                    output_file = self.create_field_plane_animation(plane, field_type)
                    if output_file:
                        animations_created.append(output_file)
                except Exception as e:
                    print(f"⚠️  创建 {plane} 平面 {field_type} 场动画失败: {e}")
        
        # 创建3D体积动画
        for field_type in ['incident', 'scattered', 'total']:
            try:
                output_file = self.create_3d_field_animation(field_type)
                if output_file:
                    animations_created.append(output_file)
            except Exception as e:
                print(f"⚠️  创建3D {field_type} 场动画失败: {e}")
        
        print(f"✅ 动画生成完成！共创建了 {len(animations_created)} 个动画文件:")
        for anim in animations_created:
            print(f"   📹 {anim}")
        
        return animations_created

def main():
    """主函数"""
    print("=" * 80)
    print("简化电磁场动画生成器")
    print("Simplified Electromagnetic Field Animation Generator")
    print("=" * 80)
    
    # 创建动画生成器
    generator = SimpleFieldAnimationGenerator()
    
    # 生成所有动画
    animations = generator.create_all_animations()
    
    print(f"\n🎉 动画生成完成！共创建了 {len(animations)} 个动画文件。")
    print("您可以在文件浏览器中查看生成的GIF动画文件。")
    
    # 列出生成的文件
    if animations:
        print("\n📁 生成的动画文件:")
        for anim in animations:
            print(f"   • {anim}")

if __name__ == "__main__":
    main()