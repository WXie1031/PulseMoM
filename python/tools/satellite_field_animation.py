#!/usr/bin/env python3
"""
卫星电磁场动画可视化
Satellite Electromagnetic Field Animation Visualization

展示plane和volume结果的动态演化
"""

import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from mpl_toolkits.mplot3d import Axes3D
import os
import sys

# 添加父目录到路径
sys.path.append(os.path.dirname(os.path.abspath(__file__)))

from satellite_mom_final_test import SatelliteMoMTester

class SatelliteFieldAnimator:
    """卫星电磁场动画生成器"""
    
    def __init__(self, mom_tester):
        self.tester = mom_tester
        self.animation_data = {}
        
    def create_field_plane_animation(self, plane='xy', n_frames=50):
        """创建场分布平面动画"""
        
        print(f"   创建{plane.upper()}平面场动画...")
        
        # 创建平面网格
        if plane == 'xy':
            x = np.linspace(-1, 1, 40)
            y = np.linspace(-1, 1, 40)
            X, Y = np.meshgrid(x, y)
            Z = np.zeros_like(X)
        elif plane == 'xz':
            x = np.linspace(-1, 1, 40)
            z = np.linspace(-0.5, 0.5, 40)
            X, Z = np.meshgrid(x, z)
            Y = np.zeros_like(X)
        elif plane == 'yz':
            y = np.linspace(-1, 1, 40)
            z = np.linspace(-0.5, 0.5, 40)
            Y, Z = np.meshgrid(y, z)
            X = np.zeros_like(Y)
        
        # 创建观测点
        obs_points = []
        for i in range(X.shape[0]):
            for j in range(X.shape[1]):
                if plane == 'xy':
                    obs_points.append([X[i,j], Y[i,j], Z[i,j]])
                elif plane == 'xz':
                    obs_points.append([X[i,j], Y[i,j], Z[i,j]])
                elif plane == 'yz':
                    obs_points.append([X[i,j], Y[i,j], Z[i,j]])
        
        obs_points = np.array(obs_points)
        
        # 计算不同时间步的场（模拟时域演化）
        frames = []
        time_phases = np.linspace(0, 2*np.pi, n_frames)
        
        for phase in time_phases:
            # 调制入射场相位
            modulated_currents = self.tester.surface_currents * np.exp(1j * phase)
            
            # 计算散射场
            scattered_fields = self.tester.calculate_scattered_field_rwg(
                self.tester.rwg_functions, modulated_currents, obs_points, self.tester.frequency
            )
            
            # 计算总场（入射+散射）
            incident_fields = self.tester.calculate_incident_field_at_points(obs_points)
            total_fields = incident_fields + scattered_fields
            
            # 重塑为网格形状
            field_grid = np.abs(total_fields).reshape(X.shape)
            frames.append(field_grid)
        
        # 创建动画
        fig, ax = plt.subplots(figsize=(10, 8))
        
        # 初始图像
        if plane == 'xy':
            im = ax.contourf(X, Y, frames[0], levels=20, cmap='jet', alpha=0.8)
            ax.set_xlabel('X (m)')
            ax.set_ylabel('Y (m)')
        elif plane == 'xz':
            im = ax.contourf(X, Z, frames[0], levels=20, cmap='jet', alpha=0.8)
            ax.set_xlabel('X (m)')
            ax.set_ylabel('Z (m)')
        elif plane == 'yz':
            im = ax.contourf(Y, Z, frames[0], levels=20, cmap='jet', alpha=0.8)
            ax.set_xlabel('Y (m)')
            ax.set_ylabel('Z (m)')
        
        ax.set_title(f'Satellite EM Field Evolution - {plane.upper()} Plane', fontsize=14, fontweight='bold')
        ax.grid(True, alpha=0.3)
        
        # 添加卫星轮廓
        self._add_satellite_outline(ax, plane)
        
        # 颜色条
        cbar = plt.colorbar(im, ax=ax, label='Field Magnitude (V/m)')
        
        def animate(frame):
            ax.clear()
            if plane == 'xy':
                im = ax.contourf(X, Y, frames[frame], levels=20, cmap='jet', alpha=0.8)
                ax.set_xlabel('X (m)')
                ax.set_ylabel('Y (m)')
            elif plane == 'xz':
                im = ax.contourf(X, Z, frames[frame], levels=20, cmap='jet', alpha=0.8)
                ax.set_xlabel('X (m)')
                ax.set_ylabel('Z (m)')
            elif plane == 'yz':
                im = ax.contourf(Y, Z, frames[frame], levels=20, cmap='jet', alpha=0.8)
                ax.set_xlabel('Y (m)')
                ax.set_ylabel('Z (m)')
            
            ax.set_title(f'Satellite EM Field Evolution - {plane.upper()} Plane\nFrame: {frame+1}/{n_frames}', 
                        fontsize=14, fontweight='bold')
            ax.grid(True, alpha=0.3)
            self._add_satellite_outline(ax, plane)
            return im.collections
        
        anim = animation.FuncAnimation(fig, animate, frames=n_frames, 
                                     interval=100, blit=False, repeat=True)
        
        # 保存动画
        output_file = f'satellite_field_animation_{plane}.gif'
        anim.save(output_file, writer='pillow', fps=10)
        plt.close()
        
        print(f"   ✅ {plane.upper()}平面动画已生成: {output_file}")
        return output_file
    
    def create_3d_volume_animation(self, n_frames=30):
        """创建3D体积场动画"""
        
        print("   创建3D体积场动画...")
        
        # 创建3D网格点
        x = np.linspace(-0.8, 0.8, 20)
        y = np.linspace(-0.8, 0.8, 20)
        z = np.linspace(-0.3, 0.3, 15)
        
        # 创建观测点
        obs_points = []
        for xi in x:
            for yi in y:
                for zi in z:
                    obs_points.append([xi, yi, zi])
        
        obs_points = np.array(obs_points)
        
        # 计算不同相位的场
        frames = []
        time_phases = np.linspace(0, 2*np.pi, n_frames)
        
        for phase in time_phases:
            print(f"     计算帧 {len(frames)+1}/{n_frames}")
            
            # 调制电流相位
            modulated_currents = self.tester.surface_currents * np.exp(1j * phase)
            
            # 计算散射场
            scattered_fields = self.tester.calculate_scattered_field_rwg(
                self.tester.rwg_functions, modulated_currents, obs_points, self.tester.frequency
            )
            
            # 总场
            incident_fields = self.tester.calculate_incident_field_at_points(obs_points)
            total_fields = incident_fields + scattered_fields
            
            # 存储场数据
            field_data = {
                'points': obs_points,
                'magnitude': np.abs(total_fields),
                'phase': np.angle(total_fields)
            }
            frames.append(field_data)
        
        # 创建3D动画
        fig = plt.figure(figsize=(12, 10))
        ax = fig.add_subplot(111, projection='3d')
        
        def animate(frame):
            ax.clear()
            
            data = frames[frame]
            points = data['points']
            magnitude = data['magnitude']
            
            # 筛选出场值较大的点进行可视化
            threshold = np.percentile(magnitude, 70)
            mask = magnitude > threshold
            
            if np.any(mask):
                x = points[mask, 0]
                y = points[mask, 1]
                z = points[mask, 2]
                colors = magnitude[mask]
                
                # 3D散点图
                scatter = ax.scatter(x, y, z, c=colors, cmap='plasma', 
                                   s=30, alpha=0.6, vmin=np.min(magnitude), vmax=np.max(magnitude))
            
            # 添加卫星轮廓
            self._add_satellite_3d_outline(ax)
            
            ax.set_xlabel('X (m)')
            ax.set_ylabel('Y (m)')
            ax.set_zlabel('Z (m)')
            ax.set_title(f'Satellite 3D EM Field Volume - Frame {frame+1}/{n_frames}', 
                         fontsize=14, fontweight='bold')
            
            ax.set_xlim([-1, 1])
            ax.set_ylim([-1, 1])
            ax.set_zlim([-0.5, 0.5])
            
            # 颜色条
            if frame == 0:
                self.scatter_colorbar = plt.colorbar(scatter, ax=ax, shrink=0.8, label='Field Magnitude (V/m)')
            
            return ax
        
        anim = animation.FuncAnimation(fig, animate, frames=n_frames,
                                     interval=200, blit=False, repeat=True)
        
        # 保存动画
        output_file = 'satellite_3d_volume_animation.gif'
        anim.save(output_file, writer='pillow', fps=5)
        plt.close()
        
        print(f"   ✅ 3D体积动画已生成: {output_file}")
        return output_file
    
    def create_current_animation(self):
        """创建表面电流动画"""
        
        print("   创建表面电流动画...")
        
        # 获取RWG函数和电流数据
        rwg_functions = self.tester.rwg_functions
        currents = self.tester.surface_currents
        
        # 创建时间相位
        n_frames = 40
        time_phases = np.linspace(0, 2*np.pi, n_frames)
        
        # 创建图形
        fig = plt.figure(figsize=(12, 10))
        ax = fig.add_subplot(111, projection='3d')
        
        def animate(frame):
            ax.clear()
            
            # 当前相位
            phase = time_phases[frame]
            modulated_currents = currents * np.exp(1j * phase)
            
            # 绘制每个RWG函数的电流
            for i, rwg in enumerate(rwg_functions):
                if 'triangles' in rwg and len(rwg['triangles']) >= 2:
                    # 获取三角形顶点
                    tri1 = np.array(rwg['triangles'][0])
                    tri2 = np.array(rwg['triangles'][1])
                    
                    # 计算中心点
                    center1 = np.mean(tri1, axis=0)
                    center2 = np.mean(tri2, axis=0)
                    
                    # 电流大小
                    current_mag = abs(modulated_currents[i])
                    current_phase = np.angle(modulated_currents[i])
                    
                    # 绘制电流矢量
                    if current_mag > 1e3:  # 只显示大电流
                        direction = center2 - center1
                        direction_norm = direction / (np.linalg.norm(direction) + 1e-10)
                        
                        # 矢量长度与电流大小成正比
                        vector_length = min(0.1, current_mag / 1e5)
                        
                        ax.quiver(center1[0], center1[1], center1[2],
                                direction_norm[0] * vector_length,
                                direction_norm[1] * vector_length,
                                direction_norm[2] * vector_length,
                                color=plt.cm.plasma(current_mag / np.max(np.abs(currents))),
                                arrow_length_ratio=0.3, linewidth=2)
            
            # 添加卫星轮廓
            self._add_satellite_3d_outline(ax)
            
            ax.set_xlabel('X (m)')
            ax.set_ylabel('Y (m)')
            ax.set_zlabel('Z (m)')
            ax.set_title(f'Satellite Surface Current Evolution\nFrame {frame+1}/{n_frames} - Phase: {phase:.2f} rad', 
                        fontsize=14, fontweight='bold')
            
            ax.set_xlim([-0.5, 0.5])
            ax.set_ylim([-0.5, 0.5])
            ax.set_zlim([-0.3, 0.3])
            
            return ax
        
        anim = animation.FuncAnimation(fig, animate, frames=n_frames,
                                     interval=150, blit=False, repeat=True)
        
        # 保存动画
        output_file = 'satellite_current_animation.gif'
        anim.save(output_file, writer='pillow', fps=8)
        plt.close()
        
        print(f"   ✅ 表面电流动画已生成: {output_file}")
        return output_file
    
    def _add_satellite_outline(self, ax, plane='xy'):
        """添加卫星轮廓"""
        # 基于STL几何数据创建简化的卫星轮廓
        if hasattr(self.tester, 'vertices') and len(self.tester.vertices) > 0:
            vertices = self.tester.vertices
            
            if plane == 'xy':
                ax.scatter(vertices[:, 0], vertices[:, 1], c='black', s=1, alpha=0.5)
            elif plane == 'xz':
                ax.scatter(vertices[:, 0], vertices[:, 2], c='black', s=1, alpha=0.5)
            elif plane == 'yz':
                ax.scatter(vertices[:, 1], vertices[:, 2], c='black', s=1, alpha=0.5)
    
    def _add_satellite_3d_outline(self, ax):
        """添加3D卫星轮廓"""
        if hasattr(self.tester, 'vertices') and len(self.tester.vertices) > 0:
            vertices = self.tester.vertices
            ax.scatter(vertices[:, 0], vertices[:, 1], vertices[:, 2], 
                      c='black', s=2, alpha=0.3)
    
    def create_all_animations(self):
        """创建所有动画"""
        print("\n" + "="*60)
        print("创建卫星电磁场动画可视化")
        print("Satellite EM Field Animation Generation")
        print("="*60)
        
        animation_files = []
        
        # 创建平面动画
        for plane in ['xy', 'xz', 'yz']:
            try:
                file = self.create_field_plane_animation(plane)
                animation_files.append(file)
            except Exception as e:
                print(f"   ⚠️ {plane.upper()}平面动画生成失败: {e}")
        
        # 创建3D体积动画
        try:
            file = self.create_3d_volume_animation()
            animation_files.append(file)
        except Exception as e:
            print(f"   ⚠️ 3D体积动画生成失败: {e}")
        
        # 创建电流动画
        try:
            file = self.create_current_animation()
            animation_files.append(file)
        except Exception as e:
            print(f"   ⚠️ 表面电流动画生成失败: {e}")
        
        print(f"\n✅ 动画生成完成！共生成 {len(animation_files)} 个动画文件:")
        for file in animation_files:
            print(f"   📁 {file}")
        
        return animation_files

def main():
    """主函数"""
    
    print("启动卫星电磁场动画生成器...")
    
    # 创建测试实例（使用之前成功的参数）
    tester = SatelliteMoMTester(
        stl_file='tests/test_hpm/weixing_v1.stl',
        pfd_file='tests/test_hpm/weixing_v1_case.pfd',
        max_facets=2000,
        frequency=10e9
    )
    
    # 运行完整测试（确保数据可用）
    print("运行MoM仿真...")
    success = tester.run_complete_test()
    
    if not success:
        print("❌ MoM仿真失败，无法生成动画")
        return 1
    
    # 创建动画生成器
    animator = SatelliteFieldAnimator(tester)
    
    # 生成所有动画
    animation_files = animator.create_all_animations()
    
    print(f"\n🎉 卫星电磁场动画生成完成！")
    print(f"📁 输出文件: {', '.join(animation_files)}")
    
    return 0

if __name__ == "__main__":
    sys.exit(main())