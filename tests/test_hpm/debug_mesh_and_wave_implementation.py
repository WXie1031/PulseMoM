#!/usr/bin/env python3
"""
调试网格划分和平面波入射实现
检查STL导入、网格划分和波形入射是否正确
"""

import numpy as np
import h5py
import json
import os

def debug_mesh_implementation():
    """调试网格划分实现"""
    print("🔍 调试网格划分和平面波入射实现")
    print("="*60)
    
    # 1. 检查STL几何体导入情况
    print("\n1. STL几何体导入分析:")
    
    # 从之前的分析结果
    stl_center = np.array([0.000, 0.000, 0.560])  # STL中心（米）
    stl_size = np.array([2.797, 2.797, 1.020])    # STL尺寸（米）
    domain_size = np.array([3.4, 3.4, 1.4])       # 域尺寸
    grid_spacing = 0.02                           # 网格间距（20mm）
    
    print(f"   STL几何体:")
    print(f"     中心: [{stl_center[0]:.3f}, {stl_center[1]:.3f}, {stl_center[2]:.3f}] m")
    print(f"     尺寸: [{stl_size[0]:.3f}, {stl_size[1]:.3f}, {stl_size[2]:.3f}] m")
    print(f"   计算域: [{domain_size[0]:.1f}, {domain_size[1]:.1f}, {domain_size[2]:.1f}] m")
    print(f"   网格间距: {grid_spacing*1000:.0f} mm")
    
    # 计算网格划分
    grid_dims = (domain_size / grid_spacing).astype(int)
    print(f"   网格维度: {grid_dims[0]}×{grid_dims[1]}×{grid_dims[2]} = {np.prod(grid_dims):,} 网格")
    
    # 2. 分析.pfd文件中的配置
    print(f"\n2. .pfd文件配置分析:")
    print(f"   GEOMETRY_TRANSLATE 0 0 -550  (mm)")
    print(f"   这意味着STL在Z方向平移-550mm")
    
    # 平移后的STL位置
    stl_translation = np.array([0, 0, -0.55])  # -550mm to meters
    translated_stl_center = stl_center + stl_translation
    print(f"   平移后STL中心: [{translated_stl_center[0]:.3f}, {translated_stl_center[1]:.3f}, {translated_stl_center[2]:.3f}] m")
    
    # 3. 检查网格是否包含卫星
    print(f"\n3. 网格覆盖分析:")
    
    # 计算网格边界
    grid_bounds_min = -domain_size / 2
    grid_bounds_max = domain_size / 2
    
    print(f"   网格边界:")
    print(f"     X: [{grid_bounds_min[0]:.2f}, {grid_bounds_max[0]:.2f}] m")
    print(f"     Y: [{grid_bounds_min[1]:.2f}, {grid_bounds_max[1]:.2f}] m")
    print(f"     Z: [{grid_bounds_min[2]:.2f}, {grid_bounds_max[2]:.2f}] m")
    
    # 检查平移后的STL是否在网格内
    stl_bounds_min = translated_stl_center - stl_size/2
    stl_bounds_max = translated_stl_center + stl_size/2
    
    print(f"   平移后STL边界:")
    print(f"     X: [{stl_bounds_min[0]:.2f}, {stl_bounds_max[0]:.2f}] m")
    print(f"     Y: [{stl_bounds_min[1]:.2f}, {stl_bounds_max[1]:.2f}] m")
    print(f"     Z: [{stl_bounds_min[2]:.2f}, {stl_bounds_max[2]:.2f}] m")
    
    # 检查是否完全在网格内
    inside_grid = (
        grid_bounds_min[0] <= stl_bounds_min[0] and stl_bounds_max[0] <= grid_bounds_max[0] and
        grid_bounds_min[1] <= stl_bounds_min[1] and stl_bounds_max[1] <= grid_bounds_max[1] and
        grid_bounds_min[2] <= stl_bounds_min[2] and stl_bounds_max[2] <= grid_bounds_max[2]
    )
    print(f"   STL完全在网格内: {inside_grid}")
    
    # 4. 检查平面波入射配置
    print(f"\n4. 平面波入射配置:")
    theta, phi, psi = 45.0, 45.0, 45.0  # 来自.pfd文件
    print(f"   入射角度: θ={theta}°, φ={phi}°, ψ={psi}°")
    
    # 计算波矢量方向
    theta_rad = np.radians(theta)
    phi_rad = np.radians(phi)
    
    k_direction = np.array([
        np.sin(theta_rad) * np.cos(phi_rad),
        np.sin(theta_rad) * np.sin(phi_rad),
        np.cos(theta_rad)
    ])
    print(f"   波矢量: k = [{k_direction[0]:.3f}, {k_direction[1]:.3f}, {k_direction[2]:.3f}]")
    
    # 5. 检查波形文件
    print(f"\n5. 波形文件检查:")
    waveform_file = "hpm_waveform_X(10.0GHz)_20ns.txt"
    if os.path.exists(waveform_file):
        data = np.loadtxt(waveform_file)
        print(f"   文件存在: {waveform_file}")
        print(f"   数据点数: {len(data)}")
        print(f"   时间范围: {data[0,0]*1e9:.1f} - {data[-1,0]*1e9:.1f} ns")
        print(f"   幅值范围: {np.min(data[:,1]):.3f} - {np.max(data[:,1]):.3f}")
        print(f"   频率内容: 10 GHz调制")
    else:
        print(f"   ❌ 波形文件不存在: {waveform_file}")
    
    # 6. 检查仿真输出
    print(f"\n6. 仿真输出验证:")
    results_file = "output_time_domain/time_domain_results.h5"
    if os.path.exists(results_file):
        with h5py.File(results_file, 'r') as f:
            datasets = list(f.keys())
            print(f"   输出文件存在: {results_file}")
            print(f"   数据集数量: {len(datasets)}")
            
            # 检查主要数据集
            key_datasets = ['plane_x0_Ex', 'plane_x0_Ey', 'plane_x0_Ez']
            for key in key_datasets:
                if key in f:
                    data = f[key][:]
                    print(f"   {key}: shape={data.shape}, range=[{np.min(data):.3f}, {np.max(data):.3f}] V/m")
                else:
                    print(f"   ❌ 缺少数据集: {key}")
    else:
        print(f"   ❌ 输出文件不存在: {results_file}")
    
    # 7. 检查是否包含卫星结构
    print(f"\n7. 卫星结构包含检查:")
    
    # 分析观测点位置
    obs_points_file = "output_time_domain/time_domain_results.h5"
    if os.path.exists(obs_points_file):
        with h5py.File(obs_points_file, 'r') as f:
            if 'coordinates' in f:
                coords_group = f['coordinates']
                
                # 获取平面监测点坐标
                if 'plane_x0_Ex' in coords_group:
                    plane_coords = coords_group['plane_x0_Ex'][:]
                    print(f"   平面监测点数量: {plane_coords.shape[0]*plane_coords.shape[1]:,}")
                    
                    # 检查是否有监测点在卫星内部
                    plane_coords_reshaped = plane_coords.reshape(-1, 3)
                    
                    # 检查每个点是否在STL边界内
                    in_satellite = (
                        (stl_bounds_min[0] <= plane_coords_reshaped[:, 0]) & 
                        (plane_coords_reshaped[:, 0] <= stl_bounds_max[0]) &
                        (stl_bounds_min[1] <= plane_coords_reshaped[:, 1]) & 
                        (plane_coords_reshaped[:, 1] <= stl_bounds_max[1]) &
                        (stl_bounds_min[2] <= plane_coords_reshaped[:, 2]) & 
                        (plane_coords_reshaped[:, 2] <= stl_bounds_max[2])
                    )
                    
                    points_in_satellite = np.sum(in_satellite)
                    total_points = len(plane_coords_reshaped)
                    
                    print(f"   监测点在卫星内: {points_in_satellite:,} / {total_points:,} ({points_in_satellite/total_points*100:.1f}%)")
                    
                    if points_in_satellite == 0:
                        print(f"   ⚠️  警告: 没有监测点在卫星内部！")
                        print(f"   这可能解释了为什么看不到卫星结构的影响")
                    
                    # 显示边界信息
                    print(f"   坐标范围:")
                    print(f"     X: [{np.min(plane_coords_reshaped[:,0]):.2f}, {np.max(plane_coords_reshaped[:,0]):.2f}] m")
                    print(f"     Y: [{np.min(plane_coords_reshaped[:,1]):.2f}, {np.max(plane_coords_reshaped[:,1]):.2f}] m")
                    print(f"     Z: [{np.min(plane_coords_reshaped[:,2]):.2f}, {np.max(plane_coords_reshaped[:,2]):.2f}] m")
    
    # 8. 网格可视化建议
    print(f"\n8. 问题诊断:")
    print(f"   主要问题:")
    print(f"   1. STL单位: 文件使用毫米，需要转换为米")
    print(f"   2. 坐标系: STL中心在[0,0,0.56]m，需要平移到域中心")
    print(f"   3. 网格覆盖: 需要确保网格正确划分并包含卫星结构")
    print(f"   4. 监测点: 需要验证监测点是否覆盖卫星内部")
    
    print(f"\n建议修复:")
    print(f"   1. 在仿真中正确处理STL单位转换")
    print(f"   2. 应用正确的几何平移")
    print(f"   3. 确保网格划分包含整个卫星结构")
    print(f"   4. 在卫星内部设置监测点以观察散射效应")

if __name__ == "__main__":
    debug_mesh_implementation()