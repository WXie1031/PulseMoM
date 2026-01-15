#!/usr/bin/env python3
"""
Debug RWG Basis Function Coupling Analysis
分析RWG基函数与平面波的耦合情况
"""

import numpy as np
import matplotlib.pyplot as plt
from pathlib import Path
import sys

# 导入现有的测试框架
sys.path.append(str(Path(__file__).parent))

try:
    from fixed_satellite_mom_peec_final import FixedSatelliteMoMPEECTester
    from satellite_mom_peec_final import ProfessionalSTLParser, AdvancedRWGBasisGenerator
except ImportError as e:
    print(f"导入失败: {e}")
    sys.exit(1)

def get_triangle_center(triangle):
    """计算三角形中心"""
    vertices = triangle['vertices']
    if len(vertices) != 3:
        return [0, 0, 0]
    
    center = np.mean(np.array(vertices), axis=0)
    return center.tolist()

def analyze_rwg_orientations_and_coupling():
    """分析RWG基函数方向和平面波耦合"""
    
    print("=" * 80)
    print("RWG基函数方向与平面波耦合分析")
    print("=" * 80)
    
    # 初始化测试器
    tester = FixedSatelliteMoMPEECTester(
        stl_file='tests/test_hpm/weixing_v1.stl',
        frequency=10e9,
        max_facets=1000  # 减少面片数量便于分析
    )
    
    # 解析STL文件
    print("1. 解析STL文件...")
    success = tester._parse_stl_geometry_fixed()
    if not success:
        print("❌ STL文件解析失败")
        return
    
    print(f"   顶点数量: {len(tester.vertices)}")
    print(f"   面片数量: {len(tester.facets)}")
    
    # 生成RWG基函数
    print("2. 生成RWG基函数...")
    rwg_success = tester._generate_rwg_basis()
    if not rwg_success:
        print("❌ RWG基函数生成失败")
        return
    
    print(f"   RWG基函数数量: {len(tester.rwg_functions)}")
    
    # 分析卫星几何
    print("3. 分析卫星几何...")
    vertices_array = np.array(tester.vertices)
    min_coords = np.min(vertices_array, axis=0)
    max_coords = np.max(vertices_array, axis=0)
    dimensions = max_coords - min_coords
    center = (min_coords + max_coords) / 2
    
    print(f"   卫星中心: {center}")
    print(f"   卫星尺寸: X={dimensions[0]:.3f}, Y={dimensions[1]:.3f}, Z={dimensions[2]:.3f}")
    print(f"   最大尺寸方向: {'XYZ'[np.argmax(dimensions)]}")
    
    # 分析RWG基函数方向
    print("4. 分析RWG基函数方向...")
    rwg_orientations = []
    rwg_positions = []
    
    for i, rwg in enumerate(tester.rwg_functions[:20]):  # 只分析前20个
        # 获取正负三角形中心
        plus_center = get_triangle_center(rwg['plus_triangle'])
        minus_center = get_triangle_center(rwg['minus_triangle'])
        
        # 计算RWG基函数主要方向（从负三角形指向正三角形）
        rwg_direction = np.array(plus_center) - np.array(minus_center)
        rwg_direction = rwg_direction / (np.linalg.norm(rwg_direction) + 1e-10)
        
        # 使用正负三角形中心的中点作为位置
        rwg_center = (np.array(plus_center) + np.array(minus_center)) / 2
        
        rwg_orientations.append(rwg_direction)
        rwg_positions.append(rwg_center)
        
        if i < 5:  # 打印前5个的详细信息
            print(f"   RWG {i}:")
            print(f"     中心: {rwg_center}")
            print(f"     方向: {rwg_direction}")
            print(f"     边长: {rwg.get('edge_length', 'N/A'):.6f}")
            print(f"     正三角形面积: {rwg['plus_triangle']['area']:.6f}")
            print(f"     负三角形面积: {rwg['minus_triangle']['area']:.6f}")
    
    # 分析不同入射方向的耦合
    print("5. 分析不同入射方向的耦合...")
    
    # 定义多个入射方向
    incident_directions = [
        np.array([1, 0, 0]),   # X方向
        np.array([0, 1, 0]),   # Y方向  
        np.array([0, 0, 1]),   # Z方向
        np.array([1, 1, 0]),   # XY对角线
        np.array([1, 0, 1]),   # XZ对角线
        np.array([0, 1, 1]),   # YZ对角线
        np.array([1, 1, 1]),   # XYZ对角线
    ]
    
    # 对应的极化方向（垂直于入射方向）
    polarizations = []
    for direction in incident_directions:
        # 选择垂直于入射方向的极化
        if abs(direction[0]) < 0.9:
            polarization = np.array([1, 0, 0])
        elif abs(direction[1]) < 0.9:
            polarization = np.array([0, 1, 0])
        else:
            polarization = np.array([0, 0, 1])
        
        # 确保极化垂直于入射方向
        polarization = polarization - np.dot(polarization, direction) * direction
        polarization = polarization / (np.linalg.norm(polarization) + 1e-10)
        polarizations.append(polarization)
    
    # 测试每个方向的耦合
    best_direction = None
    best_coupling = 0
    best_excitations = None
    
    for i, (direction, polarization) in enumerate(zip(incident_directions, polarizations)):
        print(f"   测试方向 {i+1}: {direction} (极化: {polarization})")
        
        # 计算该方向的激励
        excitations = []
        total_coupling = 0
        
        for j, rwg in enumerate(tester.rwg_functions[:50]):  # 测试前50个
            excitation = tester._calculate_plane_wave_excitation_fixed(rwg, direction, polarization)
            excitations.append(excitation)
            total_coupling += abs(excitation)
        
        avg_coupling = total_coupling / len(excitations)
        max_coupling = max(abs(exc) for exc in excitations)
        
        print(f"     平均耦合: {avg_coupling:.6e}")
        print(f"     最大耦合: {max_coupling:.6e}")
        print(f"     非零激励数量: {sum(1 for exc in excitations if abs(exc) > 1e-15)}")
        
        if avg_coupling > best_coupling:
            best_coupling = avg_coupling
            best_direction = direction
            best_excitations = excitations
            best_polarization = polarization
    
    print(f"\n   最佳入射方向: {best_direction} (耦合: {best_coupling:.6e})")
    
    # 分析最佳方向的激励分布
    if best_excitations:
        print("6. 分析最佳方向的激励分布...")
        excitation_array = np.array(best_excitations)
        
        print(f"   激励统计:")
        print(f"     平均值: {np.mean(np.abs(excitation_array)):.6e}")
        print(f"     标准差: {np.std(np.abs(excitation_array)):.6e}")
        print(f"     最小值: {np.min(np.abs(excitation_array)):.6e}")
        print(f"     最大值: {np.max(np.abs(excitation_array)):.6e}")
        print(f"     非零比例: {np.sum(np.abs(excitation_array) > 1e-15) / len(excitation_array) * 100:.1f}%")
        
        # 找出激励最大的RWG函数
        max_idx = np.argmax(np.abs(excitation_array))
        print(f"   最大激励的RWG函数索引: {max_idx}")
        print(f"   最大激励值: {excitation_array[max_idx]:.6e}")
        
        # 分析该RWG函数的几何特征
        if max_idx < len(tester.rwg_functions):
            max_rwg = tester.rwg_functions[max_idx]
            max_plus_center = get_triangle_center(max_rwg['plus_triangle'])
            max_minus_center = get_triangle_center(max_rwg['minus_triangle'])
            max_center = (np.array(max_plus_center) + np.array(max_minus_center)) / 2
            
            print(f"   最大激励RWG几何:")
            print(f"     中心: {max_center}")
            print(f"     边长: {max_rwg.get('edge_length', 'N/A'):.6f}")
            print(f"     正三角形面积: {max_rwg['plus_triangle']['area']:.6f}")
            print(f"     负三角形面积: {max_rwg['minus_triangle']['area']:.6f}")
    
    # 建议改进方案
    print("\n7. 改进建议:")
    print(f"   1. 使用最佳入射方向: {best_direction}")
    print(f"   2. 使用最佳极化方向: {best_polarization}")
    print(f"   3. 考虑使用多个入射方向进行仿真")
    print(f"   4. 检查RWG基函数实现是否正确")
    print(f"   5. 考虑增加网格密度以提高耦合")
    
    return {
        'best_direction': best_direction,
        'best_polarization': best_polarization,
        'best_coupling': best_coupling,
        'excitation_stats': {
            'mean': np.mean(np.abs(excitation_array)),
            'std': np.std(np.abs(excitation_array)),
            'min': np.min(np.abs(excitation_array)),
            'max': np.max(np.abs(excitation_array))
        }
    }

def test_enhanced_coupling():
    """测试增强耦合机制"""
    
    print("\n" + "=" * 80)
    print("测试增强耦合机制")
    print("=" * 80)
    
    # 首先运行分析
    analysis_results = analyze_rwg_orientations_and_coupling()
    
    if analysis_results['best_coupling'] < 1e-12:
        print("❌ 耦合仍然太弱，需要进一步改进")
        
        # 建议的改进措施
        print("\n建议的改进措施:")
        print("1. 检查平面波幅度 - 可能需要增加电场强度")
        print("2. 检查RWG基函数归一化 - 可能需要重新归一化")
        print("3. 检查单位一致性 - 确保所有单位匹配")
        print("4. 考虑使用磁流激励代替电流激励")
        print("5. 检查三角形法向量方向 - 确保一致性")
        
    else:
        print(f"✅ 找到有效耦合方向: {analysis_results['best_coupling']:.6e}")
        print("可以在此方向上进行完整仿真")

if __name__ == "__main__":
    test_enhanced_coupling()