#!/usr/bin/env python3
"""
卫星电磁场动画生成器
Satellite Electromagnetic Field Animation Generator

使用现有的仿真数据生成动画
Uses existing simulation data to generate animations
"""

import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from matplotlib.animation import PillowWriter
import json
import os
import sys

class FieldAnimationGenerator:
    """电磁场动画生成器"""
    
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
        except FileNotFoundError:
            print(f"❌ 仿真数据文件未找到: {self.data_file}")
            # 创建模拟数据用于演示
            self.create_demo_data()
        except Exception as e:
            print(f"❌ 加载仿真数据失败: {e}")
            self.create_demo_data()
    
    def create_demo_data(self):
        """创建演示数据"""
        print("📝 创建演示数据...")
        
        # 创建观测点网格
        x = np.linspace(-2, 2, 50)
        y = np.linspace(-2, 2, 50)
        z = np.linspace(-1, 1, 25)
        X, Y, Z = np.meshgrid(x, y, z)
        
        # 创建卫星几何（简化版）
        satellite_mask = ((X**2 + Y**2 + (Z*2)**2) < 1.0)
        
        # 创建平面波场
        k = 2 * np.pi / 0.03  # 10GHz波数
        omega = 2 * np.pi * 10e9
        
        # 时间步
        time_steps = 50
        time = np.linspace(0, 2*np.pi/omega, time_steps)
        
        # 存储场数据
        incident_fields = []
        scattered_fields = []
        total_fields = []
        
        for t in time:
            # 入射平面波
            incident = np.exp(1j * (k * X - omega * t))
            
            # 散射场（卫星周围的散射）
            scattered = np.zeros_like(incident)
            scattered[satellite_mask] = 0.5 * incident[satellite_mask]
            
            # 总场
            total = incident + scattered
            
            # 存储
            incident_fields.append(np.abs(incident))
            scattered_fields.append(np.abs(scattered))
            total_fields.append(np.abs(total))
        
        self.simulation_data = {
            "frequency": 10e9,
            "wavelength": 0.03,
            "observation_points": np.column_stack([X.flatten(), Y.flatten(), Z.flatten()]).tolist(),
            "incident_fields": [field.flatten().tolist() for field in incident_fields],
            "scattered_fields": [field.flatten().tolist() for field in scattered_fields],
            "total_fields": [field.flatten().tolist() for field in total_fields],
            "time_steps": time_steps,
            "simulation_time": time.tolist()
        }
        
        # 保存演示数据
        with open('demo_simulation_data.json', 'w', encoding='utf-8') as f:
            json.dump(self.simulation_data, f, ensure_ascii=False, indent=2)
        
        print("✅ 演示数据创建完成")
    
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
        
        time_steps = self.simulation_data.get('time_steps', len(field_data))
        
        # 选择平面
        if plane == 'xy':
            plane_mask = np.abs(obs_points[:, 2]) < 0.1
            x_idx, y_idx = 0, 1
            title = f'{field_type.title()} Field (XY Plane)'
        elif plane == 'xz':
            plane_mask = np.abs(obs_points[:, 1]) < 0.1
            x_idx, y_idx = 0, 2
            title = f'{field_type.title()} Field (XZ Plane)'
        else:  # yz
            plane_mask = np.abs(obs_points[:, 0]) < 0.1
            x_idx, y_idx = 1, 2
            title = f'{field_type.title()} Field (YZ Plane)'
        
        # 创建图形
        fig, ax = plt.subplots(figsize=(10, 8))
        
        # 获取平面数据
        x_coords = obs_points[plane_mask, x_idx]
        y_coords = obs_points[plane_mask, y_idx]
        
        # 创建网格
        xi = np.linspace(x_coords.min(), x_coords.max(), 30)
        yi = np.linspace(y_coords.min(), y_coords.max(), 30)
        xi, yi = np.meshgrid(xi, yi)
        
        # 动画更新函数
        def update_frame(frame):
            ax.clear()
            
            # 获取当前时间步的场数据
            current_field = np.array(field_data[frame % len(field_data)])
            field_values = current_field[plane_mask]
            
            # 插值
            from scipy.interpolate import griddata
            zi = griddata((x_coords, y_coords), field_values, (xi, yi), method='linear')
            zi = np.nan_to_num(zi, nan=0.0)
            
            # 绘制等高线图
            contour = ax.contourf(xi, yi, zi, levels=20, cmap='jet', alpha=0.8)
            
            # 添加标题和标签
            ax.set_title(f'{title} - Time Step {frame + 1}/{time_steps}')
            ax.set_xlabel(f'{plane[0].upper()} (m)')
            ax.set_ylabel(f'{plane[1].upper()} (m)')
            
            # 添加颜色条
            if frame == 0:
                fig.colorbar(contour, ax=ax, label='Field Magnitude (V/m)')
            
            # 添加卫星轮廓
            self._add_satellite_outline(ax, plane)
            
            return contour,
        
        # 创建动画
        anim = animation.FuncAnimation(fig, update_frame, frames=time_steps, 
                                     interval=200, blit=False, repeat=True)
        
        # 保存动画
        writer = PillowWriter(fps=5)
        anim.save(output_file, writer=writer)
        plt.close()
        
        print(f"✅ 动画创建完成: {output_file}")
        return output_file
    
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
        
        time_steps = self.simulation_data.get('time_steps', len(field_data))
        
        # 创建多个视角的动画
        fig = plt.figure(figsize=(15, 10))
        
        # 创建子图用于不同视角
        ax1 = fig.add_subplot(2, 2, 1, projection='3d')
        ax2 = fig.add_subplot(2, 2, 2, projection='3d')
        ax3 = fig.add_subplot(2, 2, 3, projection='3d')
        ax4 = fig.add_subplot(2, 2, 4)
        
        axes = [ax1, ax2, ax3, ax4]
        
        def update_frame(frame):
            # 清除所有子图
            for ax in axes[:-1]:  # 除了最后一个2D图
                ax.clear()
            axes[-1].clear()
            
            # 获取当前时间步的场数据
            current_field = np.array(field_data[frame % len(field_data)])
            
            # 创建散点图显示3D场分布
            field_magnitude = np.abs(current_field)
            
            # 选择要显示的点（避免太多点导致性能问题）
            n_points = min(1000, len(obs_points))
            indices = np.random.choice(len(obs_points), n_points, replace=False)
            
            x = obs_points[indices, 0]
            y = obs_points[indices, 1]
            z = obs_points[indices, 2]
            c = field_magnitude[indices]
            
            # 设置不同的视角
            for i, ax in enumerate(axes[:-1]):
                ax.scatter(x, y, z, c=c, cmap='jet', s=20, alpha=0.6)
                
                # 设置不同的视角角度
                if i == 0:
                    ax.view_init(elev=20, azim=45)
                    ax.set_title('View 1: Front-Right')
                elif i == 1:
                    ax.view_init(elev=20, azim=135)
                    ax.set_title('View 2: Back-Right')
                else:
                    ax.view_init(elev=60, azim=90)
                    ax.set_title('View 3: Top')
                
                ax.set_xlabel('X (m)')
                ax.set_ylabel('Y (m)')
                ax.set_zlabel('Z (m)')
                
                # 添加卫星轮廓
                self._add_satellite_3d_outline(ax)
            
            # 最后一个子图显示场强随时间变化
            time_values = []
            for t in range(min(10, time_steps)):
                idx = (frame - t) % time_steps
                time_values.append(np.max(np.abs(field_data[idx])))
            
            axes[-1].plot(range(len(time_values)), time_values, 'b-o')
            axes[-1].set_title('Maximum Field Strength')
            axes[-1].set_xlabel('Time Steps Back')
            axes[-1].set_ylabel('Max Field (V/m)')
            axes[-1].grid(True)
            
            # 主标题
            fig.suptitle(f'{field_type.title()} Field 3D Volume - Time Step {frame + 1}/{time_steps}')
            
            return []
        
        # 创建动画
        anim = animation.FuncAnimation(fig, update_frame, frames=min(20, time_steps), 
                                     interval=300, blit=False, repeat=True)
        
        # 保存动画
        writer = PillowWriter(fps=3)
        anim.save(output_file, writer=writer)
        plt.close()
        
        print(f"✅ 3D动画创建完成: {output_file}")
        return output_file
    
    def _add_satellite_outline(self, ax, plane):
        """添加卫星轮廓到2D图"""
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
            ax.legend()
    
    def _add_satellite_3d_outline(self, ax):
        """添加卫星3D轮廓"""
        # 创建椭球体表示卫星
        u = np.linspace(0, 2*np.pi, 20)
        v = np.linspace(0, np.pi, 20)
        
        # 卫星尺寸（根据之前的2.8×2.3×1.07m）
        a, b, c = 1.4, 1.15, 0.54  # 半轴
        
        x = a * np.outer(np.cos(u), np.sin(v))
        y = b * np.outer(np.sin(u), np.sin(v))
        z = c * np.outer(np.ones(np.size(u)), np.cos(v))
        
        # 绘制半透明表面
        ax.plot_surface(x, y, z, alpha=0.1, color='gray', linewidth=0)
        
        # 绘制轮廓线
        ax.plot(x[0, :], y[0, :], z[0, :], 'k-', alpha=0.5, linewidth=1)
        ax.plot(x[:, 0], y[:, 0], z[:, 0], 'k-', alpha=0.5, linewidth=1)
    
    def create_all_animations(self):
        """创建所有动画"""
        print("🎬 开始生成所有电磁场动画...")
        
        animations_created = []
        
        # 创建平面动画
        for plane in ['xy', 'xz', 'yz']:
            for field_type in ['incident', 'scattered', 'total']:
                try:
                    output_file = self.create_field_plane_animation(plane, field_type)
                    animations_created.append(output_file)
                except Exception as e:
                    print(f"⚠️  创建 {plane} 平面 {field_type} 场动画失败: {e}")
        
        # 创建3D体积动画
        for field_type in ['incident', 'scattered', 'total']:
            try:
                output_file = self.create_3d_field_animation(field_type)
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
    print("卫星电磁场动画生成器")
    print("Satellite Electromagnetic Field Animation Generator")
    print("=" * 80)
    
    # 创建动画生成器
    generator = FieldAnimationGenerator()
    
    # 生成所有动画
    animations = generator.create_all_animations()
    
    print("\n🎉 所有动画生成完成！")
    print("您可以在文件浏览器中查看生成的GIF动画文件。")

if __name__ == "__main__":
    main()