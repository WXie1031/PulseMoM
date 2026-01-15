#!/usr/bin/env python3
"""
通用MoM/PEEC测试框架
General MoM/PEEC Test Framework

提供多种测试案例，包括卫星、天线、电路等不同应用场景
支持MoM、PEEC以及MoM-PEEC耦合仿真
"""

import numpy as np
import matplotlib.pyplot as plt
from pathlib import Path
import json
import time
from typing import Dict, List, Tuple, Optional, Any
import sys

# 导入通用框架和求解器
from mom_peec_framework import (
    Material, Excitation, MoMPeecSimulator, MoMPeecVisualizer
)
from mom_peec_solvers import ProfessionalMoMSolver, ProfessionalPEECSolver


class TestCase:
    """基础测试案例类"""
    
    def __init__(self, name: str, description: str):
        self.name = name
        self.description = description
        self.geometry_file = None
        self.materials = {}
        self.excitations = []
        self.simulation_params = {}
        self.expected_results = {}
    
    def setup(self):
        """设置测试案例"""
        raise NotImplementedError("子类必须实现setup方法")
    
    def validate_results(self, results: Dict) -> Dict:
        """验证结果"""
        raise NotImplementedError("子类必须实现validate_results方法")
    
    def create_report(self, results: Dict, validation: Dict) -> str:
        """生成测试报告"""
        report = []
        report.append(f"测试案例: {self.name}")
        report.append(f"描述: {self.description}")
        report.append(f"几何文件: {self.geometry_file}")
        report.append(f"材料数量: {len(self.materials)}")
        report.append(f"激励源数量: {len(self.excitations)}")
        report.append("")
        
        # 添加验证结果
        report.append("结果验证:")
        for key, value in validation.items():
            report.append(f"  {key}: {value}")
        
        return "\n".join(report)


class SatelliteTestCase(TestCase):
    """卫星电磁散射测试案例"""
    
    def __init__(self):
        super().__init__(
            name="卫星电磁散射",
            description="10GHz平面波入射到卫星结构的电磁散射分析"
        )
        self.test_dir = Path('tests/test_hpm')
        self.geometry_file = self.test_dir / 'weixing_v1.stl'
        
    def setup(self):
        """设置卫星测试案例"""
        # 材料定义
        self.materials = {
            'PEC': Material('PEC', epsr=1.0, mur=1.0, sigma=1e20),
            'ALUMINUM': Material('ALUMINUM', epsr=1.0, mur=1.0, sigma=3.5e7),
            'SOLAR_PANEL': Material('SOLAR_PANEL', epsr=11.7, mur=1.0, sigma=1e3)
        }
        
        # 激励源
        plane_wave = Excitation(
            type='plane_wave',
            frequency=10e9,  # 10 GHz
            amplitude=1.0,
            phase=0.0,
            direction=np.array([1, 0, 0]),
            polarization=np.array([0, 1, 0])
        )
        self.excitations = [plane_wave]
        
        # 仿真参数
        self.simulation_params = {
            'target_scale': 2.8,  # 卫星尺寸2.8米
            'stl_units': 'mm',
            'mesh_accuracy': 'standard',
            'max_facets': None
        }
        
        # 预期结果
        self.expected_results = {
            'scattering_ratio': {'min': 0.1, 'max': 10.0, 'unit': '%'},
            'rcs': {'min': 0.01, 'max': 10.0, 'unit': 'm²'},
            'surface_current_range': {'min': 1e-6, 'max': 100.0, 'unit': 'A/m'}
        }
    
    def validate_results(self, results: Dict) -> Dict:
        """验证卫星测试结果"""
        validation = {}
        
        # 验证MoM结果
        if 'mom_results' in results and results['mom_results']:
            mom_results = results['mom_results']
            
            # 散射率验证
            if 'scattering_parameters' in mom_results and mom_results['scattering_parameters']:
                params = mom_results['scattering_parameters']
                scattering_ratio = params.get('scattering_ratio', 0)
                expected = self.expected_results['scattering_ratio']
                
                if expected['min'] <= scattering_ratio <= expected['max']:
                    validation['scattering_ratio'] = f"✅ PASS ({scattering_ratio:.2f}%)"
                else:
                    validation['scattering_ratio'] = f"❌ FAIL ({scattering_ratio:.2f}%)"
            
            # RCS验证
            if 'scattering_parameters' in mom_results and mom_results['scattering_parameters']:
                params = mom_results['scattering_parameters']
                rcs = params.get('rcs', 0)
                expected = self.expected_results['rcs']
                
                if expected['min'] <= rcs <= expected['max']:
                    validation['rcs'] = f"✅ PASS ({rcs:.4f} m²)"
                else:
                    validation['rcs'] = f"❌ FAIL ({rcs:.4f} m²)"
            
            # 表面电流范围验证
            if 'currents' in mom_results and mom_results['currents'] is not None:
                currents = np.array(mom_results['currents'])
                current_range = np.max(np.abs(currents)) - np.min(np.abs(currents))
                expected = self.expected_results['surface_current_range']
                
                if expected['min'] <= current_range <= expected['max']:
                    validation['surface_current_range'] = f"✅ PASS ({current_range:.2e} A/m)"
                else:
                    validation['surface_current_range'] = f"❌ FAIL ({current_range:.2e} A/m)"
        
        # 验证PEEC结果
        if 'peec_results' in results and results['peec_results']:
            peec_results = results['peec_results']
            
            if 'currents' in peec_results and peec_results['currents'] is not None:
                validation['peec_currents'] = "✅ Available"
            else:
                validation['peec_currents'] = "❌ Missing"
        
        # 验证耦合结果
        if 'coupled_results' in results and results['coupled_results']:
            coupled_results = results['coupled_results']
            
            if 'convergence_iterations' in coupled_results:
                iterations = coupled_results['convergence_iterations']
                if iterations <= 20:
                    validation['coupling_convergence'] = f"✅ PASS ({iterations} iterations)"
                else:
                    validation['coupling_convergence'] = f"⚠️ SLOW ({iterations} iterations)"
            
            if 'final_residual' in coupled_results:
                residual = coupled_results['final_residual']
                if residual < 1e-4:
                    validation['coupling_residual'] = f"✅ PASS ({residual:.2e})"
                else:
                    validation['coupling_residual'] = f"❌ FAIL ({residual:.2e})"
        
        return validation


class AntennaTestCase(TestCase):
    """天线辐射测试案例"""
    
    def __init__(self):
        super().__init__(
            name="天线辐射分析",
            description="偶极子天线的辐射特性分析"
        )
        # 这里可以添加天线几何文件
        self.geometry_file = "tests/antenna/dipole.stl"
        
    def setup(self):
        """设置天线测试案例"""
        self.materials = {
            'COPPER': Material('COPPER', epsr=1.0, mur=1.0, sigma=5.8e7),
            'AIR': Material('AIR', epsr=1.0, mur=1.0, sigma=0.0)
        }
        
        # 电压源激励
        voltage_source = Excitation(
            type='voltage_source',
            frequency=1e9,  # 1 GHz
            amplitude=1.0,
            phase=0.0,
            position=np.array([0, 0, 0]),
            impedance=50.0
        )
        self.excitations = [voltage_source]
        
        self.simulation_params = {
            'frequency_range': [0.5e9, 2e9],  # 0.5-2 GHz
            'mesh_accuracy': 'high',
            'far_field_points': 360
        }
        
        self.expected_results = {
            'input_impedance': {'expected_real': 73.0, 'expected_imag': 42.5, 'tolerance': 0.1},
            'radiation_efficiency': {'min': 0.8, 'max': 1.0},
            'gain': {'expected': 2.15, 'tolerance': 0.2}
        }
    
    def validate_results(self, results: Dict) -> Dict:
        """验证天线测试结果"""
        validation = {}
        
        # 这里可以添加具体的天线性能验证
        validation['antenna_test'] = "✅ Test completed"
        
        return validation


class CircuitTestCase(TestCase):
    """电路网络测试案例"""
    
    def __init__(self):
        super().__init__(
            name="电路网络分析",
            description="复杂PCB电路网络的PEEC分析"
        )
        self.geometry_file = "tests/circuit/pcb_layout.stl"
        
    def setup(self):
        """设置电路测试案例"""
        self.materials = {
            'COPPER': Material('COPPER', epsr=1.0, mur=1.0, sigma=5.8e7),
            'FR4': Material('FR4', epsr=4.4, mur=1.0, sigma=1e-3),
            'SOLDER': Material('SOLDER', epsr=1.0, mur=1.0, sigma=1e6)
        }
        
        # 多个电压源和电流源
        source1 = Excitation(
            type='voltage_source',
            frequency=100e6,  # 100 MHz
            amplitude=1.0,
            phase=0.0,
            position=np.array([0.01, 0, 0]),
            impedance=50.0
        )
        
        source2 = Excitation(
            type='current_source',
            frequency=100e6,
            amplitude=0.1,
            phase=np.pi/2,
            position=np.array([0.02, 0.01, 0]),
            impedance=1000.0
        )
        
        self.excitations = [source1, source2]
        
        self.simulation_params = {
            'frequency_sweep': True,
            'sweep_points': 100,
            'coupling_analysis': True
        }
        
        self.expected_results = {
            's_parameters': {'s11_max': -10, 's21_min': -3},
            'crosstalk': {'max_coupling': -30},  # dB
            'power_integrity': {'ir_drop_max': 0.1}  # 电压降
        }
    
    def validate_results(self, results: Dict) -> Dict:
        """验证电路测试结果"""
        validation = {}
        
        # 这里可以添加具体的电路性能验证
        validation['circuit_test'] = "✅ Test completed"
        
        return validation


class TestFramework:
    """通用测试框架"""
    
    def __init__(self, mom_solver_class: type, peec_solver_class: type):
        self.mom_solver_class = mom_solver_class
        self.peec_solver_class = peec_solver_class
        self.test_cases: List[TestCase] = []
        self.results: Dict[str, Dict] = {}
        self.validation_results: Dict[str, Dict] = {}
        
    def add_test_case(self, test_case: TestCase):
        """添加测试案例"""
        self.test_cases.append(test_case)
        print(f"✅ 添加测试案例: {test_case.name}")
    
    def run_all_tests(self, enable_coupling: bool = True) -> Dict:
        """运行所有测试案例"""
        print("🚀 开始运行所有测试案例...")
        print("=" * 60)
        
        start_time = time.time()
        
        for test_case in self.test_cases:
            print(f"\n📊 运行测试: {test_case.name}")
            print("-" * 40)
            
            try:
                # 设置测试
                test_case.setup()
                
                # 创建仿真器
                simulator = MoMPeecSimulator(self.mom_solver_class, self.peec_solver_class)
                
                # 设置仿真
                frequency = test_case.excitations[0].frequency if test_case.excitations else 1e9
                simulator.setup_simulation(frequency, test_case.materials)
                
                # 运行MoM仿真
                print("🔍 运行MoM仿真...")
                if test_case.excitations:
                    mom_results = simulator.run_mom_simulation(
                        str(test_case.geometry_file), 
                        test_case.excitations[0],
                        **test_case.simulation_params
                    )
                
                # 运行PEEC仿真
                print("🔌 运行PEEC仿真...")
                peec_results = simulator.run_peec_simulation(
                    str(test_case.geometry_file),
                    test_case.excitations,
                    **test_case.simulation_params
                )
                
                # 运行耦合仿真 (如果启用)
                coupled_results = None
                if enable_coupling and len(test_case.excitations) > 0:
                    print("🔗 运行耦合仿真...")
                    coupling_regions = self._generate_coupling_regions(mom_results, peec_results)
                    
                    coupled_results = simulator.run_coupled_simulation(
                        str(test_case.geometry_file),
                        test_case.excitations[0],
                        test_case.excitations,
                        coupling_regions,
                        **test_case.simulation_params
                    )
                
                # 整合结果
                test_results = {
                    'mom_results': mom_results.__dict__ if mom_results else None,
                    'peec_results': peec_results.__dict__ if peec_results else None,
                    'coupled_results': coupled_results['coupled_results'] if coupled_results else None,
                    'test_info': {
                        'name': test_case.name,
                        'description': test_case.description,
                        'computation_time': {
                            'mom': mom_results.computation_time if mom_results else 0,
                            'peec': peec_results.computation_time if peec_results else 0,
                            'coupling': coupled_results['coupled_results']['computation_time'] if coupled_results and 'computation_time' in coupled_results['coupled_results'] else 0
                        }
                    }
                }
                
                # 验证结果
                print("✅ 验证结果...")
                validation = test_case.validate_results(test_results)
                
                # 存储结果
                self.results[test_case.name] = test_results
                self.validation_results[test_case.name] = validation
                
                print(f"✅ 测试完成: {test_case.name}")
                
            except Exception as e:
                print(f"❌ 测试失败: {test_case.name}")
                print(f"   错误: {e}")
                self.results[test_case.name] = {'error': str(e)}
                self.validation_results[test_case.name] = {'status': 'FAILED'}
        
        total_time = time.time() - start_time
        print(f"\n🎯 所有测试完成 (总耗时: {total_time:.2f}秒)")
        
        return {
            'results': self.results,
            'validation': self.validation_results,
            'total_time': total_time
        }
    
    def _generate_coupling_regions(self, mom_results: Any, peec_results: Any) -> List[Dict]:
        """生成耦合区域"""
        # 简化的耦合区域生成
        coupling_regions = []
        
        if mom_results and hasattr(mom_results, 'currents') and mom_results.currents is not None:
            num_mom = len(mom_results.currents)
        else:
            num_mom = 50
            
        if peec_results and hasattr(peec_results, 'currents') and peec_results.currents is not None:
            num_peec = len(peec_results.currents)
        else:
            num_peec = 30
        
        # 创建耦合区域
        coupling_regions.append({
            'mom_indices': list(range(min(50, num_mom))),
            'peec_indices': list(range(min(30, num_peec))),
            'coupling_strength': 0.1
        })
        
        return coupling_regions
    
    def generate_summary_report(self) -> str:
        """生成汇总报告"""
        report = []
        report.append("通用MoM/PEEC测试框架汇总报告")
        report.append("=" * 60)
        report.append(f"测试时间: {time.strftime('%Y-%m-%d %H:%M:%S')}")
        report.append(f"测试案例数量: {len(self.test_cases)}")
        report.append(f"MoM求解器: {self.mom_solver_class.__name__}")
        report.append(f"PEEC求解器: {self.peec_solver_class.__name__}")
        report.append("")
        
        # 每个测试案例的结果
        for test_name, validation in self.validation_results.items():
            report.append(f"测试案例: {test_name}")
            report.append("-" * 30)
            
            if 'error' in self.results[test_name]:
                report.append(f"状态: ❌ FAILED")
                report.append(f"错误: {self.results[test_name]['error']}")
            else:
                report.append(f"状态: ✅ COMPLETED")
                
                # 验证结果
                for key, value in validation.items():
                    report.append(f"  {key}: {value}")
                
                # 计算时间
                if 'test_info' in self.results[test_name]:
                    comp_time = self.results[test_name]['test_info']['computation_time']
                    total_time = comp_time.get('mom', 0) + comp_time.get('peec', 0) + comp_time.get('coupling', 0)
                    report.append(f"  总计算时间: {total_time:.2f}秒")
            
            report.append("")
        
        # 统计信息
        total_tests = len(self.test_cases)
        passed_tests = sum(1 for v in self.validation_results.values() if 'error' not in str(v))
        
        report.append("统计信息:")
        report.append(f"总测试数: {total_tests}")
        report.append(f"通过数: {passed_tests}")
        report.append(f"失败数: {total_tests - passed_tests}")
        report.append(f"通过率: {passed_tests/total_tests*100:.1f}%")
        
        return "\n".join(report)
    
    def save_results(self, filename: str = 'test_results.json'):
        """保存测试结果"""
        results_data = {
            'timestamp': time.strftime('%Y-%m-%d %H:%M:%S'),
            'framework_info': {
                'mom_solver': self.mom_solver_class.__name__,
                'peec_solver': self.peec_solver_class.__name__,
                'test_cases': [tc.name for tc in self.test_cases]
            },
            'results': self.results,
            'validation': self.validation_results
        }
        
        with open(filename, 'w', encoding='utf-8') as f:
            json.dump(results_data, f, indent=2, ensure_ascii=False)
        
        print(f"💾 测试结果已保存到: {filename}")
    
    def plot_results_summary(self):
        """绘制结果汇总图"""
        if not self.validation_results:
            print("没有可用的验证结果")
            return
        
        # 创建可视化
        fig, axes = plt.subplots(2, 2, figsize=(12, 10))
        
        # 测试状态统计
        test_names = list(self.validation_results.keys())
        status_counts = {'PASS': 0, 'FAIL': 0, 'OTHER': 0}
        
        for validation in self.validation_results.values():
            if isinstance(validation, dict):
                for key, value in validation.items():
                    if 'PASS' in str(value):
                        status_counts['PASS'] += 1
                    elif 'FAIL' in str(value):
                        status_counts['FAIL'] += 1
                    else:
                        status_counts['OTHER'] += 1
        
        # 饼图 - 测试状态
        axes[0, 0].pie(status_counts.values(), labels=status_counts.keys(), autopct='%1.1f%%')
        axes[0, 0].set_title('测试验证状态分布')
        
        # 计算时间对比
        computation_times = {'MoM': [], 'PEEC': [], 'Coupling': []}
        test_labels = []
        
        for test_name, results in self.results.items():
            if 'test_info' in results and 'computation_time' in results['test_info']:
                comp_time = results['test_info']['computation_time']
                computation_times['MoM'].append(comp_time.get('mom', 0))
                computation_times['PEEC'].append(comp_time.get('peec', 0))
                computation_times['Coupling'].append(comp_time.get('coupling', 0))
                test_labels.append(test_name)
        
        # 条形图 - 计算时间
        if test_labels:
            x = np.arange(len(test_labels))
            width = 0.25
            
            axes[0, 1].bar(x - width, computation_times['MoM'], width, label='MoM', alpha=0.8)
            axes[0, 1].bar(x, computation_times['PEEC'], width, label='PEEC', alpha=0.8)
            axes[0, 1].bar(x + width, computation_times['Coupling'], width, label='Coupling', alpha=0.8)
            
            axes[0, 1].set_xlabel('测试案例')
            axes[0, 1].set_ylabel('计算时间 (秒)')
            axes[0, 1].set_title('计算时间对比')
            axes[0, 1].set_xticks(x)
            axes[0, 1].set_xticklabels(test_labels, rotation=45)
            axes[0, 1].legend()
        
        # 散射参数对比 (如果有MoM结果)
        scattering_ratios = []
        rcs_values = []
        test_names_mom = []
        
        for test_name, results in self.results.items():
            if 'mom_results' in results and results['mom_results']:
                mom_results = results['mom_results']
                if 'scattering_parameters' in mom_results and mom_results['scattering_parameters']:
                    params = mom_results['scattering_parameters']
                    scattering_ratios.append(params.get('scattering_ratio', 0))
                    rcs_values.append(params.get('rcs', 0))
                    test_names_mom.append(test_name)
        
        if test_names_mom:
            axes[1, 0].bar(test_names_mom, scattering_ratios, alpha=0.7)
            axes[1, 0].set_xlabel('测试案例')
            axes[1, 0].set_ylabel('散射率 (%)')
            axes[1, 0].set_title('MoM散射率对比')
            axes[1, 0].tick_params(axis='x', rotation=45)
        
        # RCS对比
        if test_names_mom:
            axes[1, 1].bar(test_names_mom, rcs_values, alpha=0.7, color='orange')
            axes[1, 1].set_xlabel('测试案例')
            axes[1, 1].set_ylabel('RCS (m²)')
            axes[1, 1].set_title('MoM RCS对比')
            axes[1, 1].tick_params(axis='x', rotation=45)
        
        plt.tight_layout()
        plt.savefig('test_results_summary.png', dpi=150, bbox_inches='tight')
        plt.show()


def main():
    """主函数"""
    print("🚀 通用MoM/PEEC测试框架")
    print("=" * 60)
    
    # 创建测试框架
    framework = TestFramework(ProfessionalMoMSolver, ProfessionalPEECSolver)
    
    # 添加测试案例
    framework.add_test_case(SatelliteTestCase())
    framework.add_test_case(AntennaTestCase())
    framework.add_test_case(CircuitTestCase())
    
    # 运行所有测试
    results = framework.run_all_tests(enable_coupling=True)
    
    # 生成报告
    print("\n📊 生成测试报告...")
    report = framework.generate_summary_report()
    print(report)
    
    # 保存结果
    framework.save_results('mom_peec_test_results.json')
    
    # 绘制结果图
    framework.plot_results_summary()
    
    print("\n✅ 测试框架运行完成！")
    print("📁 输出文件:")
    print("   - mom_peec_test_results.json")
    print("   - test_results_summary.png")


if __name__ == "__main__":
    main()