#!/usr/bin/env python3
"""
卫星MoM/PEEC电磁仿真最终统一测试文件 - 增强C求解器集成版
Satellite MoM/PEEC Electromagnetic Simulation Final Unified Test - Enhanced C Solver Integration

该文件整合了所有测试功能，提供完整的卫星电磁散射仿真验证
包括：
- 原生C求解器接口 (ctypes/subprocess)
- 专业STL几何处理与单位转换
- C mesh_engine网格生成替代Python实现
- 真实MoM阻抗矩阵计算与奇异性处理
- 完整PEEC等效电路建模
- Python vs C求解器结果对比
- 分层介质支持
- 专业场输出与可视化
"""

import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
import re
import time
import os
import sys
import struct
import json
import subprocess
import tempfile
from scipy.fft import fft, ifft, fftfreq
from scipy.signal import chirp, gaussian
from scipy.sparse import csr_matrix, lil_matrix
from scipy.sparse.linalg import gmres, bicgstab
from typing import Dict, List, Optional, Tuple, Any
import ctypes
import platform

# 导入C求解器接口
try:
    from c_solver_interface import CSolverInterface
    from c_executable_interface import CExecutableInterface
    C_SOLVER_AVAILABLE = True
except ImportError as e:
    print(f"警告: 无法导入C求解器接口: {e}")
    C_SOLVER_AVAILABLE = False
    CSolverInterface = None
    CExecutableInterface = None

# 专业电磁仿真常量
class EMConstants:
    """电磁仿真物理常量"""
    MU_0 = 4 * np.pi * 1e-7  # 真空磁导率
    EPSILON_0 = 8.854187817e-12  # 真空介电常数
    C = 299792458.0  # 光速
    ETA_0 = 376.73  # 真空波阻抗

# 专业材料数据库
class MaterialDatabase:
    """专业电磁材料数据库"""
    
    MATERIALS = {
        'PEC': {'epsr': 1.0, 'mur': 1.0, 'sigma': 1e20, 'name': 'Perfect Electric Conductor'},
        'ALUMINUM': {'epsr': 1.0, 'mur': 1.0, 'sigma': 3.5e7, 'name': 'Aluminum'},
        'COPPER': {'epsr': 1.0, 'mur': 1.0, 'sigma': 5.8e7, 'name': 'Copper'},
        'STEEL': {'epsr': 1.0, 'mur': 100.0, 'sigma': 1.0e6, 'name': 'Steel'},
        'CARBON_FIBER': {'epsr': 3.5, 'mur': 1.0, 'sigma': 1e4, 'name': 'Carbon Fiber Composite'},
        'SOLAR_PANEL': {'epsr': 11.7, 'mur': 1.0, 'sigma': 1e3, 'name': 'Solar Panel Substrate'}
    }
    
    @classmethod
    def get_material(cls, material_name):
        """获取材料属性"""
        material_name_upper = material_name.upper()
        if material_name_upper in cls.MATERIALS:
            return cls.MATERIALS[material_name_upper].copy()
        else:
            # 默认返回PEC
            return cls.MATERIALS['PEC'].copy()
    
    @classmethod
    def is_conductor(cls, material):
        """判断是否为导体"""
        return material['sigma'] > 1e6  # 电导率大于1e6 S/m认为是导体

# 原生C求解器接口管理器
class NativeCSolverManager:
    """原生C求解器管理器 - 提供统一的C求解器调用接口"""
    
    def __init__(self, src_dir: str = None):
        self.src_dir = src_dir or os.path.join(os.path.dirname(__file__), 'src')
        self.build_dir = os.path.join(self.src_dir, 'build')
        
        # 初始化C求解器接口
        self.ctypes_interface = None
        self.subprocess_interface = None
        self.c_solvers_available = False
        
        # 尝试初始化C求解器
        self._initialize_c_solvers()
        
        # 结果缓存
        self.result_cache = {}
        
    def _initialize_c_solvers(self):
        """初始化C求解器"""
        print("正在初始化原生C求解器...")
        
        if not C_SOLVER_AVAILABLE:
            print("⚠️ C求解器接口不可用")
            return
        
        try:
            # 初始化ctypes接口
            self.ctypes_interface = CSolverInterface(self.src_dir)
            
            # 初始化subprocess接口
            self.subprocess_interface = CExecutableInterface(self.src_dir)
            
            # 测试C求解器可用性
            if self._test_c_solvers():
                self.c_solvers_available = True
                print("✅ 原生C求解器初始化成功")
            else:
                print("⚠️ C求解器测试失败，将使用Python实现")
                
        except Exception as e:
            print(f"❌ C求解器初始化失败: {e}")
            self.c_solvers_available = False
    
    def _test_c_solvers(self) -> bool:
        """测试C求解器功能"""
        try:
            # 测试MoM求解器
            mom_test = self.call_mom_solver({'test': True, 'frequency': 1e9})
            if not mom_test.get('success', False):
                print("⚠️ C MoM求解器测试失败")
                return False
            
            # 测试PEEC求解器
            peec_test = self.call_peec_solver({'test': True, 'frequency': 1e9})
            if not peec_test.get('success', False):
                print("⚠️ C PEEC求解器测试失败")
                return False
            
            # 测试网格引擎
            mesh_test = self.call_mesh_engine({'test': True, 'simple_geometry': True})
            if not mesh_test.get('success', False):
                print("⚠️ C网格引擎测试失败")
                return False
            
            return True
            
        except Exception as e:
            print(f"C求解器测试异常: {e}")
            return False
    
    def call_mom_solver(self, config: Dict[str, Any]) -> Dict[str, Any]:
        """调用C MoM求解器"""
        if not self.c_solvers_available:
            return {'success': False, 'error': 'C solvers not available'}
        
        cache_key = f"mom_{hash(str(sorted(config.items())))}"
        if cache_key in self.result_cache:
            return self.result_cache[cache_key]
        
        try:
            # 优先使用subprocess接口（更稳定）
            if hasattr(self.subprocess_interface, 'run_mom_solver'):
                result = self.subprocess_interface.run_mom_solver(config)
            else:
                # 回退到ctypes接口
                result = self.ctypes_interface.solve_mom(config)
            
            self.result_cache[cache_key] = result
            return result
            
        except Exception as e:
            print(f"C MoM求解器调用失败: {e}")
            return {'success': False, 'error': str(e)}
    
    def call_peec_solver(self, config: Dict[str, Any]) -> Dict[str, Any]:
        """调用C PEEC求解器"""
        if not self.c_solvers_available:
            return {'success': False, 'error': 'C solvers not available'}
        
        cache_key = f"peec_{hash(str(sorted(config.items())))}"
        if cache_key in self.result_cache:
            return self.result_cache[cache_key]
        
        try:
            # 优先使用subprocess接口
            if hasattr(self.subprocess_interface, 'run_peec_solver'):
                result = self.subprocess_interface.run_peec_solver(config)
            else:
                # 回退到ctypes接口
                result = self.ctypes_interface.solve_peec(config)
            
            self.result_cache[cache_key] = result
            return result
            
        except Exception as e:
            print(f"C PEEC求解器调用失败: {e}")
            return {'success': False, 'error': str(e)}
    
    def call_mesh_engine(self, config: Dict[str, Any]) -> Dict[str, Any]:
        """调用C网格引擎"""
        if not self.c_solvers_available:
            return {'success': False, 'error': 'C solvers not available'}
        
        cache_key = f"mesh_{hash(str(sorted(config.items())))}"
        if cache_key in self.result_cache:
            return self.result_cache[cache_key]
        
        try:
            # 优先使用subprocess接口
            if hasattr(self.subprocess_interface, 'run_mesh_engine'):
                result = self.subprocess_interface.run_mesh_engine(config)
            else:
                # 回退到ctypes接口
                result = self.ctypes_interface.call_mesh_engine(config.get('stl_file'), config)
            
            self.result_cache[cache_key] = result
            return result
            
        except Exception as e:
            print(f"C网格引擎调用失败: {e}")
            return {'success': False, 'error': str(e)}
    
    def is_available(self) -> bool:
        """检查C求解器是否可用"""
        return self.c_solvers_available

# 专业STL解析器
class ProfessionalSTLParser:
    """专业STL文件解析器 - 处理单位转换和几何验证"""
    
    def __init__(self, target_scale=2.8, stl_units='mm'):
        self.target_scale = target_scale  # 目标卫星尺寸2.8米
        self.stl_units = stl_units
        self.vertices = []
        self.facets = []
        self.normals = []
        self.material_regions = []
        
    def parse_stl_file(self, filename, max_facets=None):
        """专业STL文件解析"""
        
        print(f"   专业STL解析: {filename}")
        
        try:
            with open(filename, 'r', encoding='utf-8', errors='ignore') as f:
                content = f.read()
            
            # 检测文件类型 (ASCII vs Binary)
            if content.startswith('solid'):
                return self._parse_ascii_stl(content, max_facets)
            else:
                return self._parse_binary_stl(filename, max_facets)
                
        except Exception as e:
            print(f"   ❌ STL解析失败: {e}")
            return False
    
    def _parse_ascii_stl(self, content, max_facets):
        """解析ASCII STL文件"""
        
        lines = content.strip().split('\n')
        
        # 统计总面片数
        total_facets = sum(1 for line in lines if line.strip().startswith('facet normal'))
        print(f"   STL总面片数: {total_facets}")
        
        # 计算采样
        if max_facets and total_facets > max_facets:
            sampling_ratio = max_facets / total_facets
            sampling_step = max(1, int(1.0 / sampling_ratio))
            print(f"   采样比例: {sampling_ratio:.1%}, 步长: {sampling_step}")
        else:
            sampling_step = 1
        
        # 解析几何数据
        current_facet = None
        facet_count = 0
        parsed_facets = 0
        
        for line in lines:
            line = line.strip()
            
            if line.startswith('facet normal'):
                # 开始新的面片
                facet_count += 1
                if facet_count % sampling_step == 0:
                    current_facet = {'normal': [], 'vertices': []}
                    parts = line.split()
                    if len(parts) >= 5:
                        current_facet['normal'] = [float(parts[2]), float(parts[3]), float(parts[4])]
                    parsed_facets += 1
                
            elif line.startswith('vertex') and current_facet is not None:
                # 添加顶点
                parts = line.split()
                if len(parts) >= 4:
                    vertex = [float(parts[1]), float(parts[2]), float(parts[3])]
                    current_facet['vertices'].append(vertex)
                
            elif line.startswith('endfacet') and current_facet is not None:
                # 结束当前面片
                if len(current_facet.get('vertices', [])) == 3:
                    self._add_facet(current_facet)
                current_facet = None
        
        print(f"   解析面片数: {parsed_facets}")
        print(f"   最终顶点数: {len(self.vertices)}, 面片数: {len(self.facets)}")
        return self._post_process_geometry()
    
    def _parse_binary_stl(self, filename, max_facets):
        """解析二进制STL文件"""
        
        with open(filename, 'rb') as f:
            # 跳过头部
            f.read(80)
            
            # 读取面片数
            num_facets = struct.unpack('<I', f.read(4))[0]
            print(f"   二进制STL面片数: {num_facets}")
            
            # 采样设置
            if max_facets and num_facets > max_facets:
                sampling_step = max(1, num_facets // max_facets)
            else:
                sampling_step = 1
            
            # 读取面片数据
            for i in range(num_facets):
                if i % sampling_step == 0:
                    # 读取法向量
                    normal = struct.unpack('<fff', f.read(12))
                    
                    # 读取三个顶点
                    vertices = []
                    for j in range(3):
                        vertex = struct.unpack('<fff', f.read(12))
                        vertices.append(list(vertex))
                    
                    # 读取属性字节数
                    f.read(2)
                    
                    self._add_facet({'normal': list(normal), 'vertices': vertices})
                else:
                    # 跳过这个面片
                    f.read(50)
            
            return self._post_process_geometry()
    
    def _add_facet(self, facet):
        """添加面片并处理顶点"""
        
        # 单位转换
        if self.stl_units == 'mm':
            scale_factor = 1e-3
        else:
            scale_factor = 1.0
        
        # 转换顶点坐标
        converted_vertices = []
        for vertex in facet['vertices']:
            converted_vertex = [coord * scale_factor for coord in vertex]
            converted_vertices.append(converted_vertex)
        
        # 添加到列表
        self.facets.append(converted_vertices)
        self.normals.append(facet['normal'])
        
        # 添加到顶点列表（后续去重）
        for vertex in converted_vertices:
            self.vertices.append(vertex)
    
    def _post_process_geometry(self):
        """后处理几何数据 - 去重、缩放、验证"""
        
        print(f"   后处理几何数据: {len(self.vertices)} 顶点, {len(self.facets)} 面片")
        
        if not self.vertices or len(self.vertices) == 0:
            print("   ❌ 没有解析到顶点数据")
            return False
        
        # 转换为numpy数组
        vertices_array = np.array(self.vertices)
        
        # 计算原始边界
        min_coords = np.min(vertices_array, axis=0)
        max_coords = np.max(vertices_array, axis=0)
        original_size = max_coords - min_coords
        
        print(f"   原始几何尺寸: {original_size[0]:.3f} × {original_size[1]:.3f} × {original_size[2]:.3f} m")
        
        # 计算额外缩放以达到目标尺寸
        current_max_size = np.max(original_size)
        if current_max_size > 0:
            additional_scale = self.target_scale / current_max_size
        else:
            additional_scale = 1.0
        
        print(f"   额外缩放因子: {additional_scale:.2f}")
        
        # 应用缩放
        scaled_vertices = vertices_array * additional_scale
        
        # 重新计算边界
        self.vertices = scaled_vertices.tolist()
        self.facets = [[[v[0]*additional_scale, v[1]*additional_scale, v[2]*additional_scale] 
                       for v in facet] for facet in self.facets]
        
        # 更新边界信息
        self.bounds = {
            'min': np.min(scaled_vertices, axis=0).tolist(),
            'max': np.max(scaled_vertices, axis=0).tolist(),
            'center': np.mean(scaled_vertices, axis=0).tolist(),
            'size': (np.max(scaled_vertices, axis=0) - np.min(scaled_vertices, axis=0)).tolist()
        }
        
        print(f"   最终几何尺寸: {self.bounds['size'][0]:.3f} × {self.bounds['size'][1]:.3f} × {self.bounds['size'][2]:.3f} m")
        print(f"   几何中心: {self.bounds['center']}")
        
        return True

# C网格引擎接口
class CMeshEngineInterface:
    """C网格引擎接口 - 替代Python网格生成"""
    
    def __init__(self, c_solver_manager: NativeCSolverManager):
        self.c_solver_manager = c_solver_manager
        
    def generate_mesh_with_c_engine(self, stl_file: str, frequency: float, 
                                    target_edge_length: float = None) -> Dict[str, Any]:
        """使用C网格引擎生成网格"""
        
        if not self.c_solver_manager.is_available():
            print("⚠️ C网格引擎不可用，回退到Python实现")
            return None
        
        # 计算目标边长（如果未提供）
        if target_edge_length is None:
            wavelength = EMConstants.C / frequency
            target_edge_length = wavelength / 10  # λ/10规则
        
        print(f"   调用C网格引擎: {stl_file}")
        print(f"   目标边长: {target_edge_length:.4f} m")
        
        # 准备C网格引擎配置
        mesh_config = {
            'stl_file': stl_file,
            'frequency': frequency,
            'target_edge_length': target_edge_length,
            'mesh_quality': 'high',
            'preserve_features': True,
            'optimize_mesh': True,
            'min_angle': 15.0,  # 最小角度限制
            'max_aspect_ratio': 10.0,  # 最大长宽比
            'generate_normals': True,
            'validate_geometry': True
        }
        
        # 调用C网格引擎
        result = self.c_solver_manager.call_mesh_engine(mesh_config)
        
        if result.get('success', False):
            print(f"   ✅ C网格引擎成功生成网格")
            print(f"      顶点数: {result.get('num_vertices', 0)}")
            print(f"      三角形数: {result.get('num_triangles', 0)}")
            print(f"      网格质量: {result.get('mesh_quality', 'unknown')}")
            
            return {
                'vertices': result.get('vertices', []),
                'triangles': result.get('triangles', []),
                'normals': result.get('normals', []),
                'mesh_quality_metrics': result.get('quality_metrics', {}),
                'generation_time': result.get('generation_time', 0.0),
                'c_engine_used': True
            }
        else:
            print(f"   ⚠️ C网格引擎失败: {result.get('error', 'unknown error')}")
            return None

# C MoM求解器接口
class CMoMSolverInterface:
    """C MoM求解器接口"""
    
    def __init__(self, c_solver_manager: NativeCSolverManager):
        self.c_solver_manager = c_solver_manager
        
    def calculate_impedance_matrix_with_c_solver(self, rwg_functions: List[Dict], 
                                                 frequency: float, 
                                                 material_properties: Dict) -> Optional[np.ndarray]:
        """使用C MoM求解器计算阻抗矩阵"""
        
        if not self.c_solver_manager.is_available():
            print("⚠️ C MoM求解器不可用，回退到Python实现")
            return None
        
        print(f"   调用C MoM求解器计算阻抗矩阵 ({len(rwg_functions)} × {len(rwg_functions)})...")
        
        # 准备C MoM求解器配置
        mom_config = {
            'frequency': frequency,
            'basis_functions': rwg_functions,
            'material_properties': material_properties,
            'solver_type': 'fullwave',
            'integration_order': 4,
            'singularity_handling': 'analytical',
            'matrix_solver': 'lu',  # 或 'iterative'
            'tolerance': 1e-6,
            'parallel': True,
            'cache_enabled': True
        }
        
        # 调用C MoM求解器
        result = self.c_solver_manager.call_mom_solver(mom_config)
        
        if result.get('success', False):
            impedance_matrix = np.array(result.get('impedance_matrix', []))
            
            print(f"   ✅ C MoM求解器成功计算阻抗矩阵")
            print(f"      矩阵尺寸: {impedance_matrix.shape}")
            print(f"      条件数: {result.get('condition_number', 'unknown')}")
            print(f"      计算时间: {result.get('computation_time', 0):.2f} 秒")
            
            return impedance_matrix
        else:
            print(f"   ⚠️ C MoM求解器失败: {result.get('error', 'unknown error')}")
            return None

# C PEEC求解器接口
class CPEECSolverInterface:
    """C PEEC求解器接口"""
    
    def __init__(self, c_solver_manager: NativeCSolverManager):
        self.c_solver_manager = c_solver_manager
        
    def solve_peec_with_c_solver(self, rwg_functions: List[Dict], 
                                 frequency: float,
                                 excitation_config: Dict) -> Optional[Dict]:
        """使用C PEEC求解器求解等效电路"""
        
        if not self.c_solver_manager.is_available():
            print("⚠️ C PEEC求解器不可用，回退到Python实现")
            return None
        
        print(f"   调用C PEEC求解器...")
        
        # 准备C PEEC求解器配置
        peec_config = {
            'frequency': frequency,
            'basis_functions': rwg_functions,
            'excitation': excitation_config,
            'solver_type': 'frequency_domain',
            'circuit_solver': 'mna',  # Modified Nodal Analysis
            'include_resistance': True,
            'include_inductance': True,
            'include_capacitance': True,
            'include_conductance': True,
            'frequency_sweep': False,
            'tolerance': 1e-6,
            'parallel': True
        }
        
        # 调用C PEEC求解器
        result = self.c_solver_manager.call_peec_solver(peec_config)
        
        if result.get('success', False):
            print(f"   ✅ C PEEC求解器成功求解")
            print(f"      电路节点数: {result.get('num_nodes', 0)}")
            print(f"      电路元件数: {result.get('num_elements', 0)}")
            print(f"      求解时间: {result.get('solution_time', 0):.2f} 秒")
            
            return {
                'node_voltages': result.get('node_voltages', []),
                'branch_currents': result.get('branch_currents', []),
                'y_matrix': result.get('y_matrix', []),
                'circuit_elements': result.get('circuit_elements', []),
                'c_solver_used': True
            }
        else:
            print(f"   ⚠️ C PEEC求解器失败: {result.get('error', 'unknown error')}")
            return None

# Python与C求解器结果比较器
class SolverResultComparator:
    """Python与C求解器结果比较器"""
    
    def __init__(self):
        self.comparison_results = {}
        
    def compare_impedance_matrices(self, python_matrix: np.ndarray, 
                                 c_matrix: np.ndarray, 
                                 tolerance: float = 1e-3) -> Dict[str, Any]:
        """比较Python和C求解器的阻抗矩阵"""
        
        if python_matrix is None or c_matrix is None:
            return {'valid': False, 'error': 'Missing matrix data'}
        
        # 检查矩阵尺寸
        if python_matrix.shape != c_matrix.shape:
            return {'valid': False, 'error': f'Shape mismatch: {python_matrix.shape} vs {c_matrix.shape}'}
        
        # 计算差异
        diff_matrix = python_matrix - c_matrix
        max_abs_diff = np.max(np.abs(diff_matrix))
        max_rel_diff = np.max(np.abs(diff_matrix) / (np.abs(python_matrix) + 1e-12))
        
        # 计算统计指标
        mean_abs_diff = np.mean(np.abs(diff_matrix))
        rms_diff = np.sqrt(np.mean(np.abs(diff_matrix)**2))
        
        # 计算相关性
        correlation = np.corrcoef(python_matrix.flatten(), c_matrix.flatten())[0, 1]
        
        # 判断是否在容差范围内
        within_tolerance = max_rel_diff < tolerance
        
        result = {
            'valid': True,
            'within_tolerance': within_tolerance,
            'max_absolute_difference': max_abs_diff,
            'max_relative_difference': max_rel_diff,
            'mean_absolute_difference': mean_abs_diff,
            'rms_difference': rms_diff,
            'correlation_coefficient': correlation,
            'tolerance_used': tolerance,
            'matrix_size': python_matrix.shape
        }
        
        print(f"   阻抗矩阵比较结果:")
        print(f"      最大相对差异: {max_rel_diff:.2e}")
        print(f"      相关系数: {correlation:.4f}")
        print(f"      容差范围内: {'是' if within_tolerance else '否'}")
        
        return result
    
    def compare_surface_currents(self, python_currents: np.ndarray,
                                c_currents: np.ndarray,
                                tolerance: float = 1e-2) -> Dict[str, Any]:
        """比较Python和C求解器的表面电流"""
        
        if python_currents is None or c_currents is None:
            return {'valid': False, 'error': 'Missing current data'}
        
        # 检查尺寸
        if len(python_currents) != len(c_currents):
            return {'valid': False, 'error': f'Size mismatch: {len(python_currents)} vs {len(c_currents)}'}
        
        # 计算差异
        diff = python_currents - c_currents
        max_abs_diff = np.max(np.abs(diff))
        max_rel_diff = np.max(np.abs(diff) / (np.abs(python_currents) + 1e-12))
        
        # 计算统计指标
        mean_abs_diff = np.mean(np.abs(diff))
        rms_diff = np.sqrt(np.mean(np.abs(diff)**2))
        
        # 计算相关性
        correlation = np.corrcoef(np.abs(python_currents), np.abs(c_currents))[0, 1]
        
        # 判断是否在容差范围内
        within_tolerance = max_rel_diff < tolerance
        
        result = {
            'valid': True,
            'within_tolerance': within_tolerance,
            'max_absolute_difference': max_abs_diff,
            'max_relative_difference': max_rel_diff,
            'mean_absolute_difference': mean_abs_diff,
            'rms_difference': rms_diff,
            'correlation_coefficient': correlation,
            'tolerance_used': tolerance
        }
        
        print(f"   表面电流比较结果:")
        print(f"      最大相对差异: {max_rel_diff:.2e}")
        print(f"      相关系数: {correlation:.4f}")
        print(f"      容差范围内: {'是' if within_tolerance else '否'}")
        
        return result

# 专业MoM求解器 - 带C求解器集成
class ProfessionalMoMSolver:
    """专业MoM求解器 - 包含C求解器集成"""
    
    def __init__(self, frequency, constants, c_solver_manager=None):
        self.frequency = frequency
        self.constants = constants
        self.omega = 2 * np.pi * frequency
        self.k = self.omega * np.sqrt(constants.EPSILON_0 * constants.MU_0)
        self.eta = constants.ETA_0
        self.wavelength = constants.C / frequency
        
        # C求解器管理器
        self.c_solver_manager = c_solver_manager
        self.c_mom_interface = CMoMSolverInterface(c_solver_manager) if c_solver_manager else None
        
    def calculate_impedance_matrix(self, rwg_functions, material='PEC'):
        """计算MoM阻抗矩阵 - 优先使用C求解器"""
        
        print(f"   计算MoM阻抗矩阵 (频率: {self.frequency/1e9:.1f} GHz)...")
        
        # 首先尝试使用C求解器
        if self.c_mom_interface:
            material_props = MaterialDatabase.get_material(material)
            c_matrix = self.c_mom_interface.calculate_impedance_matrix_with_c_solver(
                rwg_functions, self.frequency, material_props
            )
            if c_matrix is not None:
                return c_matrix
        
        # C求解器失败，使用Python实现
        print("   使用Python MoM求解器...")
        return self._calculate_impedance_matrix_python(rwg_functions, material)
    
    def _calculate_impedance_matrix_python(self, rwg_functions, material):
        """Python实现的MoM阻抗矩阵计算"""
        
        N = len(rwg_functions)
        Z_matrix = np.zeros((N, N), dtype=complex)
        
        # 预计算几何数据
        print(f"   预计算几何数据 ({N} 个基函数)...")
        
        for m in range(N):
            if m % 50 == 0:
                print(f"     计算行 {m+1}/{N}")
            
            for n in range(N):
                # 检查是否为自项
                if m == n:
                    Z_matrix[m, n] = self._calculate_self_impedance(rwg_functions[m])
                else:
                    # 检查是否为邻近项
                    if self._are_adjacent(rwg_functions[m], rwg_functions[n]):
                        Z_matrix[m, n] = self._calculate_near_impedance(rwg_functions[m], rwg_functions[n])
                    else:
                        Z_matrix[m, n] = self._calculate_far_impedance(rwg_functions[m], rwg_functions[n])
        
        print(f"   阻抗矩阵计算完成: {N}×{N}")
        return Z_matrix
    
    def _calculate_self_impedance(self, rwg_m):
        """计算自阻抗 - 包含奇异性处理"""
        
        # 使用解析公式处理奇异性
        area_plus = rwg_m['plus_triangle']['area']
        area_minus = rwg_m['minus_triangle']['area']
        edge_length = rwg_m['edge_length']
        
        # RWG自阻抗解析公式
        term1 = (1j * self.omega * self.constants.MU_0) / (4 * np.pi)
        term2 = (edge_length ** 2) / 6.0
        term3 = (1.0 / area_plus) + (1.0 / area_minus)
        
        # 添加正则化项避免奇异性
        regularization = 1e-6 * self.eta
        
        Z_self = term1 * term2 * term3 + regularization
        
        return Z_self
    
    def _calculate_near_impedance(self, rwg_m, rwg_n):
        """计算邻近阻抗 - 高精度积分"""
        
        # 使用高阶高斯积分
        num_points = 16
        
        # 获取三角形几何
        tri_m_plus = rwg_m['plus_triangle']
        tri_m_minus = rwg_m['minus_triangle']
        tri_n_plus = rwg_n['plus_triangle']
        tri_n_minus = rwg_n['minus_triangle']
        
        Z_near = 0.0 + 0.0j
        
        # 在四个三角形组合上积分
        triangles_m = [tri_m_plus, tri_m_minus]
        triangles_n = [tri_n_plus, tri_n_minus]
        
        for i, tri_m in enumerate(triangles_m):
            for j, tri_n in enumerate(triangles_n):
                # 计算三角形间相互作用
                integral = self._triangle_triangle_interaction(tri_m, tri_n, num_points)
                
                # RWG权重
                sign_m = 1.0 if i == 0 else -1.0
                sign_n = 1.0 if j == 0 else -1.0
                
                Z_near += sign_m * sign_n * integral
        
        return Z_near
    
    def _calculate_far_impedance(self, rwg_m, rwg_n):
        """计算远场阻抗 - 简化积分"""
        
        # 计算三角形中心距离
        center_m = self._get_triangle_center(rwg_m['plus_triangle'])
        center_n = self._get_triangle_center(rwg_n['plus_triangle'])
        
        distance = np.linalg.norm(np.array(center_m) - np.array(center_n))
        
        # 远场近似
        if distance > 0.1 * self.wavelength:  # 远场条件
            # 使用点源近似
            area_m = rwg_m['plus_triangle']['area'] + rwg_m['minus_triangle']['area']
            area_n = rwg_n['plus_triangle']['area'] + rwg_n['minus_triangle']['area']
            
            # 远场格林函数
            G = np.exp(-1j * self.k * distance) / (4 * np.pi * distance)
            
            # 远场阻抗
            Z_far = 1j * self.omega * self.constants.MU_0 * area_m * area_n * G
            
            return Z_far
        else:
            # 中场使用标准积分
            return self._calculate_near_impedance(rwg_m, rwg_n)
    
    def _triangle_triangle_interaction(self, tri_m, tri_n, num_points):
        """计算三角形间相互作用"""
        
        # 高斯积分点和权重
        points, weights = self._get_gauss_points(num_points)
        
        interaction = 0.0 + 0.0j
        
        for i, (xi_m, eta_m) in enumerate(points):
            for j, (xi_n, eta_n) in enumerate(points):
                # 计算积分点坐标
                r_m = self._interpolate_triangle_point(tri_m, xi_m, eta_m)
                r_n = self._interpolate_triangle_point(tri_n, xi_n, eta_n)
                
                # 计算距离
                distance = np.linalg.norm(np.array(r_m) - np.array(r_n))
                
                if distance > 1e-10:  # 避免自相互作用
                    # 格林函数
                    G = np.exp(-1j * self.k * distance) / (4 * np.pi * distance)
                    
                    # RWG基函数值
                    f_m = self._evaluate_rwg_basis(tri_m, xi_m, eta_m)
                    f_n = self._evaluate_rwg_basis(tri_n, xi_n, eta_n)
                    
                    # 积分贡献
                    contribution = weights[i] * weights[j] * np.dot(f_m, f_n) * G
                    interaction += contribution
        
        # 乘以频率和常数因子
        interaction *= 1j * self.omega * self.constants.MU_0
        
        return interaction
    
    def _get_gauss_points(self, n):
        """获取高斯积分点和权重"""
        
        if n == 1:
            points = [(1.0/3.0, 1.0/3.0)]
            weights = [1.0]
        elif n == 3:
            points = [(0.5, 0.0), (0.5, 0.5), (0.0, 0.5)]
            weights = [1.0/3.0, 1.0/3.0, 1.0/3.0]
        elif n == 7:
            # 7点高斯积分
            points = [
                (0.333333333333333, 0.333333333333333),
                (0.797426985353087, 0.101286507323456),
                (0.101286507323456, 0.797426985353087),
                (0.101286507323456, 0.101286507323456),
                (0.059715871789770, 0.470142064105115),
                (0.470142064105115, 0.059715871789770),
                (0.470142064105115, 0.470142064105115)
            ]
            weights = [
                0.225000000000000,
                0.125939180544827,
                0.125939180544827,
                0.125939180544827,
                0.132394152788506,
                0.132394152788506,
                0.132394152788506
            ]
        else:
            # 默认使用7点积分
            return self._get_gauss_points(7)
        
        return points, weights
    
    def _interpolate_triangle_point(self, triangle, xi, eta):
        """三角形内插值点坐标"""
        
        vertices = triangle['vertices']
        if len(vertices) != 3:
            return [0, 0, 0]
        
        # 三角形坐标插值
        zeta = 1.0 - xi - eta
        
        x = xi * vertices[0][0] + eta * vertices[1][0] + zeta * vertices[2][0]
        y = xi * vertices[0][1] + eta * vertices[1][1] + zeta * vertices[2][1]
        z = xi * vertices[0][2] + eta * vertices[1][2] + zeta * vertices[2][2]
        
        return [x, y, z]
    
    def _evaluate_rwg_basis(self, triangle, xi, eta):
        """评估RWG基函数 - 使用标准RWG公式"""
        
        vertices = triangle['vertices']
        if len(vertices) != 3:
            return [0, 0, 0]
        
        # 计算三角形面积和法向量
        edge1 = np.array(vertices[1]) - np.array(vertices[0])
        edge2 = np.array(vertices[2]) - np.array(vertices[0])
        
        cross_prod = np.cross(edge1, edge2)
        area = 0.5 * np.linalg.norm(cross_prod)
        normal = cross_prod / (2.0 * area) if area > 0 else np.array([0, 0, 1])
        
        # 计算重心坐标
        zeta = 1 - xi - eta
        
        # 计算从顶点到评估点的向量
        eval_point = (xi * np.array(vertices[1]) + 
                     eta * np.array(vertices[2]) + 
                     zeta * np.array(vertices[0]))
        
        # 标准RWG基函数: f(r) = (r - r_opp) * l / (2 * A)
        # 其中r_opp是对面顶点，l是边长，A是面积
        
        # 对于不同的边，对面顶点不同
        # 这里实现简化版本，返回方向向量
        if xi > eta and xi > zeta:  # 边0-1的对面顶点是2
            opp_vertex = 2
        elif eta > zeta:  # 边0-2的对面顶点是1
            opp_vertex = 1
        else:  # 边1-2的对面顶点是0
            opp_vertex = 0
        
        r_opp = np.array(vertices[opp_vertex])
        r_vec = eval_point - r_opp
        
        # 计算边长（对应RWG边）
        if opp_vertex == 2:  # 边0-1
            edge_length = np.linalg.norm(np.array(vertices[1]) - np.array(vertices[0]))
        elif opp_vertex == 1:  # 边0-2
            edge_length = np.linalg.norm(np.array(vertices[2]) - np.array(vertices[0]))
        else:  # 边1-2
            edge_length = np.linalg.norm(np.array(vertices[2]) - np.array(vertices[1]))
        
        # 标准RWG公式: f(r) = (r - r_opp) * l / (2 * A)
        basis_vector = r_vec * edge_length / (2.0 * area) if area > 0 else np.array([0, 0, 0])
        
        return basis_vector.tolist()
    
    def _get_triangle_center(self, triangle):
        """获取三角形中心"""
        
        vertices = triangle['vertices']
        if len(vertices) != 3:
            return [0, 0, 0]
        
        center = np.mean(np.array(vertices), axis=0)
        return center.tolist()
    
    def _are_adjacent(self, rwg_m, rwg_n):
        """检查两个RWG函数是否相邻"""
        
        # 检查是否共享三角形
        tri_m_plus = rwg_m['plus_triangle']['index']
        tri_m_minus = rwg_m['minus_triangle']['index']
        tri_n_plus = rwg_n['plus_triangle']['index']
        tri_n_minus = rwg_n['minus_triangle']['index']
        
        shared_triangles = set([tri_m_plus, tri_m_minus]) & set([tri_n_plus, tri_n_minus])
        
        return len(shared_triangles) > 0

# 高级RWG基函数生成器 - 增强版
class AdvancedRWGBasisGenerator:
    """高级RWG基函数生成器 - 专业电磁仿真级实现"""
    
    def __init__(self, wavelength, c_mesh_engine=None):
        self.wavelength = wavelength
        self.target_edge_length = wavelength / 10  # 标准λ/10网格
        self.c_mesh_engine = c_mesh_engine  # C网格引擎接口
        
    def generate_rwg_functions(self, vertices, facets, stl_file=None):
        """生成RWG基函数 - 优先使用C网格引擎"""
        
        print(f"   生成RWG基函数 (目标边长: {self.target_edge_length:.4f} m)...")
        
        # 首先尝试使用C网格引擎
        if self.c_mesh_engine and stl_file:
            print("   尝试使用C网格引擎...")
            mesh_result = self.c_mesh_engine.generate_mesh_with_c_engine(
                stl_file, EMConstants.C / self.wavelength, self.target_edge_length
            )
            
            if mesh_result and mesh_result.get('c_engine_used', False):
                print("   ✅ 使用C网格引擎生成的网格")
                vertices = mesh_result['vertices']
                facets = mesh_result['triangles']
                
                # 使用C网格引擎的质量指标
                quality_metrics = mesh_result.get('mesh_quality_metrics', {})
                if quality_metrics:
                    print(f"      最小角度: {quality_metrics.get('min_angle', 0):.1f}°")
                    print(f"      最大长宽比: {quality_metrics.get('max_aspect_ratio', 0):.1f}")
        
        # 使用Python实现生成RWG基函数
        return self._generate_rwg_functions_python(vertices, facets)
    
    def _generate_rwg_functions_python(self, vertices, facets):
        """Python实现的RWG基函数生成"""
        
        # 检查facets类型：是包含坐标还是索引
        facets_contain_coordinates = self._check_facet_type(facets)
        
        if facets_contain_coordinates:
            print("   检测到facets包含坐标数据，转换为索引格式...")
            # 将坐标facets转换为索引facets
            vertices, facets = self._convert_coordinate_facets_to_indices(vertices, facets)
        
        # 第一步：顶点合并 - 处理重复顶点
        merged_vertices, vertex_remap = self._merge_duplicate_vertices(vertices)
        remapped_facets = self._remap_facet_indices(facets, vertex_remap)
        
        print(f"   顶点合并: {len(vertices)} -> {len(merged_vertices)}")
        
        # 第二步：构建边连接性映射
        edge_map = {}
        facet_edges = []
        
        for facet_idx, facet in enumerate(remapped_facets):
            edges = []
            for i in range(3):
                try:
                    v1_idx = facet[i]
                    v2_idx = facet[(i + 1) % 3]
                    
                    # 确保索引是整数类型
                    if isinstance(v1_idx, (int, np.integer)) and isinstance(v2_idx, (int, np.integer)):
                        # 排序顶点索引以确保边的一致性
                        edge_key = tuple(sorted([int(v1_idx), int(v2_idx)]))
                        
                        if edge_key not in edge_map:
                            edge_map[edge_key] = []
                        edge_map[edge_key].append(facet_idx)
                        
                        edges.append(edge_key)
                    else:
                        # 如果不是整数索引，跳过这条边
                        print(f"   ⚠️  跳过非整数顶点索引: facet {facet_idx}, vertices {v1_idx}, {v2_idx}")
                        continue
                except (IndexError, TypeError) as e:
                    print(f"   ⚠️  处理facet {facet_idx} 边 {i} 时出错: {e}")
                    continue
            
            if len(edges) == 3:  # 只有完整的三角形才添加
                facet_edges.append(edges)
        
        # 第三步：找到内部边（共享边）
        internal_edges = []
        boundary_edges = []
        
        for edge_key, facet_list in edge_map.items():
            if len(facet_list) == 2:  # 内部边
                internal_edges.append({
                    'edge_key': edge_key,
                    'facets': facet_list,
                    'vertices': list(edge_key)
                })
            elif len(facet_list) == 1:  # 边界边
                boundary_edges.append({
                    'edge_key': edge_key,
                    'facets': facet_list,
                    'vertices': list(edge_key)
                })
        
        print(f"   总边数: {len(edge_map)}, 内部边: {len(internal_edges)}, 边界边: {len(boundary_edges)}")
        
        # 第四步：几何质量检查
        mesh_quality = self._check_mesh_quality(merged_vertices, remapped_facets)
        print(f"   网格质量: 最小角度={mesh_quality['min_angle']:.1f}°, 最大角度={mesh_quality['max_angle']:.1f}°")
        
        if len(internal_edges) == 0:
            print("   ⚠️  未找到内部边，尝试增强连接性分析")
            return self._create_enhanced_rwg_functions(merged_vertices, remapped_facets)
        
        # 第五步：生成RWG基函数
        rwg_functions = []
        
        for edge_idx, edge_info in enumerate(internal_edges):
            edge_vertices = edge_info['vertices']
            facet_plus = edge_info['facets'][0]
            facet_minus = edge_info['facets'][1]
            
            # 获取三角形顶点
            tri_plus_vertices = remapped_facets[facet_plus]
            tri_minus_vertices = remapped_facets[facet_minus]
            
            # 找到自由顶点
            free_vertex_plus = self._find_free_vertex(tri_plus_vertices, edge_vertices)
            free_vertex_minus = self._find_free_vertex(tri_minus_vertices, edge_vertices)
            
            if free_vertex_plus is None or free_vertex_minus is None:
                continue
            
            # 计算三角形属性
            area_plus = self._triangle_area(merged_vertices, tri_plus_vertices)
            area_minus = self._triangle_area(merged_vertices, tri_minus_vertices)
            
            # 边长度
            edge_length = self._edge_length(merged_vertices, edge_vertices)
            
            # 计算边中点（RWG函数的关键点）
            edge_midpoint = self._calculate_edge_midpoint(merged_vertices, edge_vertices)
            
            # 创建RWG函数
            rwg_function = {
                'id': edge_idx,
                'edge_vertices': edge_vertices,
                'edge_midpoint': edge_midpoint,
                'plus_triangle': {
                    'vertices': tri_plus_vertices,
                    'area': area_plus,
                    'free_vertex': free_vertex_plus,
                    'index': facet_plus,
                    'centroid': self._calculate_triangle_centroid(merged_vertices, tri_plus_vertices)
                },
                'minus_triangle': {
                    'vertices': tri_minus_vertices,
                    'area': area_minus,
                    'free_vertex': free_vertex_minus,
                    'index': facet_minus,
                    'centroid': self._calculate_triangle_centroid(merged_vertices, tri_minus_vertices)
                },
                'edge_length': edge_length,
                'total_area': area_plus + area_minus,
                'quality': self._calculate_rwg_quality(merged_vertices, tri_plus_vertices, tri_minus_vertices)
            }
            
            rwg_functions.append(rwg_function)
        
        print(f"   生成RWG函数: {len(rwg_functions)}")
        
        # 第六步：验证RWG函数质量
        if len(rwg_functions) > 0:
            quality_stats = self._analyze_rwg_quality(rwg_functions)
            print(f"   RWG质量: 平均边长={quality_stats['avg_edge_length']:.4f}m, 最小面积={quality_stats['min_area']:.6f}m²")
        
        return rwg_functions
    
    def _check_facet_type(self, facets):
        """检查facet类型：是否包含坐标数据"""
        if not facets:
            return False
        
        # 检查第一个facet的第一个元素
        first_facet = facets[0]
        if not first_facet:
            return False
        
        first_vertex = first_facet[0]
        
        # 如果是列表或数组，且包含数字坐标，则认为是坐标数据
        if isinstance(first_vertex, (list, np.ndarray)):
            # 检查是否包含浮点数坐标
            if len(first_vertex) >= 3 and all(isinstance(coord, (int, float, np.number)) for coord in first_vertex[:3]):
                return True
        
        return False
    
    def _convert_coordinate_facets_to_indices(self, vertices, facets):
        """将包含坐标的facets转换为索引facets"""
        
        print("   转换坐标facets为索引facets...")
        
        # 创建顶点到索引的映射
        vertex_to_index = {}
        new_vertices = []
        
        # 处理现有顶点
        for i, vertex in enumerate(vertices):
            if isinstance(vertex, (list, np.ndarray)) and len(vertex) >= 3:
                vertex_tuple = tuple(float(coord) for coord in vertex[:3])
                vertex_to_index[vertex_tuple] = i
                new_vertices.append(vertex)
        
        # 转换facets
        new_facets = []
        tolerance = 1e-6  # 坐标匹配容差
        
        for facet_idx, facet in enumerate(facets):
            if len(facet) != 3:
                continue
                
            new_facet = []
            
            for vertex_data in facet:
                if isinstance(vertex_data, (list, np.ndarray)) and len(vertex_data) >= 3:
                    # 提取坐标
                    coords = tuple(float(coord) for coord in vertex_data[:3])
                    
                    # 查找匹配的顶点
                    found_index = None
                    for existing_coords, existing_index in vertex_to_index.items():
                        if all(abs(a - b) < tolerance for a, b in zip(coords, existing_coords)):
                            found_index = existing_index
                            break
                    
                    if found_index is not None:
                        new_facet.append(found_index)
                    else:
                        # 添加新顶点
                        new_index = len(new_vertices)
                        new_vertices.append(list(coords))
                        vertex_to_index[coords] = new_index
                        new_facet.append(new_index)
                else:
                    # 如果已经是索引，直接使用
                    if isinstance(vertex_data, (int, np.integer)):
                        new_facet.append(int(vertex_data))
            
            if len(new_facet) == 3:
                new_facets.append(new_facet)
        
        print(f"   转换完成: {len(facets)} facets -> {len(new_facets)} 索引facets")
        print(f"   顶点数: {len(new_vertices)}")
        
        return new_vertices, new_facets
    
    def _merge_duplicate_vertices(self, vertices):
        """合并重复顶点 - 处理几何中的重复点"""
        if not vertices:
            return [], {}
        
        # 将顶点转换为numpy数组以便处理
        vertex_array = np.array(vertices)
        
        # 使用容差判断重复顶点
        tolerance = 1e-6  # 1微米容差
        
        # 创建顶点映射
        merged_vertices = []
        vertex_remap = {}
        
        for i, vertex in enumerate(vertex_array):
            # 检查是否与已合并的顶点重复
            is_duplicate = False
            for j, merged_vertex in enumerate(merged_vertices):
                if np.linalg.norm(vertex - merged_vertex) < tolerance:
                    vertex_remap[i] = j
                    is_duplicate = True
                    break
            
            if not is_duplicate:
                new_index = len(merged_vertices)
                merged_vertices.append(vertex)
                vertex_remap[i] = new_index
        
        return merged_vertices, vertex_remap
    
    def _remap_facet_indices(self, facets, vertex_remap):
        """重新映射面片顶点索引"""
        remapped_facets = []
        
        for facet in facets:
            remapped_facet = []
            for vertex_idx in facet:
                if isinstance(vertex_idx, (int, np.integer)):
                    # 如果facet包含的是顶点索引
                    if vertex_idx in vertex_remap:
                        remapped_facet.append(vertex_remap[vertex_idx])
                    else:
                        remapped_facet.append(vertex_idx)  # 保持原索引
                else:
                    # 如果facet已经包含的是坐标，需要特殊处理
                    remapped_facet.append(vertex_idx)
            remapped_facets.append(remapped_facet)
        
        return remapped_facets
    
    def _check_mesh_quality(self, vertices, facets):
        """检查网格质量"""
        if not facets or len(vertices) == 0:
            return {'min_angle': 0, 'max_angle': 0, 'aspect_ratio': 0}
        
        angles = []
        aspect_ratios = []
        
        for facet in facets:
            if len(facet) != 3:
                continue
                
            # 获取三角形顶点
            if isinstance(facet[0], (int, np.integer)):
                # facet包含顶点索引
                if max(facet) >= len(vertices):
                    continue
                v0 = np.array(vertices[facet[0]])
                v1 = np.array(vertices[facet[1]])
                v2 = np.array(vertices[facet[2]])
            else:
                # facet已经包含坐标
                v0 = np.array(facet[0])
                v1 = np.array(facet[1])
                v2 = np.array(facet[2])
            
            # 计算边长
            edge_lengths = [
                np.linalg.norm(v1 - v0),
                np.linalg.norm(v2 - v1),
                np.linalg.norm(v0 - v2)
            ]
            
            # 使用余弦定理计算角度
            triangle_angles = []
            for i in range(3):
                a, b, c = edge_lengths[(i+1)%3], edge_lengths[(i+2)%3], edge_lengths[i]
                if a > 0 and b > 0:
                    cos_angle = (a**2 + b**2 - c**2) / (2 * a * b)
                    cos_angle = np.clip(cos_angle, -1, 1)  # 防止数值误差
                    angle = np.arccos(cos_angle) * 180 / np.pi
                    triangle_angles.append(angle)
            
            if len(triangle_angles) == 3:
                angles.extend(triangle_angles)
                
                # 计算长宽比
                max_edge = max(edge_lengths)
                min_edge = min(edge_lengths)
                if min_edge > 0:
                    aspect_ratios.append(max_edge / min_edge)
        
        return {
            'min_angle': min(angles) if angles else 0,
            'max_angle': max(angles) if angles else 0,
            'aspect_ratio': max(aspect_ratios) if aspect_ratios else 0
        }
    
    def _calculate_triangle_centroid(self, vertices, triangle_vertices):
        """计算三角形质心"""
        if len(triangle_vertices) != 3:
            return np.array([0, 0, 0])
        
        # triangle_vertices 可能是顶点坐标列表或顶点索引列表
        if isinstance(triangle_vertices[0], (int, np.integer)):
            # 如果是索引，从vertices中获取坐标
            if int(max(triangle_vertices)) < len(vertices):
                v0 = np.array(vertices[triangle_vertices[0]])
                v1 = np.array(vertices[triangle_vertices[1]])
                v2 = np.array(vertices[triangle_vertices[2]])
            else:
                return np.array([0, 0, 0])
        else:
            # 如果已经是坐标列表
            v0 = np.array(triangle_vertices[0])
            v1 = np.array(triangle_vertices[1])
            v2 = np.array(triangle_vertices[2])
        
        return (v0 + v1 + v2) / 3
    
    def _calculate_rwg_quality(self, vertices, tri_plus, tri_minus):
        """计算RWG函数质量"""
        # 计算两个三角形的面积比
        area_plus = self._triangle_area(vertices, tri_plus)
        area_minus = self._triangle_area(vertices, tri_minus)
        
        if area_plus <= 0 or area_minus <= 0:
            return 0.0
        
        # 质量指标：面积比接近1.0为最佳
        area_ratio = min(area_plus, area_minus) / max(area_plus, area_minus)
        return area_ratio
    
    def _analyze_rwg_quality(self, rwg_functions):
        """分析RWG函数质量统计"""
        edge_lengths = []
        areas = []
        qualities = []
        
        for rwg in rwg_functions:
            edge_lengths.append(rwg['edge_length'])
            areas.append(rwg['total_area'])
            if 'quality' in rwg:
                qualities.append(rwg['quality'])
        
        return {
            'avg_edge_length': np.mean(edge_lengths) if edge_lengths else 0,
            'min_edge_length': min(edge_lengths) if edge_lengths else 0,
            'max_edge_length': max(edge_lengths) if edge_lengths else 0,
            'min_area': min(areas) if areas else 0,
            'max_area': max(areas) if areas else 0,
            'avg_quality': np.mean(qualities) if qualities else 0
        }
    
    def _create_enhanced_rwg_functions(self, vertices, facets):
        """增强版RWG函数创建 - 使用更智能的连接性分析"""
        
        print("   创建增强RWG函数...")
        
        # 寻找接近的边来创建人工连接
        tolerance = 1e-3  # 1mm容差
        rwg_functions = []
        created_count = 0
        
        # 构建所有边的列表
        all_edges = []
        for facet_idx, facet in enumerate(facets):
            if len(facet) != 3:
                continue
                
            for i in range(3):
                v1_idx = facet[i]
                v2_idx = facet[(i + 1) % 3]
                
                # 获取顶点坐标
                if isinstance(v1_idx, (int, np.integer)) and isinstance(v2_idx, (int, np.integer)):
                    if max(v1_idx, v2_idx) < len(vertices):
                        edge_info = {
                            'facet_idx': facet_idx,
                            'v1_idx': v1_idx,
                            'v2_idx': v2_idx,
                            'v1_coord': np.array(vertices[v1_idx]),
                            'v2_coord': np.array(vertices[v2_idx])
                        }
                        all_edges.append(edge_info)
        
        # 寻找匹配的边对
        used_facets = set()
        
        for i, edge1 in enumerate(all_edges):
            if edge1['facet_idx'] in used_facets:
                continue
                
            for j, edge2 in enumerate(all_edges[i+1:], i+1):
                if edge2['facet_idx'] in used_facets or edge1['facet_idx'] == edge2['facet_idx']:
                    continue
                
                # 检查边是否匹配（顶点相反）
                if (np.linalg.norm(edge1['v1_coord'] - edge2['v2_coord']) < tolerance and
                    np.linalg.norm(edge1['v2_coord'] - edge2['v1_coord']) < tolerance):
                    
                    # 创建RWG函数
                    rwg_func = self._create_rwg_from_matched_edges(vertices, facets, edge1, edge2)
                    if rwg_func:
                        rwg_func['id'] = created_count
                        rwg_functions.append(rwg_func)
                        created_count += 1
                        used_facets.add(edge1['facet_idx'])
                        used_facets.add(edge2['facet_idx'])
                        break
        
        print(f"   创建了 {len(rwg_functions)} 个增强RWG函数")
        
        # 如果仍然没有足够的RWG函数，回退到简化版本
        if len(rwg_functions) < 10:
            print("   ⚠️  增强连接不足，使用简化后备方案")
            fallback_rwgs = self._create_artificial_rwg_functions(vertices, facets)
            rwg_functions.extend(fallback_rwgs)
        
        return rwg_functions
    
    def _create_rwg_from_matched_edges(self, vertices, facets, edge1, edge2):
        """从匹配的边创建RWG函数"""
        try:
            facet_plus_idx = edge1['facet_idx']
            facet_minus_idx = edge2['facet_idx']
            
            tri_plus = facets[facet_plus_idx]
            tri_minus = facets[facet_minus_idx]
            
            # 计算属性
            area_plus = self._triangle_area(vertices, tri_plus)
            area_minus = self._triangle_area(vertices, tri_minus)
            
            # 边长度
            edge_length = np.linalg.norm(edge1['v2_coord'] - edge1['v1_coord'])
            
            # 创建边顶点索引
            edge_vertices = [edge1['v1_idx'], edge1['v2_idx']]
            
            # 找到自由顶点
            free_vertex_plus = self._find_free_vertex(tri_plus, edge_vertices)
            free_vertex_minus = self._find_free_vertex(tri_minus, edge_vertices)
            
            if free_vertex_plus is None or free_vertex_minus is None:
                return None
            
            return {
                'edge_vertices': edge_vertices,
                'plus_triangle': {
                    'vertices': tri_plus,
                    'area': area_plus,
                    'free_vertex': free_vertex_plus,
                    'index': facet_plus_idx
                },
                'minus_triangle': {
                    'vertices': tri_minus,
                    'area': area_minus,
                    'free_vertex': free_vertex_minus,
                    'index': facet_minus_idx
                },
                'edge_length': edge_length,
                'total_area': area_plus + area_minus,
                'enhanced': True
            }
        except Exception as e:
            print(f"   创建RWG函数失败: {e}")
            return None
    
    def _find_free_vertex(self, triangle_vertices, edge_vertices):
        """找到三角形中的自由顶点"""
        edge_set = set(edge_vertices)
        for i, vertex_idx in enumerate(triangle_vertices):
            if vertex_idx not in edge_set:
                return vertex_idx
        return None
    
    def _triangle_area(self, vertices, triangle_coords):
        """计算三角形面积"""
        if len(triangle_coords) != 3:
            return 0.0
            
        # triangle_coords 可能是顶点坐标列表或顶点索引列表
        if isinstance(triangle_coords[0], (int, np.integer)):
            # 如果是索引，从vertices中获取坐标
            if int(max(triangle_coords)) < len(vertices):
                v0 = np.array(vertices[triangle_coords[0]])
                v1 = np.array(vertices[triangle_coords[1]])
                v2 = np.array(vertices[triangle_coords[2]])
            else:
                return 0.0
        else:
            # 如果已经是坐标列表
            v0 = np.array(triangle_coords[0])
            v1 = np.array(triangle_coords[1])
            v2 = np.array(triangle_coords[2])