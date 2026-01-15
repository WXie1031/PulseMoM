#!/usr/bin/env python3
"""
Analyze STL geometry and coordinate mapping for satellite HPM simulation
Address user questions about STL import correctness and coordinate alignment
"""

import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
import os

def parse_stl_ascii(filename):
    """Parse ASCII STL file"""
    vertices = []
    normals = []
    
    try:
        with open(filename, 'r') as f:
            lines = f.readlines()
        
        i = 0
        while i < len(lines):
            line = lines[i].strip()
            
            if line.startswith('facet normal'):
                # 解析法向量
                parts = line.split()
                normal = [float(parts[2]), float(parts[3]), float(parts[4])]
                normals.append(normal)
                
                # 跳转到outer loop
                i += 1
                
                # 解析三个顶点
                triangle_vertices = []
                for j in range(3):
                    i += 1  # 跳过'outer loop'或'vertex'
                    vertex_line = lines[i].strip()
                    if vertex_line.startswith('vertex'):
                        parts = vertex_line.split()
                        vertex = [float(parts[1]), float(parts[2]), float(parts[3])]
                        triangle_vertices.append(vertex)
                
                vertices.extend(triangle_vertices)
                
                # 跳过endloop和endfacet
                i += 2
            else:
                i += 1
        
        return np.array(vertices), np.array(normals)
        
    except Exception as e:
        print(f"Error parsing STL file: {e}")
        return None, None

def analyze_geometry(vertices, title="STL Geometry Analysis"):
    """Analyze geometry properties"""
    print(f"\n{'='*60}")
    print(f"{title}")
    print(f"{'='*60}")
    
    if vertices is None or len(vertices) == 0:
        print("No vertices found!")
        return None
    
    # 基本统计
    print(f"Total vertices: {len(vertices)}")
    print(f"Unique vertices: {len(np.unique(vertices, axis=0))}")
    
    # 边界框
    min_coords = np.min(vertices, axis=0)
    max_coords = np.max(vertices, axis=0)
    center = (min_coords + max_coords) / 2
    size = max_coords - min_coords
    
    print(f"\nBounding Box:")
    print(f"  Min: [{min_coords[0]:.1f}, {min_coords[1]:.1f}, {min_coords[2]:.1f}] mm")
    print(f"  Max: [{max_coords[0]:.1f}, {max_coords[1]:.1f}, {max_coords[2]:.1f}] mm")
    print(f"  Center: [{center[0]:.1f}, {center[1]:.1f}, {center[2]:.1f}] mm")
    print(f"  Size: [{size[0]:.1f}, {size[1]:.1f}, {size[2]:.1f}] mm")
    
    # 几何属性
    print(f"\nGeometry Properties:")
    print(f"  X-range: {size[0]:.1f} mm")
    print(f"  Y-range: {size[1]:.1f} mm") 
    print(f"  Z-range: {size[2]:.1f} mm")
    print(f"  Volume estimate: {size[0]*size[1]*size[2]/1e9:.3f} m³")
    
    return {
        'min_coords': min_coords,
        'max_coords': max_coords,
        'center': center,
        'size': size,
        'vertices': vertices
    }

def check_coordinate_mapping(stl_info, domain_config):
    """Check coordinate mapping between STL and FDTD domain"""
    print(f"\n{'='*60}")
    print("Coordinate System Mapping Analysis")
    print(f"{'='*60}")
    
    # STL几何信息（转换为米）
    stl_center_m = stl_info['center'] / 1000  # mm -> m
    stl_size_m = stl_info['size'] / 1000      # mm -> m
    
    # FDTD域配置
    domain_size = np.array(domain_config['domain_size'])  # [3.4, 3.4, 1.4] m
    domain_center = domain_size / 2  # [1.7, 1.7, 0.7] m
    satellite_size = np.array(domain_config['satellite_size'])  # [2.8, 2.8, 1.0] m
    
    print(f"FDTD Domain Configuration:")
    print(f"  Domain size: [{domain_size[0]:.1f}, {domain_size[1]:.1f}, {domain_size[2]:.1f}] m")
    print(f"  Domain center: [{domain_center[0]:.1f}, {domain_center[1]:.1f}, {domain_center[2]:.1f}] m")
    print(f"  Expected satellite size: [{satellite_size[0]:.1f}, {satellite_size[1]:.1f}, {satellite_size[2]:.1f}] m")
    
    print(f"\nSTL Geometry (converted to meters):")
    print(f"  STL center: [{stl_center_m[0]:.3f}, {stl_center_m[1]:.3f}, {stl_center_m[2]:.3f}] m")
    print(f"  STL size: [{stl_size_m[0]:.3f}, {stl_size_m[1]:.3f}, {stl_size_m[2]:.3f}] m")
    
    # 检查尺寸匹配
    size_tolerance = 0.1  # 10% tolerance
    size_match = all(abs(stl_size_m[i] - satellite_size[i]) / satellite_size[i] < size_tolerance 
                     for i in range(3))
    
    print(f"\nSize Matching Check:")
    print(f"  X-dimension: STL {stl_size_m[0]:.3f}m vs Expected {satellite_size[0]:.3f}m "
          f"({'✓' if abs(stl_size_m[0] - satellite_size[0]) / satellite_size[0] < size_tolerance else '✗'})")
    print(f"  Y-dimension: STL {stl_size_m[1]:.3f}m vs Expected {satellite_size[1]:.3f}m "
          f"({'✓' if abs(stl_size_m[1] - satellite_size[1]) / satellite_size[1] < size_tolerance else '✗'})")
    print(f"  Z-dimension: STL {stl_size_m[2]:.3f}m vs Expected {satellite_size[2]:.3f}m "
          f"({'✓' if abs(stl_size_m[2] - satellite_size[2]) / satellite_size[2] < size_tolerance else '✗'})")
    print(f"  Overall size match: {'✓ PASS' if size_match else '✗ FAIL'}")
    
    # 坐标系对齐分析
    print(f"\nCoordinate System Alignment:")
    
    # 预期的卫星中心位置（在域中心）
    expected_satellite_center = domain_center  # [1.7, 1.7, 0.7] m
    
    # 当前STL中心位置
    current_stl_center = stl_center_m
    
    # 需要的平移量
    required_translation = expected_satellite_center - current_stl_center
    
    print(f"  Expected satellite center (domain center): [{expected_satellite_center[0]:.3f}, {expected_satellite_center[1]:.3f}, {expected_satellite_center[2]:.3f}] m")
    print(f"  Current STL center: [{current_stl_center[0]:.3f}, {current_stl_center[1]:.3f}, {current_stl_center[2]:.3f}] m")
    print(f"  Required translation: [{required_translation[0]:.3f}, {required_translation[1]:.3f}, {required_translation[2]:.3f}] m")
    
    # 检查观测点配置
    print(f"\nObservation Points Configuration Check:")
    print(f"  Domain center: (0, 0, 0) in simulation coordinates")
    print(f"  Satellite should be at: {required_translation} in simulation coordinates")
    print(f"  Observation points should center around: (0, 0, 0) - domain center")
    
    return {
        'size_match': size_match,
        'required_translation': required_translation,
        'stl_center_m': stl_center_m,
        'expected_satellite_center': expected_satellite_center
    }

def visualize_geometry_comparison(stl_info, mapping_info):
    """Visualize geometry and coordinate mapping"""
    fig = plt.figure(figsize=(15, 10))
    
    # 1. STL几何原始位置
    ax1 = fig.add_subplot(221, projection='3d')
    vertices = stl_info['vertices'] / 1000  # 转换为米
    ax1.scatter(vertices[:, 0], vertices[:, 1], vertices[:, 2], 
               c='blue', s=1, alpha=0.3, label='STL vertices')
    
    # 边界框
    min_m = stl_info['min_coords'] / 1000
    max_m = stl_info['max_coords'] / 1000
    center_m = stl_info['center'] / 1000
    
    # 绘制边界框
    bbox = np.array([
        [min_m[0], min_m[1], min_m[2]],
        [max_m[0], min_m[1], min_m[2]],
        [max_m[0], max_m[1], min_m[2]],
        [min_m[0], max_m[1], min_m[2]],
        [min_m[0], min_m[1], max_m[2]],
        [max_m[0], min_m[1], max_m[2]],
        [max_m[0], max_m[1], max_m[2]],
        [min_m[0], max_m[1], max_m[2]]
    ])
    
    ax1.scatter(center_m[0], center_m[1], center_m[2], 
               c='red', s=100, marker='*', label='STL center')
    ax1.set_title('Original STL Geometry')
    ax1.set_xlabel('X (m)')
    ax1.set_ylabel('Y (m)')
    ax1.set_zlabel('Z (m)')
    
    # 2. FDTD域配置
    ax2 = fig.add_subplot(222, projection='3d')
    domain_size = np.array([3.4, 3.4, 1.4])
    domain_center = domain_size / 2
    
    # 绘制域边界
    domain_bbox = np.array([
        [0, 0, 0],
        [domain_size[0], 0, 0],
        [domain_size[0], domain_size[1], 0],
        [0, domain_size[1], 0],
        [0, 0, domain_size[2]],
        [domain_size[0], 0, domain_size[2]],
        [domain_size[0], domain_size[1], domain_size[2]],
        [0, domain_size[1], domain_size[2]]
    ])
    
    ax2.scatter(domain_center[0], domain_center[1], domain_center[2], 
               c='green', s=100, marker='*', label='Domain center')
    ax2.set_title('FDTD Domain Configuration')
    ax2.set_xlabel('X (m)')
    ax2.set_ylabel('Y (m)')
    ax2.set_zlabel('Z (m)')
    
    # 3. 坐标映射分析
    ax3 = fig.add_subplot(223)
    
    # 卫星预期位置（域中心）
    expected_center = mapping_info['expected_satellite_center']
    stl_center = mapping_info['stl_center_m']
    translation = mapping_info['required_translation']
    
    # 绘制位置关系
    positions = np.array([
        stl_center,
        expected_center,
        [0, 0, 0]  # 观测点中心
    ])
    
    labels = ['STL Center', 'Expected Center', 'Observation Center']
    colors = ['red', 'green', 'blue']
    
    ax3.scatter(positions[:, 0], positions[:, 2], c=colors, s=100)
    for i, label in enumerate(labels):
        ax3.annotate(label, (positions[i, 0], positions[i, 2]), 
                    xytext=(5, 5), textcoords='offset points')
    
    # 绘制平移向量
    ax3.arrow(stl_center[0], stl_center[2], 
             translation[0], translation[2],
             head_width=0.1, head_length=0.1, 
             fc='orange', ec='orange', label='Required translation')
    
    ax3.set_xlabel('X (m)')
    ax3.set_ylabel('Z (m)')
    ax3.set_title('Coordinate Mapping (X-Z plane)')
    ax3.grid(True, alpha=0.3)
    ax3.legend()
    
    # 4. 观测点配置验证
    ax4 = fig.add_subplot(224)
    
    # 显示观测点应该覆盖的区域
    satellite_size = np.array([2.8, 2.8, 1.0])
    observation_radius = np.linalg.norm(satellite_size) / 2 * 1.2  # 20% margin
    
    # 观测点中心在原点（域中心）
    obs_center = np.array([0, 0, 0])
    
    # 绘制观测范围
    circle = plt.Circle((obs_center[0], obs_center[2]), 
                         observation_radius, 
                         fill=False, color='purple', 
                         linestyle='--', label='Observation range')
    ax4.add_patch(circle)
    
    # 卫星位置（平移后）
    satellite_center = translation
    ax4.scatter(satellite_center[0], satellite_center[2], 
               c='red', s=100, marker='s', label='Satellite center')
    
    # 卫星边界
    satellite_rect = plt.Rectangle(
        (satellite_center[0] - satellite_size[0]/2, 
         satellite_center[2] - satellite_size[2]/2),
        satellite_size[0], satellite_size[2],
        fill=False, color='red', label='Satellite bounds'
    )
    ax4.add_patch(satellite_rect)
    
    ax4.set_xlabel('X (m)')
    ax4.set_ylabel('Z (m)')
    ax4.set_title('Observation Coverage Check')
    ax4.grid(True, alpha=0.3)
    ax4.legend()
    ax4.set_aspect('equal')
    
    plt.tight_layout()
    plt.savefig('stl_geometry_coordinate_analysis.png', dpi=300, bbox_inches='tight')
    plt.show()

def main():
    """Main analysis function"""
    print("🛰️ STL Geometry and Coordinate Mapping Analysis")
    print("=" * 60)
    
    # 文件路径
    stl_file = "weixing_v1.stl"
    
    if not os.path.exists(stl_file):
        print(f"❌ STL file not found: {stl_file}")
        return
    
    # 解析STL文件
    print("📋 Parsing STL file...")
    vertices, normals = parse_stl_ascii(stl_file)
    
    if vertices is None:
        print("❌ Failed to parse STL file")
        return
    
    # 分析几何
    stl_info = analyze_geometry(vertices, "Original STL Geometry Analysis")
    
    # FDTD域配置（来自仿真配置）
    domain_config = {
        'domain_size': [3.4, 3.4, 1.4],  # meters
        'satellite_size': [2.8, 2.8, 1.0]  # meters
    }
    
    # 检查坐标映射
    mapping_info = check_coordinate_mapping(stl_info, domain_config)
    
    # 可视化分析
    visualize_geometry_comparison(stl_info, mapping_info)
    
    # 总结报告
    print(f"\n{'='*60}")
    print("ANALYSIS SUMMARY")
    print(f"{'='*60}")
    
    print(f"STL Import Status: {'✓ CORRECT' if mapping_info['size_match'] else '✗ INCORRECT'}")
    print(f"Size Match: {'✓ PASS' if mapping_info['size_match'] else '✗ FAIL'}")
    print(f"Required Translation: {mapping_info['required_translation']} m")
    
    print(f"\nRecommendations:")
    if not mapping_info['size_match']:
        print("  ⚠️  Check STL units and scaling")
    
    print("  📍 Apply translation to STL geometry or adjust observation points")
    print("  🔍 Verify material assignment for PEC properties")
    print("  ⚙️  Ensure mesh generation includes satellite structure")
    
    print(f"\n📊 Analysis complete! Check stl_geometry_coordinate_analysis.png for visualization")

if __name__ == "__main__":
    main()