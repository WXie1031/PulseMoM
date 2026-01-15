import numpy as np
import sys
sys.path.append('.')

# 快速测试RWG激励计算
from fixed_satellite_mom_peec_final import FixedSatelliteMoMPEECTester

# 创建完整求解器实例
solver = FixedSatelliteMoMPEECTester('satellite.stl', frequency=1e9)

# 运行完整仿真以加载数据
print('运行仿真以加载几何数据...')
try:
    success = solver.run_complete_simulation_with_fixes()
    if success:
        print('✅ 仿真运行成功，数据已加载')
    else:
        print('❌ 仿真运行失败')
except Exception as e:
    print(f'仿真运行错误: {e}')
    print('继续分析现有数据...')

# 分析卫星几何
print('分析卫星几何...')
print(f'求解器类型: {type(solver)}')
print(f'频率: {solver.frequency/1e9} GHz')
print(f'波长: {solver.wavelength:.3f} m')

# 测试平面波激励计算
print('\\n测试平面波激励计算...')

# 定义测试参数
incident_direction = np.array([0, 1, 0])  # Y方向入射
polarization = np.array([1, 0, 0])  # X方向极化
k = solver.mom_solver.k

print(f'波数 k = {k:.3f} rad/m')
print(f'入射方向: {incident_direction}')
print(f'极化方向: {polarization}')

# 测试几个空间点的电场
test_points = [
    np.array([0, 0, 0]),
    np.array([0.1, 0, 0]),
    np.array([0, 0.1, 0]),
    np.array([0, 0, 0.1])
]

print('\\n测试点电场计算:')
for i, point in enumerate(test_points):
    phase = -1j * k * np.dot(point, incident_direction)
    E_field = np.exp(phase) * polarization
    print(f'  点{i+1} {point}: E = {E_field}')

# 测试RWG基函数耦合
print('\\n测试RWG基函数耦合...')

# 创建测试RWG函数
test_rwg = {
    'id': 0,
    'plus_triangle': {
        'vertices': [0, 1, 2],  # 使用顶点索引
        'area': 0.01
    },
    'minus_triangle': {
        'vertices': [0, 2, 3],  # 使用顶点索引  
        'area': 0.01
    }
}

# 如果有真实顶点数据，使用真实数据
if hasattr(solver, 'vertices') and solver.vertices:
    vertices = np.array(solver.vertices)
    print(f'找到 {len(vertices)} 个顶点')
    print(f'顶点范围: X[{vertices[:,0].min():.3f}, {vertices[:,0].max():.3f}]')
    print(f'          Y[{vertices[:,1].min():.3f}, {vertices[:,1].max():.3f}]')
    print(f'          Z[{vertices[:,2].min():.3f}, {vertices[:,2].max():.3f}]')
    
    # 计算质心
    centroid = np.mean(vertices, axis=0)
    print(f'质心: {centroid}')
    
    # 测试前几个RWG函数
    if hasattr(solver, 'rwg_functions') and solver.rwg_functions:
        print(f'\\n找到 {len(solver.rwg_functions)} 个RWG函数')
        
        for i in range(min(3, len(solver.rwg_functions))):
            rwg = solver.rwg_functions[i]
            print(f'\\nRWG {i}:')
            
            # 获取三角形数据
            plus_tri = rwg.get('plus_triangle', {})
            minus_tri = rwg.get('minus_triangle', {})
            
            plus_vertices = plus_tri.get('vertices', [])
            minus_vertices = minus_tri.get('vertices', [])
            
            print(f'  正三角形顶点: {plus_vertices}')
            print(f'  负三角形顶点: {minus_vertices}')
            
            if plus_vertices and len(plus_vertices) == 3:
                # 计算几何特性
                try:
                    v0_coord = np.array(solver.vertices[plus_vertices[0]])
                    v1_coord = np.array(solver.vertices[plus_vertices[1]])
                    v2_coord = np.array(solver.vertices[plus_vertices[2]])
                    
                    # 计算法向量
                    edge1 = v1_coord - v0_coord
                    edge2 = v2_coord - v0_coord
                    normal = np.cross(edge1, edge2)
                    area = 0.5 * np.linalg.norm(normal)
                    
                    print(f'  正三角形面积: {area:.6f}')
                    print(f'  正三角形法向量: {normal / (np.linalg.norm(normal) + 1e-10)}')
                    
                    # 计算质心
                    tri_centroid = (v0_coord + v1_coord + v2_coord) / 3
                    print(f'  正三角形质心: {tri_centroid}')
                    
                    # 计算在该点的平面波电场
                    phase = -1j * k * np.dot(tri_centroid, incident_direction)
                    E_at_centroid = np.exp(phase) * polarization
                    print(f'  质心处电场: {E_at_centroid}')
                    
                    # 计算简单耦合（点积）
                    # 假设RWG基函数方向近似为法向量方向
                    coupling = np.dot(normal / (np.linalg.norm(normal) + 1e-10), polarization)
                    print(f'  简单耦合强度: {coupling:.6f}')
                    
                except Exception as e:
                    print(f'  计算错误: {e}')
else:
    print('没有找到顶点数据')

# 分析最佳极化方向
print('\n分析最佳极化方向...')

# 基于卫星几何尺寸分析
if hasattr(solver, 'vertices') and solver.vertices:
    vertices = np.array(solver.vertices)
    
    # 计算边界框
    min_coords = vertices.min(axis=0)
    max_coords = vertices.max(axis=0)
    dimensions = max_coords - min_coords
    
    print(f'卫星尺寸: X={dimensions[0]:.3f}m, Y={dimensions[1]:.3f}m, Z={dimensions[2]:.3f}m')
    
    # 找出最大尺寸方向
    max_dim_idx = np.argmax(dimensions)
    axis_names = ['X', 'Y', 'Z']
    
    print(f'最大尺寸方向: {axis_names[max_dim_idx]}轴 ({dimensions[max_dim_idx]:.3f}m)')
    
    # 建议极化方向
    if max_dim_idx == 0:  # X方向最大
        suggested_pol = np.array([0, 1, 0])  # Y方向极化
        suggested_inc = np.array([0, 0, 1])  # Z方向入射
    elif max_dim_idx == 1:  # Y方向最大
        suggested_pol = np.array([1, 0, 0])  # X方向极化
        suggested_inc = np.array([0, 0, 1])  # Z方向入射
    else:  # Z方向最大
        suggested_pol = np.array([1, 0, 0])  # X方向极化
        suggested_inc = np.array([0, 1, 0])  # Y方向入射
    
    print(f'建议极化方向: {suggested_pol}')
    print(f'建议入射方向: {suggested_inc}')
    
    # 验证建议方向的耦合
    print('\n验证建议方向的耦合...')
    
    # 计算一些三角形的平均耦合
    total_coupling = 0
    valid_triangles = 0
    
    for i in range(min(10, len(solver.rwg_functions))):
        rwg = solver.rwg_functions[i]
        plus_tri = rwg.get('plus_triangle', {})
        plus_vertices = plus_tri.get('vertices', [])
        
        if plus_vertices and len(plus_vertices) == 3:
            try:
                v0_coord = np.array(solver.vertices[plus_vertices[0]])
                v1_coord = np.array(solver.vertices[plus_vertices[1]])
                v2_coord = np.array(solver.vertices[plus_vertices[2]])
                
                # 计算法向量
                edge1 = v1_coord - v0_coord
                edge2 = v2_coord - v0_coord
                normal = np.cross(edge1, edge2)
                normal = normal / (np.linalg.norm(normal) + 1e-10)
                
                # 计算耦合
                coupling = abs(np.dot(normal, suggested_pol))
                total_coupling += coupling
                valid_triangles += 1
                
            except:
                continue
    
    if valid_triangles > 0:
        avg_coupling = total_coupling / valid_triangles
        print(f'平均耦合强度: {avg_coupling:.6f}')
        print(f'预计激励强度: {avg_coupling:.6f}')
    else:
        print('无法计算有效耦合')