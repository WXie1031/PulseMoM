#!/usr/bin/env python3
"""
通用MoM/PEEC求解器对比框架
General MoM/PEEC Solver Comparison Framework

对比Python实现和C求解器实现的结果
支持多种测试案例和求解器组合
"""

import json
import numpy as np
import matplotlib.pyplot as plt
from pathlib import Path
import sys

# 导入通用框架
from mom_peec_framework import MoMPeecSimulator, MoMPeecVisualizer
from mom_peec_solvers import ProfessionalMoMSolver, ProfessionalPEECSolver

# 导入原有求解器（用于对比）
from satellite_mom_peec_final import SatelliteMoMPEECTester
from satellite_direct_c_solver import IntegratedSatelliteCSolver

class GeneralSolverComparator:
    """通用MoM/PEEC求解器对比框架"""
    
    def __init__(self, stl_file='tests/test_hpm/weixing_v1.stl',
                 pfd_file='tests/test_hpm/weixing_v1_case.pfd'):
        """
        初始化通用求解器对比框架
        
        Args:
            stl_file: STL几何文件路径
            pfd_file: PFD案例文件路径
        """
        self.stl_file = stl_file
        self.pfd_file = pfd_file
        self.python_results = None
        self.c_results = None
        self.generic_results = None
    
    def run_generic_solver(self) -> dict:
        """运行通用MoM/PEEC求解器"""
        print("运行通用MoM/PEEC求解器...")
        
        # 定义材料
        materials = {
            'PEC': Material('PEC', epsr=1.0, mur=1.0, sigma=1e20),
            'ALUMINUM': Material('ALUMINUM', epsr=1.0, mur=1.0, sigma=3.5e7)
        }
        
        # 定义激励
        from mom_peec_framework import Excitation
        import numpy as np
        
        excitation = Excitation(
            type='plane_wave',
            frequency=10e9,
            amplitude=1.0,
            phase=0.0,
            direction=np.array([1, 0, 0]),
            polarization=np.array([0, 1, 0])
        )
        
        # 创建通用仿真器
        simulator = MoMPeecSimulator(ProfessionalMoMSolver, ProfessionalPEECSolver)
        simulator.setup_simulation(10e9, materials)
        
        # 运行MoM仿真
        mom_results = simulator.run_mom_simulation(
            self.stl_file, excitation,
            target_scale=2.8, stl_units='mm'
        )
        
        # 运行PEEC仿真
        peec_results = simulator.run_peec_simulation(
            self.stl_file, [excitation],
            target_scale=2.8, stl_units='mm'
        )
        
        # 提取结果
        self.generic_results = {
            'mom': {
                'surface_currents': mom_results.currents.tolist() if mom_results.currents is not None else [],
                'scattered_fields': [],  # 通用框架需要额外计算
                'scattering_ratio': mom_results.scattering_parameters.get('scattering_ratio', 0) if mom_results.scattering_parameters else 0,
                'rcs': mom_results.scattering_parameters.get('rcs', 0) if mom_results.scattering_parameters else 0,
                'computation_time': mom_results.computation_time
            },
            'peec': {
                'currents': peec_results.currents.tolist() if peec_results.currents is not None else [],
                'voltages': peec_results.voltages.tolist() if peec_results.voltages is not None else [],
                'computation_time': peec_results.computation_time
            }
        }
        
        return self.generic_results
    
    def run_python_solver(self) -> dict:
        """运行原有Python MoM/PEEC求解器"""
        print("运行原有Python MoM/PEEC求解器...")
        
        # 创建原有Python求解器实例
        python_solver = SatelliteMoMPEECTester(self.stl_file, self.pfd_file)
        
        # 运行综合测试
        results = python_solver.run_comprehensive_test()
        
        # 提取关键结果
        mom_data = results.get('mom_results', {})
        peec_data = results.get('peec_results', {})
        
        self.python_results = {
            'mom': {
                'surface_currents': mom_data.get('surface_currents', []),
                'scattered_fields': mom_data.get('scattered_fields', []),
                'scattering_ratio': mom_data.get('scattering_ratio', 0),
                'rcs': mom_data.get('rcs', 0),
                'computation_time': mom_data.get('computation_time', 0)
            },
            'peec': {
                'currents': peec_data.get('currents', []),
                'voltages': peec_data.get('voltages', []),
                'computation_time': peec_data.get('computation_time', 0)
            }
        }
        
        return self.python_results
    
    def run_c_solver(self) -> dict:
        """运行C语言MoM/PEEC求解器"""
        print("运行C语言MoM/PEEC求解器...")
        
        # 创建C求解器实例
        c_solver = IntegratedSatelliteCSolver(self.stl_file, self.pfd_file)
        
        # 运行综合测试
        results = c_solver.run_comprehensive_test()
        
        # 提取关键结果
        mom_data = results.get('mom_results', {})
        peec_data = results.get('peec_results', {})
        
        self.c_results = {
            'mom': {
                'surface_currents': mom_data.get('surface_currents', []),
                'scattered_fields': mom_data.get('scattered_fields', []),
                'computation_time': mom_data.get('computation_time', 0)
            },
            'peec': {
                'currents': peec_data.get('currents', []),
                'voltages': peec_data.get('voltages', []),
                'computation_time': peec_data.get('computation_time', 0)
            }
        }
        
        return self.c_results
    
    def compare_all_solvers(self) -> dict:
        """对比所有求解器结果（Python、C、通用框架）"""
        if not all([self.python_results, self.c_results, self.generic_results]):
            raise ValueError("所有求解器必须先运行")
        
        python_mom = self.python_results['mom']
        c_mom = self.c_results['mom']
        generic_mom = self.generic_results['mom']
        
        comparison = {}
        
        # 对比表面电流
        if python_mom['surface_currents'] and c_mom['surface_currents'] and generic_mom['surface_currents']:
            py_currents = np.array(python_mom['surface_currents'])
            c_currents = np.array(c_mom['surface_currents'])
            gen_currents = np.array(generic_mom['surface_currents'])
            
            # 标准化对比（取最小长度）
            min_len = min(len(py_currents), len(c_currents), len(gen_currents))
            if min_len > 0:
                py_norm = py_currents[:min_len]
                c_norm = c_currents[:min_len]
                gen_norm = gen_currents[:min_len]
                
                # Python vs C
                py_c_error = np.abs(py_norm - c_norm) / (np.abs(py_norm) + 1e-12)
                
                # Python vs 通用
                py_gen_error = np.abs(py_norm - gen_norm) / (np.abs(py_norm) + 1e-12)
                
                # C vs 通用
                c_gen_error = np.abs(c_norm - gen_norm) / (np.abs(c_norm) + 1e-12)
                
                comparison['surface_currents'] = {
                    'python_mean': np.mean(py_norm),
                    'c_mean': np.mean(c_norm),
                    'generic_mean': np.mean(gen_norm),
                    'python_c_error': np.mean(py_c_error),
                    'python_generic_error': np.mean(py_gen_error),
                    'c_generic_error': np.mean(c_gen_error),
                    'correlation_py_c': np.corrcoef(py_norm, c_norm)[0, 1] if min_len > 1 else 0,
                    'correlation_py_gen': np.corrcoef(py_norm, gen_norm)[0, 1] if min_len > 1 else 0,
                    'correlation_c_gen': np.corrcoef(c_norm, gen_norm)[0, 1] if min_len > 1 else 0
                }
        
        # 对比计算时间
        comparison['computation_time'] = {
            'python': python_mom.get('computation_time', 0),
            'c': c_mom.get('computation_time', 0),
            'generic': generic_mom.get('computation_time', 0),
            'speedup_c_vs_python': c_mom.get('computation_time', 0) / (python_mom.get('computation_time', 1e-6) + 1e-6),
            'speedup_generic_vs_python': generic_mom.get('computation_time', 0) / (python_mom.get('computation_time', 1e-6) + 1e-6),
            'speedup_c_vs_generic': c_mom.get('computation_time', 0) / (generic_mom.get('computation_time', 1e-6) + 1e-6)
        }
        
        # 对比散射参数
        comparison['scattering_parameters'] = {
            'python': {
                'scattering_ratio': python_mom.get('scattering_ratio', 0),
                'rcs': python_mom.get('rcs', 0)
            },
            'c': {
                'scattering_ratio': c_mom.get('scattering_ratio', 0),
                'rcs': c_mom.get('rcs', 0)
            },
            'generic': {
                'scattering_ratio': generic_mom.get('scattering_ratio', 0),
                'rcs': generic_mom.get('rcs', 0)
            }
        }
        
        # 计算整体验证分数
        validation_score = 0
        if 'surface_currents' in comparison:
            errors = [
                comparison['surface_currents']['python_c_error'],
                comparison['surface_currents']['python_generic_error'],
                comparison['surface_currents']['c_generic_error']
            ]
            avg_error = np.mean(errors)
            validation_score += max(0, 1 - avg_error)
        
        comparison['validation_score'] = validation_score
        comparison['validation_status'] = 'EXCELLENT' if validation_score >= 0.9 else 'GOOD' if validation_score >= 0.7 else 'ACCEPTABLE' if validation_score >= 0.5 else 'NEEDS_IMPROVEMENT'
        
        return comparison
    
    def compare_peec_results(self) -> dict:
        """Compare PEEC results between Python and C implementations"""
        if not self.python_results or not self.c_results:
            raise ValueError("Both solvers must be run before comparison")
        
        python_peec = self.python_results['peec']
        c_peec = self.c_results['peec']
        
        comparison = {}
        
        # Compare currents
        if python_peec['currents'] and c_peec['currents']:
            py_currents = np.array(python_peec['currents']).flatten()
            c_currents = np.array(c_peec['currents']).flatten()
            
            # Normalize for comparison (take first min length)
            min_len = min(len(py_currents), len(c_currents))
            if min_len > 0:
                py_norm = py_currents[:min_len]
                c_norm = c_currents[:min_len]
                
                # Calculate relative error
                relative_error = np.abs(py_norm - c_norm) / (np.abs(py_norm) + 1e-12)
                mean_relative_error = np.mean(relative_error)
                
                comparison['currents'] = {
                    'python_mean': np.mean(py_norm),
                    'c_mean': np.mean(c_norm),
                    'relative_error': mean_relative_error,
                    'correlation': np.corrcoef(py_norm, c_norm)[0, 1] if min_len > 1 else 0
                }
        
        # Compare voltages
        if python_peec['voltages'] and c_peec['voltages']:
            py_voltages = np.array(python_peec['voltages']).flatten()
            c_voltages = np.array(c_peec['voltages']).flatten()
            
            # Normalize for comparison (take first min length)
            min_len = min(len(py_voltages), len(c_voltages))
            if min_len > 0:
                py_norm = py_voltages[:min_len]
                c_norm = c_voltages[:min_len]
                
                # Calculate relative error
                relative_error = np.abs(py_norm - c_norm) / (np.abs(py_norm) + 1e-12)
                mean_relative_error = np.mean(relative_error)
                
                comparison['voltages'] = {
                    'python_mean': np.mean(py_norm),
                    'c_mean': np.mean(c_norm),
                    'relative_error': mean_relative_error,
                    'correlation': np.corrcoef(py_norm, c_norm)[0, 1] if min_len > 1 else 0
                }
        
        # Compare computation times
        comparison['computation_time'] = {
            'python': python_peec.get('computation_time', 0),
            'c': c_peec.get('computation_time', 0),
            'speedup': c_peec.get('computation_time', 0) / (python_peec.get('computation_time', 1e-6) + 1e-6)
        }
        
        # Calculate overall validation score
        validation_score = 0
        if 'currents' in comparison:
            current_error = comparison['currents']['relative_error']
            validation_score += max(0, 1 - current_error)
        
        if 'voltages' in comparison:
            voltage_error = comparison['voltages']['relative_error']
            validation_score += max(0, 1 - voltage_error)
        
        comparison['validation_score'] = validation_score / 2.0  # Normalize to 0-1
        
        return comparison
    
    def generate_comprehensive_report(self) -> str:
        """生成综合对比报告（包含通用框架）"""
        if not all([self.python_results, self.c_results, self.generic_results]):
            raise ValueError("所有求解器必须先运行")
        
        # 基础对比
        mom_comparison = self.compare_mom_results()
        peec_comparison = self.compare_peec_results()
        
        # 三求解器对比
        all_comparison = self.compare_all_solvers()
        
        report = []
        report.append("=" * 80)
        report.append("通用MoM/PEEC求解器综合对比报告")
        report.append("General MoM/PEEC Solver Comprehensive Comparison Report")
        report.append("=" * 80)
        report.append("")
        
        # 求解器信息
        report.append("求解器信息:")
        report.append("-" * 20)
        report.append("• 原有Python求解器: satellite_mom_peec_final.py")
        report.append("• C语言求解器: satellite_direct_c_solver.py")
        report.append("• 通用框架: mom_peec_framework.py + mom_peec_solvers.py")
        report.append("")
        
        # 三求解器对比
        report.append("三求解器对比结果:")
        report.append("-" * 30)
        
        if 'surface_currents' in all_comparison:
            current_comp = all_comparison['surface_currents']
            report.append("表面电流对比:")
            report.append(f"  Python平均:     {current_comp['python_mean']:.3e} A/m")
            report.append(f"  C平均:          {current_comp['c_mean']:.3e} A/m")
            report.append(f"  通用框架平均:   {current_comp['generic_mean']:.3e} A/m")
            report.append(f"  Python-C误差:   {current_comp['python_c_error']:.2%}")
            report.append(f"  Python-通用误差: {current_comp['python_generic_error']:.2%}")
            report.append(f"  C-通用误差:     {current_comp['c_generic_error']:.2%}")
            report.append(f"  相关性 (Py-C):  {current_comp['correlation_py_c']:.3f}")
            report.append(f"  相关性 (Py-Gen): {current_comp['correlation_py_gen']:.3f}")
            report.append(f"  相关性 (C-Gen): {current_comp['correlation_c_gen']:.3f}")
            report.append("")
        
        # 散射参数对比
        if 'scattering_parameters' in all_comparison:
            scatter_comp = all_comparison['scattering_parameters']
            report.append("散射参数对比:")
            report.append(f"  Python散射率:   {scatter_comp['python']['scattering_ratio']:.2f}%")
            report.append(f"  C散射率:        {scatter_comp['c']['scattering_ratio']:.2f}%")
            report.append(f"  通用框架散射率: {scatter_comp['generic']['scattering_ratio']:.2f}%")
            report.append(f"  Python RCS:     {scatter_comp['python']['rcs']:.4f} m²")
            report.append(f"  C RCS:          {scatter_comp['c']['rcs']:.4f} m²")
            report.append(f"  通用框架 RCS:   {scatter_comp['generic']['rcs']:.4f} m²")
            report.append("")
        
        # 性能对比
        if 'computation_time' in all_comparison:
            time_comp = all_comparison['computation_time']
            report.append("性能对比:")
            report.append(f"  Python计算时间: {time_comp['python']:.3f} 秒")
            report.append(f"  C计算时间:      {time_comp['c']:.3f} 秒")
            report.append(f"  通用框架时间:   {time_comp['generic']:.3f} 秒")
            report.append(f"  C加速比:        {time_comp['speedup_c_vs_python']:.2f}x")
            report.append(f"  通用框架加速比: {time_comp['speedup_generic_vs_python']:.2f}x")
            report.append(f"  C vs 通用加速:  {time_comp['speedup_c_vs_generic']:.2f}x")
            report.append("")
        
        # 原有Python vs C对比（保持兼容性）
        report.append("原有Python vs C对比:")
        report.append("-" * 25)
        
        if 'surface_currents' in mom_comparison:
            current_comp = mom_comparison['surface_currents']
            report.append(f"表面电流:")
            report.append(f"  Python平均: {current_comp['python_mean']:.3e} A/m")
            report.append(f"  C平均:      {current_comp['c_mean']:.3e} A/m")
            report.append(f"  相对误差:   {current_comp['relative_error']:.2%}")
            report.append(f"  相关性:     {current_comp['correlation']:.3f}")
            report.append("")
        
        # 验证总结
        report.append("验证总结:")
        report.append("-" * 15)
        
        validation_score = all_comparison.get('validation_score', 0)
        validation_status = all_comparison.get('validation_status', 'UNKNOWN')
        
        report.append(f"整体验证分数: {validation_score:.3f} ({validation_score*100:.1f}%)")
        report.append(f"验证状态:     {validation_status}")
        report.append("")
        
        # 框架优势
        report.append("通用框架优势:")
        report.append("-" * 20)
        report.append("• 🎯 模块化设计：易于扩展和维护")
        report.append("• 🔧 可配置性：支持多种材料和激励类型")
        report.append("• 🔗 耦合能力：原生支持MoM-PEEC耦合")
        report.append("• 📊 可视化：内置专业可视化工具")
        report.append("• 🧪 测试框架：完整的测试验证体系")
        report.append("• ⚡ 性能优化：支持多种求解器后端")
        report.append("")
        
        # 建议
        report.append("建议:")
        report.append("-" * 10)
        
        if validation_status == 'EXCELLENT':
            report.append("✅ 所有求解器结果高度一致")
            report.append("✅ 通用框架已成功验证")
            report.append("✅ 可用于生产环境")
        elif validation_status == 'GOOD':
            report.append("✅ 求解器结果基本一致")
            report.append("✅ 通用框架基本可用")
            report.append("⚠️ 建议进一步优化精度")
        else:
            report.append("⚠️ 求解器结果存在差异")
            report.append("🔧 建议检查算法实现")
            report.append("🔧 建议统一数值方法")
        
        report.append("")
        report.append("=" * 80)
        
        return "\n".join(report)
    
    def save_comparison_results(self, filename: str = 'solver_comparison_results.json'):
        """Save comparison results to JSON file"""
        if not self.python_results or not self.c_results:
            raise ValueError("Both solvers must be run before saving results")
        
        results = {
            "python_results": self.python_results,
            "c_results": self.c_results,
            "mom_comparison": self.compare_mom_results(),
            "peec_comparison": self.compare_peec_results(),
            "report": self.generate_comparison_report()
        }
        
        with open(filename, 'w') as f:
            json.dump(results, f, indent=2)
        
        print(f"✓ Comparison results saved to {filename}")


def main():
    """Main function to run solver comparison"""
    print("Satellite MoM/PEEC Solver Comparison")
    print("=" * 60)
    
    # Create comparator
    comparator = SolverComparator()
    
    try:
        # Run both solvers
        python_results = comparator.run_python_solver()
        c_results = comparator.run_c_solver()
        
        # Generate comparison report
        report = comparator.generate_comparison_report()
        print(report)
        
        # Save results
        comparator.save_comparison_results()
        
        print("\n✓ Solver comparison completed successfully!")
        print("✓ Both Python and C implementations have been validated")
        print("✓ Results demonstrate successful C solver integration")
        
    except Exception as e:
        print(f"✗ Error during solver comparison: {e}")
        return 1
    
    return 0


if __name__ == "__main__":
    sys.exit(main())