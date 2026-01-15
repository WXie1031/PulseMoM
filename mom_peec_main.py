#!/usr/bin/env python3
"""
通用MoM/PEEC电磁仿真框架 - 主程序
General MoM/PEEC Electromagnetic Simulation Framework - Main Program

该程序演示了通用MoM/PEEC框架的使用方法，包括：
1. 基础MoM和PEEC仿真
2. MoM-PEEC耦合仿真
3. 多种测试案例（卫星、天线、电路等）
4. 结果可视化与分析
"""

import numpy as np
import matplotlib.pyplot as plt
import sys
from pathlib import Path

# 导入框架组件
from mom_peec_framework import (
    Material, Excitation, MoMPeecSimulator, MoMPeecVisualizer
)
from mom_peec_solvers import ProfessionalMoMSolver, ProfessionalPEECSolver
from mom_peec_test_framework import (
    TestFramework, SatelliteTestCase, AntennaTestCase, CircuitTestCase
)


def demo_basic_simulation():
    """演示基础MoM/PEEC仿真"""
    print("🎯 基础MoM/PEEC仿真演示")
    print("=" * 50)
    
    # 定义材料
    materials = {
        'PEC': Material('PEC', epsr=1.0, mur=1.0, sigma=1e20),
        'ALUMINUM': Material('ALUMINUM', epsr=1.0, mur=1.0, sigma=3.5e7),
        'COPPER': Material('COPPER', epsr=1.0, mur=1.0, sigma=5.8e7)
    }
    
    # 定义激励
    plane_wave = Excitation(
        type='plane_wave',
        frequency=10e9,  # 10 GHz
        amplitude=1.0,
        phase=0.0,
        direction=np.array([1, 0, 0]),
        polarization=np.array([0, 1, 0])
    )
    
    # 创建仿真器
    simulator = MoMPeecSimulator(ProfessionalMoMSolver, ProfessionalPEECSolver)
    simulator.setup_simulation(10e9, materials)
    
    # 运行MoM仿真
    print("📐 运行MoM仿真...")
    stl_file = 'tests/test_hpm/weixing_v1.stl'
    mom_results = simulator.run_mom_simulation(stl_file, plane_wave, target_scale=2.8, stl_units='mm')
    
    # 运行PEEC仿真
    print("🔌 运行PEEC仿真...")
    peec_results = simulator.run_peec_simulation(stl_file, [plane_wave], target_scale=2.8, stl_units='mm')
    
    # 可视化结果
    visualizer = MoMPeecVisualizer()
    
    if mom_results.currents is not None:
        fig1 = visualizer.plot_current_distribution(mom_results.currents, "MoM Current Distribution")
        plt.savefig('demo_mom_currents.png', dpi=150, bbox_inches='tight')
        plt.close()
        print("✅ MoM电流分布图已保存: demo_mom_currents.png")
    
    if peec_results.currents is not None:
        fig2 = visualizer.plot_current_distribution(peec_results.currents, "PEEC Current Distribution")
        plt.savefig('demo_peec_currents.png', dpi=150, bbox_inches='tight')
        plt.close()
        print("✅ PEEC电流分布图已保存: demo_peec_currents.png")
    
    # 保存结果
    results = {
        'mom_results': mom_results.__dict__,
        'peec_results': peec_results.__dict__,
        'demo_type': 'basic_simulation'
    }
    simulator.results = results
    simulator.save_results('demo_basic_results.json')
    
    print("✅ 基础仿真演示完成！")


def demo_coupled_simulation():
    """演示MoM-PEEC耦合仿真"""
    print("\n🔄 MoM-PEEC耦合仿真演示")
    print("=" * 50)
    
    # 定义材料
    materials = {
        'PEC': Material('PEC', epsr=1.0, mur=1.0, sigma=1e20),
        'ALUMINUM': Material('ALUMINUM', epsr=1.0, mur=1.0, sigma=3.5e7)
    }
    
    # 定义激励
    plane_wave = Excitation(
        type='plane_wave',
        frequency=5e9,  # 5 GHz
        amplitude=1.0,
        phase=0.0,
        direction=np.array([0, 0, 1]),
        polarization=np.array([1, 0, 0])
    )
    
    # 创建仿真器
    simulator = MoMPeecSimulator(ProfessionalMoMSolver, ProfessionalPEECSolver)
    simulator.setup_simulation(5e9, materials)
    
    # 定义耦合区域
    coupling_regions = [
        {
            'mom_indices': list(range(0, 30)),  # 前30个MoM基函数
            'peec_indices': list(range(0, 20)),  # 前20个PEEC节点
            'coupling_strength': 0.15
        },
        {
            'mom_indices': list(range(30, 60)),  # 第30-60个MoM基函数
            'peec_indices': list(range(20, 40)),  # 第20-40个PEEC节点
            'coupling_strength': 0.08
        }
    ]
    
    # 运行耦合仿真
    print("🚀 运行MoM-PEEC耦合仿真...")
    coupled_results = simulator.run_coupled_simulation(
        'tests/test_hpm/weixing_v1.stl',
        plane_wave, [plane_wave],
        coupling_regions,
        target_scale=2.8, stl_units='mm'
    )
    
    # 可视化耦合结果
    if coupled_results and 'coupled_results' in coupled_results:
        visualizer = MoMPeecVisualizer()
        fig = visualizer.plot_coupling_results(
            coupled_results['coupled_results'], 
            "MoM-PEEC Coupled Simulation Results"
        )
        plt.savefig('demo_coupling_results.png', dpi=150, bbox_inches='tight')
        plt.close()
        print("✅ 耦合结果图已保存: demo_coupling_results.png")
    
    # 保存结果
    simulator.save_results('demo_coupled_results.json')
    
    print("✅ 耦合仿真演示完成！")


def demo_test_framework():
    """演示完整测试框架"""
    print("\n🧪 完整测试框架演示")
    print("=" * 50)
    
    # 创建测试框架
    framework = TestFramework(ProfessionalMoMSolver, ProfessionalPEECSolver)
    
    # 添加测试案例
    framework.add_test_case(SatelliteTestCase())
    framework.add_test_case(AntennaTestCase())
    framework.add_test_case(CircuitTestCase())
    
    # 运行所有测试
    print("📊 运行所有测试案例...")
    results = framework.run_all_tests(enable_coupling=True)
    
    # 生成报告
    report = framework.generate_summary_report()
    print("\n" + report)
    
    # 保存结果
    framework.save_results('demo_test_framework_results.json')
    
    # 绘制汇总图
    try:
        framework.plot_results_summary()
        print("✅ 测试汇总图已保存: test_results_summary.png")
    except Exception as e:
        print(f"⚠️ 绘图失败: {e}")
    
    print("✅ 测试框架演示完成！")


def demo_material_analysis():
    """演示材料分析功能"""
    print("\n🔬 材料分析演示")
    print("=" * 50)
    
    # 定义多种材料
    materials = {
        'PEC': Material('PEC', epsr=1.0, mur=1.0, sigma=1e20),
        'ALUMINUM': Material('ALUMINUM', epsr=1.0, mur=1.0, sigma=3.5e7),
        'COPPER': Material('COPPER', epsr=1.0, mur=1.0, sigma=5.8e7),
        'STEEL': Material('STEEL', epsr=1.0, mur=100.0, sigma=1.0e6),
        'CARBON_FIBER': Material('CARBON_FIBER', epsr=3.5, mur=1.0, sigma=1e4),
        'SOLAR_PANEL': Material('SOLAR_PANEL', epsr=11.7, mur=1.0, sigma=1e3),
        'FR4': Material('FR4', epsr=4.4, mur=1.0, sigma=1e-3),
        'AIR': Material('AIR', epsr=1.0, mur=1.0, sigma=0.0)
    }
    
    # 分析材料属性
    print("📋 材料属性分析:")
    print(f"{'材料名称':<15} {'类型':<10} {'εᵣ':<8} {'μᵣ':<8} {'σ (S/m)':<12}")
    print("-" * 60)
    
    for name, material in materials.items():
        if material.is_conductor():
            material_type = "导体"
        elif material.is_dielectric():
            material_type = "介质"
        elif material.is_magnetic():
            material_type = "磁性"
        else:
            material_type = "其他"
        
        print(f"{name:<15} {material_type:<10} {material.epsr:<8.2f} {material.mur:<8.2f} {material.sigma:<12.2e}")
    
    # 频率响应分析
    frequencies = np.logspace(6, 12, 100)  # 1MHz - 1THz
    
    plt.figure(figsize=(12, 8))
    
    # 导体材料
    for name in ['COPPER', 'ALUMINUM', 'STEEL']:
        material = materials[name]
        skin_depth = np.sqrt(2 / (2*np.pi*frequencies * EMConstants.MU_0 * material.mur * material.sigma))
        plt.loglog(frequencies/1e9, skin_depth*1000, label=f'{name} (趋肤深度)', linewidth=2)
    
    plt.xlabel('频率 (GHz)')
    plt.ylabel('趋肤深度 (mm)')
    plt.title('材料趋肤深度 vs 频率')
    plt.legend()
    plt.grid(True, alpha=0.3)
    plt.tight_layout()
    plt.savefig('demo_material_analysis.png', dpi=150, bbox_inches='tight')
    plt.close()
    
    print("\n✅ 材料分析图已保存: demo_material_analysis.png")
    
    # 保存材料数据
    material_data = {
        'materials': {name: {
            'name': mat.name,
            'epsr': mat.epsr,
            'mur': mat.mur,
            'sigma': mat.sigma,
            'type': 'conductor' if mat.is_conductor() else 'dielectric' if mat.is_dielectric() else 'magnetic' if mat.is_magnetic() else 'other'
        } for name, mat in materials.items()},
        'analysis_frequency': frequencies.tolist()
    }
    
    with open('demo_material_data.json', 'w') as f:
        import json
        json.dump(material_data, f, indent=2)
    
    print("✅ 材料分析演示完成！")


def demo_frequency_sweep():
    """演示频率扫描分析"""
    print("\n📈 频率扫描分析演示")
    print("=" * 50)
    
    # 定义频率范围
    frequencies = np.logspace(8, 11, 21)  # 100MHz - 100GHz
    
    # 定义材料
    materials = {
        'PEC': Material('PEC', epsr=1.0, mur=1.0, sigma=1e20)
    }
    
    # 存储结果
    mom_results_freq = []
    peec_results_freq = []
    
    print("🔍 进行频率扫描分析...")
    
    for i, freq in enumerate(frequencies):
        print(f"   频率 {i+1}/{len(frequencies)}: {freq/1e9:.2f} GHz")
        
        # 创建仿真器
        simulator = MoMPeecSimulator(ProfessionalMoMSolver, ProfessionalPEECSolver)
        simulator.setup_simulation(freq, materials)
        
        # 激励
        plane_wave = Excitation(
            type='plane_wave',
            frequency=freq,
            amplitude=1.0,
            direction=np.array([1, 0, 0]),
            polarization=np.array([0, 1, 0])
        )
        
        try:
            # MoM仿真
            mom_result = simulator.run_mom_simulation(
                'tests/test_hpm/weixing_v1.stl', plane_wave,
                target_scale=2.8, stl_units='mm', max_facets=500
            )
            
            # PEEC仿真
            peec_result = simulator.run_peec_simulation(
                'tests/test_hpm/weixing_v1.stl', [plane_wave],
                target_scale=2.8, stl_units='mm', max_facets=500
            )
            
            # 记录结果
            if mom_result.currents is not None:
                mom_current_mag = np.mean(np.abs(mom_result.currents))
                mom_results_freq.append(mom_current_mag)
            else:
                mom_results_freq.append(0)
            
            if peec_result.currents is not None:
                peec_current_mag = np.mean(np.abs(peec_result.currents))
                peec_results_freq.append(peec_current_mag)
            else:
                peec_results_freq.append(0)
                
        except Exception as e:
            print(f"   ⚠️ 频率 {freq/1e9:.2f} GHz 仿真失败: {e}")
            mom_results_freq.append(0)
            peec_results_freq.append(0)
    
    # 绘制频率响应
    plt.figure(figsize=(12, 8))
    
    plt.subplot(2, 1, 1)
    plt.semilogx(frequencies/1e9, mom_results_freq, 'b-o', label='MoM', linewidth=2, markersize=6)
    plt.xlabel('频率 (GHz)')
    plt.ylabel('平均电流幅度 (A)')
    plt.title('MoM频率响应')
    plt.legend()
    plt.grid(True, alpha=0.3)
    
    plt.subplot(2, 1, 2)
    plt.semilogx(frequencies/1e9, peec_results_freq, 'r-o', label='PEEC', linewidth=2, markersize=6)
    plt.xlabel('频率 (GHz)')
    plt.ylabel('平均电流幅度 (A)')
    plt.title('PEEC频率响应')
    plt.legend()
    plt.grid(True, alpha=0.3)
    
    plt.tight_layout()
    plt.savefig('demo_frequency_sweep.png', dpi=150, bbox_inches='tight')
    plt.close()
    
    print("✅ 频率响应图已保存: demo_frequency_sweep.png")
    
    # 保存频率扫描数据
    sweep_data = {
        'frequencies': frequencies.tolist(),
        'mom_results': mom_results_freq,
        'peec_results': peec_results_freq
    }
    
    with open('demo_frequency_sweep.json', 'w') as f:
        import json
        json.dump(sweep_data, f, indent=2)
    
    print("✅ 频率扫描分析演示完成！")


def main():
    """主函数"""
    print("🚀 通用MoM/PEEC电磁仿真框架")
    print("=" * 60)
    print("该演示程序展示了通用MoM/PEEC框架的主要功能:")
    print("1. 基础MoM和PEEC仿真")
    print("2. MoM-PEEC耦合仿真")
    print("3. 多种测试案例")
    print("4. 材料分析")
    print("5. 频率扫描分析")
    print("=" * 60)
    
    try:
        # 运行所有演示
        demo_basic_simulation()
        demo_coupled_simulation()
        demo_test_framework()
        demo_material_analysis()
        demo_frequency_sweep()
        
        print("\n" + "=" * 60)
        print("🎉 所有演示完成！")
        print("📁 生成的文件:")
        print("   - demo_basic_results.json")
        print("   - demo_mom_currents.png")
        print("   - demo_peec_currents.png")
        print("   - demo_coupled_results.json")
        print("   - demo_coupling_results.png")
        print("   - demo_test_framework_results.json")
        print("   - test_results_summary.png")
        print("   - demo_material_analysis.png")
        print("   - demo_material_data.json")
        print("   - demo_frequency_sweep.png")
        print("   - demo_frequency_sweep.json")
        print("=" * 60)
        
    except KeyboardInterrupt:
        print("\n⚠️ 演示被用户中断")
        sys.exit(0)
    except Exception as e:
        print(f"\n❌ 演示过程中出现错误: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)


if __name__ == "__main__":
    main()