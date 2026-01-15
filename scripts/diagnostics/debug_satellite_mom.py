#!/usr/bin/env python3
"""
卫星MoM仿真详细诊断分析
详细检测CAD导入、网格剖分、平面波源、测量点和电磁计算的每个步骤
"""

import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
import os
import sys
import time

# 添加父目录到路径
sys.path.append(os.path.dirname(os.path.abspath(__file__)))

from satellite_mom_final_test import SatelliteMoMTester

class SatelliteMoMDebugger(SatelliteMoMTester):
    """卫星MoM仿真调试器 - 详细分析每个步骤"""
    
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.debug_info = {}
    
    def run_detailed_debug(self):
        """运行详细调试分析"""
        print("🔍 启动详细调试分析...")
        print("="*80)
        
        # 1. STL文件解析调试
        print("\n📁 1. STL文件解析调试")
        print("-" * 50)
        success = self.debug_stl_parsing()
        if not success:
            print("❌ STL解析失败，停止调试")
            return False
        
        # 2. 几何数据验证
        print("\n📐 2. 几何数据验证")
        print("-" * 50)
        success = self.debug_geometry_validation()
        if not success:
            print("❌ 几何验证失败")
            return False
        
        # 3. 网格剖分调试
        print("\n🔧 3. 网格剖分调试")
        print("-" * 50)
        success = self.debug_mesh_generation()
        if not success:
            print("❌ 网格剖分失败")
            return False
        
        # 4. RWG基函数调试
        print("\n⚡ 4. RWG基函数调试")
        print("-" * 50)
        success = self.debug_rwg_basis_functions()
        if not success:
            print("❌ RWG基函数创建失败")
            return False
        
        # 5. 平面波源调试
        print("\n🌊 5. 平面波源调试")
        print("-" * 50)
        success = self.debug_plane_wave_source()
        if not success:
            print("❌ 平面波源设置失败")
            return False
        
        # 6. 观测点设置调试
        print("\n📍 6. 观测点设置调试")
        print("-" * 50)
        success = self.debug_observation_points()
        if not success:
            print("❌ 观测点设置失败")
            return False
        
        # 7. MoM矩阵调试
        print("\n🧮 7. MoM矩阵调试")
        print("-" * 50)
        success = self.debug_mom_matrix()
        if not success:
            print("❌ MoM矩阵装配失败")
            return False
        
        # 8. 电磁场计算调试
        print("\n🌐 8. 电磁场计算调试")
        print("-" * 50)
        success = self.debug_electromagnetic_calculation()
        if not success:
            print("❌ 电磁场计算失败")
            return False
        
        # 9. 生成调试报告
        print("\n📊 9. 生成调试报告")
        print("-" * 50)
        self.generate_debug_report()
        
        return True
    
    def debug_stl_parsing(self):
        """STL文件解析调试"""
        print(f"   STL文件: {self.stl_file}")
        print(f"   文件存在: {os.path.exists(self.stl_file)}")
        
        if not os.path.exists(self.stl_file):
            print("   ❌ STL文件不存在")
            return False
        
        # 读取文件基本信息
        try:
            with open(self.stl_file, 'r') as f:
                content = f.read()
            
            print(f"   文件大小: {len(content)} 字符")
            
            # 计算面片数量
            facet_count = content.count('facet normal')
            print(f"   总面片数: {facet_count}")
            
            # 检查文件格式
            if content.startswith('solid'):
                print("   ✅ ASCII STL格式检测成功")
            else:
                print("   ❌ 未知的STL格式")
                return False
            
            # 显示前几个面片的信息
            lines = content.split('\n')[:50]
            print("   STL文件头部:")
            for i, line in enumerate(lines[:20]):
                print(f"     {i+1:2d}: {line}")
                if i > 15:
                    print("     ...")
                    break
            
            self.debug_info['stl'] = {
                'file_size': len(content),
                'facet_count': facet_count,
                'format': 'ASCII',
                'first_lines': lines[:10]
            }
            
            return True
            
        except Exception as e:
            print(f"   ❌ STL文件读取错误: {e}")
            return False
    
    def debug_geometry_validation(self):
        """几何数据验证"""
        print("   解析STL几何数据...")
        
        try:
            facets, vertices = self.parse_stl_file_professional(self.stl_file)
            
            if facets is None or vertices is None:
                print("   ❌ STL几何解析失败")
                return False
            
            print(f"   ✅ 解析成功:")
            print(f"      面片数量: {len(facets)}")
            print(f"      顶点数量: {len(vertices)}")
            
            # 检查几何范围
            if len(vertices) > 0:
                bounds = {
                    'x': [np.min(vertices[:, 0]), np.max(vertices[:, 0])],
                    'y': [np.min(vertices[:, 1]), np.max(vertices[:, 1])],
                    'z': [np.min(vertices[:, 2]), np.max(vertices[:, 2])]
                }
                
                print(f"   几何边界 (米):")
                print(f"      X: [{bounds['x'][0]:.3f}, {bounds['x'][1]:.3f}]")
                print(f"      Y: [{bounds['y'][0]:.3f}, {bounds['y'][1]:.3f}]")
                print(f"      Z: [{bounds['z'][0]:.3f}, {bounds['z'][1]:.3f}]")
                
                # 计算尺寸
                size_x = bounds['x'][1] - bounds['x'][0]
                size_y = bounds['y'][1] - bounds['y'][0]
                size_z = bounds['z'][1] - bounds['z'][0]
                
                print(f"   卫星尺寸: {size_x:.3f} × {size_y:.3f} × {size_z:.3f} 米")
                
                # 检查尺寸合理性
                expected_size = 2.8  # 2.8米
                if abs(size_x - expected_size) < 0.5 and abs(size_y - expected_size) < 0.5:
                    print("   ✅ 几何尺寸符合预期")
                else:
                    print(f"   ⚠️  几何尺寸与预期不符 (预期 ~{expected_size}米)")
                
                # 检查面片质量
                areas = [f['area'] for f in facets]
                min_area = np.min(areas)
                max_area = np.max(areas)
                avg_area = np.mean(areas)
                
                print(f"   面片面积统计:")
                print(f"      最小: {min_area:.6f} m²")
                print(f"      最大: {max_area:.6f} m²") 
                print(f"      平均: {avg_area:.6f} m²")
                
                if min_area < 1e-10:
                    print("   ⚠️  发现极小面积面片，可能影响计算精度")
                
                self.facets = facets
                self.vertices = vertices
                
                self.debug_info['geometry'] = {
                    'facets': len(facets),
                    'vertices': len(vertices),
                    'bounds': bounds,
                    'dimensions': [size_x, size_y, size_z],
                    'area_stats': {'min': min_area, 'max': max_area, 'avg': avg_area}
                }
                
                return True
            else:
                print("   ❌ 没有有效的顶点数据")
                return False
                
        except Exception as e:
            print(f"   ❌ 几何验证错误: {e}")
            import traceback
            traceback.print_exc()
            return False
    
    def debug_mesh_generation(self):
        """网格剖分调试"""
        print("   生成专业表面网格...")
        
        try:
            # 确保配置已加载
            if not hasattr(self, 'config'):
                print("   加载PFD配置...")
                self.config = self.parse_pfd_config_enhanced(self.pfd_file)
            
            # 应用坐标平移
            translation = np.array(self.config['geometry_translate']) * 1e-3  # mm to m
            translated_vertices = self.vertices + translation
            
            print(f"   坐标平移: {translation} 米")
            
            # 重新计算边界
            bounds = {
                'x': [np.min(translated_vertices[:, 0]), np.max(translated_vertices[:, 0])],
                'y': [np.min(translated_vertices[:, 1]), np.max(translated_vertices[:, 1])],
                'z': [np.min(translated_vertices[:, 2]), np.max(translated_vertices[:, 2])]
            }
            
            print(f"   平移后边界 (米):")
            print(f"      X: [{bounds['x'][0]:.3f}, {bounds['x'][1]:.3f}]")
            print(f"      Y: [{bounds['y'][0]:.3f}, {bounds['y'][1]:.3f}]")
            print(f"      Z: [{bounds['z'][0]:.3f}, {bounds['z'][1]:.3f}]")
            
            # 检查是否在合理范围内
            domain_size = 3.4  # 3.4米计算域
            if (abs(bounds['x'][0]) < domain_size/2 and abs(bounds['x'][1]) < domain_size/2 and
                abs(bounds['y'][0]) < domain_size/2 and abs(bounds['y'][1]) < domain_size/2 and
                abs(bounds['z'][0]) < domain_size/2 and abs(bounds['z'][1]) < domain_size/2):
                print("   ✅ 几何在计算域范围内")
            else:
                print("   ⚠️  几何可能超出计算域范围")
            
            self.debug_info['mesh'] = {
                'translation': translation,
                'bounds_after_translation': bounds,
                'num_triangles': len(self.facets)
            }
            
            return True
            
        except Exception as e:
            print(f"   ❌ 网格剖分错误: {e}")
            return False
    
    def debug_rwg_basis_functions(self):
        """RWG基函数调试"""
        print("   创建RWG基函数...")
        
        try:
            rwg_functions = self.calculate_rwg_basis_functions_optimized(self.facets)
            
            if not rwg_functions:
                print("   ❌ RWG基函数创建失败")
                return False
            
            print(f"   ✅ 创建了 {len(rwg_functions)} 个RWG基函数")
            
            # 检查RWG函数质量
            edge_lengths = [rwg['edge_length'] for rwg in rwg_functions]
            areas = []
            for rwg in rwg_functions:
                areas.extend(rwg['areas'])
            
            min_edge = np.min(edge_lengths)
            max_edge = np.max(edge_lengths)
            avg_edge = np.mean(edge_lengths)
            
            min_area = np.min(areas)
            max_area = np.max(areas)
            avg_area = np.mean(areas)
            
            print(f"   RWG基函数统计:")
            print(f"      边长范围: {min_edge:.6f} - {max_edge:.6f} m")
            print(f"      平均边长: {avg_edge:.6f} m")
            print(f"      面积范围: {min_area:.6f} - {max_area:.6f} m²")
            print(f"      平均面积: {avg_area:.6f} m²")
            
            # 检查是否有三角形数据
            triangles_count = 0
            for rwg in rwg_functions:
                if 'triangles' in rwg and len(rwg['triangles']) >= 2:
                    triangles_count += 1
            
            print(f"      有三角形数据的RWG: {triangles_count}/{len(rwg_functions)}")
            
            if triangles_count < len(rwg_functions) * 0.5:
                print("   ⚠️  超过50%的RWG函数缺少三角形数据")
            
            self.rwg_functions = rwg_functions
            
            self.debug_info['rwg'] = {
                'num_basis_functions': len(rwg_functions),
                'edge_length_stats': {'min': min_edge, 'max': max_edge, 'avg': avg_edge},
                'area_stats': {'min': min_area, 'max': max_area, 'avg': avg_area},
                'triangles_with_data': triangles_count
            }
            
            return True
            
        except Exception as e:
            print(f"   ❌ RWG基函数错误: {e}")
            return False
    
    def debug_plane_wave_source(self):
        """平面波源调试"""
        print("   设置平面波源...")
        
        try:
            # 从配置文件获取参数
            source_angle = self.config['source_angle']
            print(f"   入射角度: θ={source_angle[0]}°, φ={source_angle[1]}°, ψ={source_angle[2]}°")
            
            # 计算入射场
            V_incident = self.calculate_incident_field_rwg(self.rwg_functions, self.frequency, source_angle)
            
            print(f"   入射场向量长度: {len(V_incident)}")
            print(f"   入射场幅度范围: {np.min(np.abs(V_incident)):.2e} - {np.max(np.abs(V_incident)):.2e}")
            
            # 检查是否有合理的激励
            if np.max(np.abs(V_incident)) < 1e-12:
                print("   ❌ 入射场幅度太小，可能没有有效激励")
                return False
            
            # 检查入射场分布
            real_parts = np.real(V_incident)
            imag_parts = np.imag(V_incident)
            
            print(f"   实部范围: {np.min(real_parts):.2e} - {np.max(real_parts):.2e}")
            print(f"   虚部范围: {np.min(imag_parts):.2e} - {np.max(imag_parts):.2e}")
            
            # 计算入射方向向量
            theta, phi = np.radians(source_angle[0]), np.radians(source_angle[1])
            k_inc = np.array([
                np.sin(theta) * np.cos(phi),
                np.sin(theta) * np.sin(phi), 
                np.cos(theta)
            ])
            print(f"   入射波方向向量: {k_inc}")
            
            self.V_incident = V_incident
            
            self.debug_info['source'] = {
                'source_angle': source_angle,
                'incident_field_length': len(V_incident),
                'incident_field_range': [np.min(np.abs(V_incident)), np.max(np.abs(V_incident))],
                'k_vector': k_inc
            }
            
            return True
            
        except Exception as e:
            print(f"   ❌ 平面波源错误: {e}")
            return False
    
    def debug_observation_points(self):
        """观测点设置调试"""
        print("   创建观测点网格...")
        
        try:
            # 创建观测点
            obs_points = self.create_observation_grid_optimized()
            
            print(f"   观测点数量: {len(obs_points)}")
            
            if len(obs_points) == 0:
                print("   ❌ 没有创建观测点")
                return False
            
            # 检查观测点分布
            bounds = {
                'x': [np.min(obs_points[:, 0]), np.max(obs_points[:, 0])],
                'y': [np.min(obs_points[:, 1]), np.max(obs_points[:, 1])],
                'z': [np.min(obs_points[:, 2]), np.max(obs_points[:, 2])]
            }
            
            print(f"   观测点范围 (米):")
            print(f"      X: [{bounds['x'][0]:.3f}, {bounds['x'][1]:.3f}]")
            print(f"      Y: [{bounds['y'][0]:.3f}, {bounds['y'][1]:.3f}]")
            print(f"      Z: [{bounds['z'][0]:.3f}, {bounds['z'][1]:.3f}]")
            
            # 检查是否包围卫星
            if hasattr(self, 'vertices'):
                sat_bounds = {
                    'x': [np.min(self.vertices[:, 0]), np.max(self.vertices[:, 0])],
                    'y': [np.min(self.vertices[:, 1]), np.max(self.vertices[:, 1])],
                    'z': [np.min(self.vertices[:, 2]), np.max(self.vertices[:, 2])]
                }
                
                # 应用坐标平移
                translation = np.array(self.config['geometry_translate']) * 1e-3
                sat_bounds_translated = {
                    'x': [sat_bounds['x'][0] + translation[0], sat_bounds['x'][1] + translation[0]],
                    'y': [sat_bounds['y'][0] + translation[1], sat_bounds['y'][1] + translation[1]],
                    'z': [sat_bounds['z'][0] + translation[2], sat_bounds['z'][1] + translation[2]]
                }
                
                print(f"   卫星位置 (平移后):")
                print(f"      X: [{sat_bounds_translated['x'][0]:.3f}, {sat_bounds_translated['x'][1]:.3f}]")
                print(f"      Y: [{sat_bounds_translated['y'][0]:.3f}, {sat_bounds_translated['y'][1]:.3f}]")
                print(f"      Z: [{sat_bounds_translated['z'][0]:.3f}, {sat_bounds_translated['z'][1]:.3f}]")
                
                # 检查观测点是否包围卫星
                surrounds_satellite = (
                    bounds['x'][0] < sat_bounds_translated['x'][0] and bounds['x'][1] > sat_bounds_translated['x'][1] and
                    bounds['y'][0] < sat_bounds_translated['y'][0] and bounds['y'][1] > sat_bounds_translated['y'][1] and
                    bounds['z'][0] < sat_bounds_translated['z'][0] and bounds['z'][1] > sat_bounds_translated['z'][1]
                )
                
                if surrounds_satellite:
                    print("   ✅ 观测点网格包围卫星")
                else:
                    print("   ⚠️  观测点网格未完全包围卫星")
            
            self.obs_points = obs_points
            
            self.debug_info['observation'] = {
                'num_points': len(obs_points),
                'bounds': bounds,
                'surrounds_satellite': surrounds_satellite if hasattr(self, 'vertices') else 'unknown'
            }
            
            return True
            
        except Exception as e:
            print(f"   ❌ 观测点设置错误: {e}")
            return False
    
    def debug_mom_matrix(self):
        """MoM矩阵调试"""
        print("   装配MoM阻抗矩阵...")
        
        try:
            start_time = time.time()
            Z_matrix = self.calculate_mom_impedance_matrix_rwg(self.rwg_functions, self.frequency)
            matrix_time = time.time() - start_time
            
            if Z_matrix is None:
                print("   ❌ MoM矩阵装配失败")
                return False
            
            print(f"   ✅ MoM矩阵装配成功")
            print(f"   矩阵大小: {Z_matrix.shape}")
            print(f"   计算时间: {matrix_time:.2f} 秒")
            
            # 检查矩阵性质
            if np.any(np.isnan(Z_matrix)):
                print("   ❌ 矩阵包含NaN值")
                return False
            
            if np.any(np.isinf(Z_matrix)):
                print("   ❌ 矩阵包含无穷大值")
                return False
            
            # 检查条件数
            try:
                condition_number = np.linalg.cond(Z_matrix)
                print(f"   矩阵条件数: {condition_number:.2e}")
                
                if condition_number > 1e15:
                    print("   ⚠️  条件数过大，可能影响求解精度")
                elif condition_number > 1e10:
                    print("   ⚠️  条件数较大")
                else:
                    print("   ✅ 条件数合理")
                    
            except Exception as e:
                print(f"   ⚠️  条件数计算失败: {e}")
            
            # 检查矩阵元素范围
            element_range = [np.min(np.abs(Z_matrix)), np.max(np.abs(Z_matrix))]
            print(f"   矩阵元素幅度范围: {element_range[0]:.2e} - {element_range[1]:.2e}")
            
            if element_range[1] / element_range[0] > 1e10:
                print("   ⚠️  矩阵元素动态范围过大")
            
            self.Z_matrix = Z_matrix
            
            self.debug_info['matrix'] = {
                'shape': Z_matrix.shape,
                'computation_time': matrix_time,
                'condition_number': condition_number if 'condition_number' in locals() else 'unknown',
                'element_range': element_range,
                'has_nan': np.any(np.isnan(Z_matrix)),
                'has_inf': np.any(np.isinf(Z_matrix))
            }
            
            return True
            
        except Exception as e:
            print(f"   ❌ MoM矩阵错误: {e}")
            return False
    
    def debug_electromagnetic_calculation(self):
        """电磁场计算调试"""
        print("   求解表面电流...")
        
        try:
            # 求解表面电流
            surface_currents = self.calculate_surface_currents_from_stl(self.Z_matrix, self.V_incident)
            
            if surface_currents is None:
                print("   ❌ 表面电流求解失败")
                return False
            
            print(f"   ✅ 表面电流求解成功")
            print(f"   电流向量长度: {len(surface_currents)}")
            print(f"   电流幅度范围: {np.min(np.abs(surface_currents)):.2e} - {np.max(np.abs(surface_currents)):.2e}")
            
            # 检查电流分布
            if np.max(np.abs(surface_currents)) < 1e-15:
                print("   ❌ 表面电流幅度太小，可能没有有效激励")
                return False
            
            # 计算散射场
            print("   计算散射电磁场...")
            scattered_fields = self.calculate_scattered_field_rwg(
                self.rwg_functions, surface_currents, self.obs_points, self.frequency
            )
            
            if scattered_fields is None:
                print("   ❌ 散射场计算失败")
                return False
            
            print(f"   ✅ 散射场计算成功")
            print(f"   散射场数据点数: {len(scattered_fields)}")
            print(f"   散射场幅度范围: {np.min(np.abs(scattered_fields)):.2e} - {np.max(np.abs(scattered_fields)):.2e}")
            
            # 验证散射场合理性
            incident_field = self.calculate_incident_field_at_points(self.obs_points)
            max_incident = np.max(np.abs(incident_field))
            max_scattered = np.max(np.abs(scattered_fields))
            scattering_ratio = max_scattered / max_incident if max_incident > 0 else 0
            
            print(f"   入射场最大幅度: {max_incident:.2e} V/m")
            print(f"   散射场最大幅度: {max_scattered:.2e} V/m")
            print(f"   散射比例: {scattering_ratio:.2%}")
            
            if scattering_ratio < 0.001:
                print("   ❌ 散射比例过小，可能没有真实的电磁相互作用")
                return False
            elif scattering_ratio > 10:
                print("   ⚠️  散射比例过大，可能需要检查计算")
            else:
                print("   ✅ 散射比例合理")
            
            self.surface_currents = surface_currents
            self.scattered_fields = scattered_fields
            
            self.debug_info['em_calculation'] = {
                'surface_currents_length': len(surface_currents),
                'current_range': [np.min(np.abs(surface_currents)), np.max(np.abs(surface_currents))],
                'scattered_fields_length': len(scattered_fields),
                'field_range': [np.min(np.abs(scattered_fields)), np.max(np.abs(scattered_fields))],
                'scattering_ratio': scattering_ratio,
                'max_incident_field': max_incident,
                'max_scattered_field': max_scattered
            }
            
            return True
            
        except Exception as e:
            print(f"   ❌ 电磁场计算错误: {e}")
            import traceback
            traceback.print_exc()
            return False
    
    def generate_debug_report(self):
        """生成调试报告"""
        print("\n📊 调试分析报告")
        print("="*80)
        
        print("\n🔍 问题诊断:")
        print("-" * 50)
        
        # 分析潜在问题
        issues = []
        warnings = []
        
        # 检查几何问题
        if 'geometry' in self.debug_info:
            geom = self.debug_info['geometry']
            if geom['vertices'] < 100:
                issues.append(f"顶点数量过少 ({geom['vertices']})")
            if geom['area_stats']['min'] < 1e-10:
                issues.append("存在极小面积面片")
        
        # 检查RWG问题
        if 'rwg' in self.debug_info:
            rwg = self.debug_info['rwg']
            if rwg['num_basis_functions'] < 10:
                issues.append(f"RWG基函数数量过少 ({rwg['num_basis_functions']})")
            if rwg['triangles_with_data'] < rwg['num_basis_functions'] * 0.5:
                issues.append("超过50%的RWG缺少三角形数据")
        
        # 检查源问题
        if 'source' in self.debug_info:
            source = self.debug_info['source']
            if source['incident_field_range'][1] < 1e-12:
                issues.append("入射场幅度太小")
        
        # 检查观测点问题
        if 'observation' in self.debug_info:
            obs = self.debug_info['observation']
            if obs['num_points'] < 100:
                issues.append(f"观测点数量过少 ({obs['num_points']})")
            if obs['surrounds_satellite'] == False:
                issues.append("观测点未包围卫星")
        
        # 检查矩阵问题
        if 'matrix' in self.debug_info:
            matrix = self.debug_info['matrix']
            if matrix['has_nan']:
                issues.append("MoM矩阵包含NaN值")
            if matrix['has_inf']:
                issues.append("MoM矩阵包含无穷大值")
            if 'condition_number' in matrix and matrix['condition_number'] > 1e15:
                issues.append("矩阵条件数过大")
        
        # 检查电磁计算问题
        if 'em_calculation' in self.debug_info:
            em = self.debug_info['em_calculation']
            if em['scattering_ratio'] < 0.001:
                issues.append("散射比例过小，无真实电磁相互作用")
            if em['current_range'][1] < 1e-12:
                issues.append("表面电流幅度太小")
        
        # 输出问题
        if issues:
            print("❌ 发现的关键问题:")
            for issue in issues:
                print(f"   • {issue}")
        else:
            print("✅ 未发现关键问题")
        
        if warnings:
            print("\n⚠️  发现的警告:")
            for warning in warnings:
                print(f"   • {warning}")
        
        # 建议
        print("\n💡 建议:")
        if issues:
            print("   1. 检查STL文件是否正确导入到计算域")
            print("   2. 验证RWG基函数是否正确创建")
            print("   3. 确认平面波源是否正确设置")
            print("   4. 检查观测点是否包围卫星结构")
            print("   5. 验证MoM矩阵计算是否正确")
        else:
            print("   ✅ 所有检查通过，仿真设置正确")
        
        # 保存调试信息到文件
        try:
            import json
            debug_file = 'satellite_mom_debug_report.json'
            with open(debug_file, 'w') as f:
                json.dump(self.debug_info, f, indent=2, default=str)
            print(f"\n📄 调试报告已保存到: {debug_file}")
        except Exception as e:
            print(f"\n⚠️  保存调试报告失败: {e}")

def main():
    """主函数"""
    print("启动卫星MoM仿真详细调试...")
    print("="*80)
    
    # 创建调试器实例
    debugger = SatelliteMoMDebugger(
        stl_file='tests/test_hpm/weixing_v1.stl',
        pfd_file='tests/test_hpm/weixing_v1_case.pfd',
        max_facets=2000,
        frequency=10e9
    )
    
    # 运行详细调试
    success = debugger.run_detailed_debug()
    
    if success:
        print("\n🎉 调试分析完成！")
    else:
        print("\n❌ 调试分析发现问题")
    
    return 0 if success else 1

if __name__ == "__main__":
    sys.exit(main())