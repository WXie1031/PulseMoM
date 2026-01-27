#!/usr/bin/env python3
"""
CAD导入和仿真测试脚本

测试从CAD文件（STL）导入几何并进行电磁仿真的完整流程。
使用Python interface调用C库进行仿真。

使用方法:
    python tests/test_cad_import_simulation.py [stl_file] [frequency]
    
示例:
    python tests/test_cad_import_simulation.py tests/test_hpm/weixing_v1.stl 10e9
"""

import sys
import os
import numpy as np
import matplotlib.pyplot as plt
from pathlib import Path

# 添加项目根目录到路径
project_root = Path(__file__).parent.parent
sys.path.insert(0, str(project_root))

from python_interface.mom_peec_ctypes import (
    MoMPEECInterface,
    run_simulation
)


def test_cad_import_basic(stl_file: str, frequency: float = 10e9):
    """
    测试基本的CAD导入和仿真
    
    Args:
        stl_file: STL文件路径
        frequency: 仿真频率（Hz）
    """
    print("=" * 80)
    print("CAD导入和仿真测试 - 基本流程")
    print("=" * 80)
    print(f"STL文件: {stl_file}")
    print(f"频率: {frequency/1e9:.1f} GHz")
    print()
    
    # 检查文件是否存在
    if not os.path.exists(stl_file):
        print(f"错误: STL文件不存在: {stl_file}")
        return False
    
    try:
        # 使用便捷函数运行完整仿真
        print("步骤1: 加载几何和生成网格...")
        results = run_simulation(
            stl_file=stl_file,
            frequency=frequency,
            solver_type='mom',
            target_edge_length=0.03,  # 3cm at 10GHz
            max_facets=10000,
            min_quality=0.3,
            adaptive_refinement=False
        )
        
        print(f"✓ 网格生成完成")
        print(f"  顶点数: {results['mesh_info']['num_vertices']}")
        print(f"  三角形数: {results['mesh_info']['num_triangles']}")
        
        print("\n步骤2: 运行MoM仿真...")
        print(f"✓ 仿真完成")
        print(f"  基函数数: {results['currents']['num_basis']}")
        print(f"  总时间: {results['performance']['total_time']:.2f}s")
        print(f"  网格生成: {results['performance']['mesh_generation_time']:.2f}s")
        print(f"  矩阵组装: {results['performance']['matrix_assembly_time']:.2f}s")
        print(f"  求解: {results['performance']['solver_time']:.2f}s")
        print(f"  内存使用: {results['performance']['memory_usage']/1024/1024:.1f} MB")
        print(f"  收敛: {'是' if results['performance']['converged'] else '否'}")
        
        print("\n步骤3: 分析结果...")
        currents = results['currents']
        print(f"  电流幅度范围: {np.min(currents['magnitude']):.2e} - {np.max(currents['magnitude']):.2e} A/m")
        print(f"  电流相位范围: {np.min(currents['phase']):.2f} - {np.max(currents['phase']):.2f} rad")
        
        fields = results['fields']
        print(f"  电场幅度范围: {np.min(fields['e_magnitude']):.2e} - {np.max(fields['e_magnitude']):.2e} V/m")
        print(f"  磁场幅度范围: {np.min(fields['h_magnitude']):.2e} - {np.max(fields['h_magnitude']):.2e} A/m")
        
        print("\n✓ 测试完成")
        return True
        
    except Exception as e:
        print(f"✗ 测试失败: {e}")
        import traceback
        traceback.print_exc()
        return False


def test_cad_import_advanced(stl_file: str, frequency: float = 10e9):
    """
    测试高级CAD导入和仿真（自定义参数）
    
    Args:
        stl_file: STL文件路径
        frequency: 仿真频率（Hz）
    """
    print("\n" + "=" * 80)
    print("CAD导入和仿真测试 - 高级流程")
    print("=" * 80)
    
    try:
        # 创建接口实例
        interface = MoMPEECInterface()
        print(f"✓ 库版本: {interface.get_version()}")
        
        # 创建仿真实例
        sim_handle = interface.create_simulation()
        print("✓ 仿真实例已创建")
        
        # 加载STL
        print(f"\n加载STL文件: {stl_file}")
        error = interface.load_stl(sim_handle, stl_file)
        if error != 0:
            raise RuntimeError(f"加载STL失败: {interface.get_error_string(error)}")
        print("✓ STL文件加载成功")
        
        # 设置材料（PEC）
        material = {
            'name': 'PEC',
            'eps_r': 1.0,
            'mu_r': 1.0,
            'sigma': 1e20,
            'tan_delta': 0.0,
            'material_id': 1
        }
        error = interface.set_material(sim_handle, material)
        if error != 0:
            raise RuntimeError(f"设置材料失败: {interface.get_error_string(error)}")
        print("✓ 材料设置成功")
        
        # 生成网格
        mesh_params = {
            'target_edge_length': 0.03,
            'max_facets': 10000,
            'min_quality': 0.3,
            'adaptive_refinement': False,
            'mesh_algorithm': 'delaunay'
        }
        error = interface.generate_mesh(sim_handle, mesh_params)
        if error != 0:
            raise RuntimeError(f"生成网格失败: {interface.get_error_string(error)}")
        print("✓ 网格生成成功")
        
        # 获取网格信息
        num_vertices, num_triangles = interface.get_mesh_info(sim_handle)
        print(f"  网格信息: {num_vertices} 顶点, {num_triangles} 三角形")
        
        # 配置求解器
        solver_config = {
            'solver_type': 'mom',
            'frequency': frequency,
            'basis_order': 1,
            'formulation': 'efie',
            'tolerance': 1e-6,
            'max_iterations': 1000,
            'use_preconditioner': True,
            'use_fast_solver': False
        }
        error = interface.configure_solver(sim_handle, solver_config)
        if error != 0:
            raise RuntimeError(f"配置求解器失败: {interface.get_error_string(error)}")
        print("✓ 求解器配置成功")
        
        # 设置激励
        excitation = {
            'frequency': frequency,
            'amplitude': 1.0,
            'phase': 0.0,
            'theta': 0.0,  # 垂直入射
            'phi': 0.0,
            'polarization': 0.0
        }
        error = interface.set_excitation(sim_handle, excitation)
        if error != 0:
            raise RuntimeError(f"设置激励失败: {interface.get_error_string(error)}")
        print("✓ 激励设置成功")
        
        # 运行仿真
        print("\n运行仿真...")
        error = interface.run_simulation(sim_handle)
        if error != 0:
            raise RuntimeError(f"仿真失败: {interface.get_error_string(error)}")
        print("✓ 仿真完成")
        
        # 获取结果
        print("\n获取结果...")
        currents = interface.get_currents(sim_handle)
        performance = interface.get_performance(sim_handle)
        
        print(f"  基函数数: {currents['num_basis']}")
        print(f"  电流幅度范围: {np.min(currents['magnitude']):.2e} - {np.max(currents['magnitude']):.2e} A/m")
        print(f"  总时间: {performance['total_time']:.2f}s")
        print(f"  收敛: {'是' if performance['converged'] else '否'}")
        
        # 创建观测点
        print("\n计算场分布...")
        x = np.linspace(-2, 2, 21)
        y = np.linspace(-2, 2, 21)
        z = np.linspace(2, 4, 11)
        X, Y, Z = np.meshgrid(x, y, z, indexing='ij')
        obs_points = np.column_stack([X.ravel(), Y.ravel(), Z.ravel()])
        
        fields = interface.get_fields(sim_handle, obs_points)
        print(f"  观测点数: {fields['num_points']}")
        print(f"  电场幅度范围: {np.min(fields['e_magnitude']):.2e} - {np.max(fields['e_magnitude']):.2e} V/m")
        
        # 清理
        interface.destroy_simulation(sim_handle)
        print("\n✓ 高级测试完成")
        return True
        
    except Exception as e:
        print(f"✗ 测试失败: {e}")
        import traceback
        traceback.print_exc()
        return False


def visualize_results(results: dict, output_dir: str = "test_output"):
    """
    可视化仿真结果
    
    Args:
        results: 仿真结果字典
        output_dir: 输出目录
    """
    os.makedirs(output_dir, exist_ok=True)
    
    # 绘制电流分布
    currents = results['currents']
    plt.figure(figsize=(10, 6))
    plt.hist(currents['magnitude'], bins=50, alpha=0.7, edgecolor='black')
    plt.xlabel('Current Magnitude (A/m)')
    plt.ylabel('Count')
    plt.title('Surface Current Distribution')
    plt.yscale('log')
    plt.grid(True, alpha=0.3)
    plt.savefig(os.path.join(output_dir, 'current_distribution.png'), dpi=150)
    plt.close()
    
    # 绘制性能指标
    perf = results['performance']
    fig, ax = plt.subplots(figsize=(8, 6))
    times = [
        perf['mesh_generation_time'],
        perf['matrix_assembly_time'],
        perf['solver_time']
    ]
    labels = ['Mesh Generation', 'Matrix Assembly', 'Solver']
    ax.bar(labels, times)
    ax.set_ylabel('Time (s)')
    ax.set_title('Simulation Performance')
    plt.xticks(rotation=45)
    plt.tight_layout()
    plt.savefig(os.path.join(output_dir, 'performance.png'), dpi=150)
    plt.close()
    
    print(f"\n✓ 可视化结果已保存到: {output_dir}")


def main():
    """主函数"""
    # 解析命令行参数
    if len(sys.argv) > 1:
        stl_file = sys.argv[1]
    else:
        # 默认STL文件
        stl_file = "tests/test_hpm/weixing_v1.stl"
        if not os.path.exists(stl_file):
            print(f"错误: 默认STL文件不存在: {stl_file}")
            print("请提供STL文件路径作为参数")
            return 1
    
    if len(sys.argv) > 2:
        frequency = float(sys.argv[2])
    else:
        frequency = 10e9  # 默认10 GHz
    
    # 运行测试
    success1 = test_cad_import_basic(stl_file, frequency)
    success2 = test_cad_import_advanced(stl_file, frequency)
    
    if success1 and success2:
        print("\n" + "=" * 80)
        print("所有测试通过！")
        print("=" * 80)
        return 0
    else:
        print("\n" + "=" * 80)
        print("部分测试失败")
        print("=" * 80)
        return 1


if __name__ == "__main__":
    sys.exit(main())
