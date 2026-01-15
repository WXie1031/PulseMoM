#!/usr/bin/env python3
"""
调试三角形面积计算问题
Debug triangle area calculation issues
"""

import numpy as np
import sys
import os
sys.path.append(os.path.dirname(os.path.abspath(__file__)))

def triangle_area_coords(vertices):
    """使用坐标计算三角形面积"""
    if len(vertices) != 3:
        return 0.0
    
    # 使用叉积计算面积
    v0, v1, v2 = vertices
    edge1 = np.array(v1) - np.array(v0)
    edge2 = np.array(v2) - np.array(v0)
    
    cross_prod = np.cross(edge1, edge2)
    area = 0.5 * np.linalg.norm(cross_prod)
    
    return area

def triangle_area_indices(vertices, indices):
    """使用顶点索引计算三角形面积"""
    if len(indices) != 3:
        return 0.0
    
    try:
        # 获取实际顶点坐标
        v0 = np.array(vertices[indices[0]])
        v1 = np.array(vertices[indices[1]])
        v2 = np.array(vertices[indices[2]])
        
        # 计算叉积
        edge1 = v1 - v0
        edge2 = v2 - v0
        
        cross_prod = np.cross(edge1, edge2)
        area = 0.5 * np.linalg.norm(cross_prod)
        
        return area
    except Exception as e:
        print(f"面积计算错误: {e}")
        return 0.0

def analyze_stl_geometry():
    """分析STL几何数据"""
    try:
        from satellite_mom_peec_final import SatelliteMoMPEECTester
        
        # 创建测试实例
        tester = SatelliteMoMPEECTester()
        
        print("=== STL几何分析 ===")
        print(f"STL文件: {tester.stl_file}")
        
        # 解析STL（简化版本）
        stl_file = "tests/test_hpm/weixing_v1.stl"
        if os.path.exists(stl_file):
            # 模拟解析过程
            vertices = [
                [0.8843922270317207, 0.9294813372452537, 0.5552120199455312],
                [0.8826585972344652, 0.9294813372452537, 0.5528438170168536],
                [0.8820240241030429, 0.9294813372452537, 0.5552120199455312]
            ]
            
            facets = [
                [0, 1, 2],  # 顶点索引
                [1, 2, 3],
                [2, 3, 4]
            ]
            
            print(f"顶点数: {len(vertices)}")
            print(f"面片数: {len(facets)}")
            
            # 分析每个三角形的面积
            for i, facet in enumerate(facets[:5]):  # 只检查前5个
                if max(facet) < len(vertices):
                    area = triangle_area_indices(vertices, facet)
                    print(f"三角形 {i}: 顶点索引 {facet} -> 面积: {area:.6e}")
                    
                    # 检查顶点坐标
                    v0 = vertices[facet[0]]
                    v1 = vertices[facet[1]]
                    v2 = vertices[facet[2]]
                    print(f"  顶点0: {v0}")
                    print(f"  顶点1: {v1}")
                    print(f"  顶点2: {v2}")
                    
                    # 计算边长
                    edge1 = np.linalg.norm(np.array(v1) - np.array(v0))
                    edge2 = np.linalg.norm(np.array(v2) - np.array(v1))
                    edge3 = np.linalg.norm(np.array(v0) - np.array(v2))
                    print(f"  边长: {edge1:.6f}, {edge2:.6f}, {edge3:.6f}")
                    
                    # 检查是否共线
                    v0_arr = np.array(v0)
                    v1_arr = np.array(v1)
                    v2_arr = np.array(v2)
                    
                    edge1_vec = v1_arr - v0_arr
                    edge2_vec = v2_arr - v0_arr
                    cross_prod = np.cross(edge1_vec, edge2_vec)
                    cross_magnitude = np.linalg.norm(cross_prod)
                    print(f"  叉积大小: {cross_magnitude:.6e}")
                    
                    if cross_magnitude < 1e-10:
                        print(f"  ⚠️  警告: 三角形 {i} 接近共线或退化！")
                    
                    print()
        
        # 检查几何尺寸
        print("=== 几何尺寸分析 ===")
        # 从实际数据文件读取一些顶点
        try:
            import json
            with open("satellite_mom_peec_simulation_data.json", "r") as f:
                data = json.load(f)
                vertices = data.get("vertices", [])
                
                if vertices:
                    # 计算边界框
                    x_coords = [v[0] for v in vertices[:100]]  # 只取前100个顶点
                    y_coords = [v[1] for v in vertices[:100]]
                    z_coords = [v[2] for v in vertices[:100]]
                    
                    x_min, x_max = min(x_coords), max(x_coords)
                    y_min, y_max = min(y_coords), max(y_coords)
                    z_min, z_max = min(z_coords), max(z_coords)
                    
                    print(f"顶点范围 (前100个):")
                    print(f"  X: {x_min:.6f} -> {x_max:.6f} (范围: {x_max-x_min:.6f})")
                    print(f"  Y: {y_min:.6f} -> {y_max:.6f} (范围: {y_max-y_min:.6f})")
                    print(f"  Z: {z_min:.6f} -> {z_max:.6f} (范围: {z_max-z_min:.6f})")
                    
                    # 检查坐标尺度
                    max_range = max(x_max-x_min, y_max-y_min, z_max-z_min)
                    print(f"最大坐标范围: {max_range:.6f} m")
                    
                    if max_range < 0.1:  # 小于10cm
                        print("⚠️  警告: 几何尺寸过小，可能需要缩放！")
                    elif max_range > 100:  # 大于100m
                        print("⚠️  警告: 几何尺寸过大，可能需要缩放！")
                    
        except Exception as e:
            print(f"无法读取仿真数据: {e}")
    
    except Exception as e:
        print(f"分析失败: {e}")
        import traceback
        traceback.print_exc()

if __name__ == "__main__":
    analyze_stl_geometry()