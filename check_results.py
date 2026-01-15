#!/usr/bin/env python3
"""Check the simulation results from the JSON file"""

import json
import os

def check_simulation_results():
    """Check the simulation results"""
    
    json_file = 'satellite_mom_peec_simulation_data.json'
    if not os.path.exists(json_file):
        print(f"❌ 结果文件 {json_file} 不存在")
        return
    
    try:
        with open(json_file, 'r') as f:
            data = json.load(f)
        
        print("📊 仿真结果摘要:")
        print("="*50)
        
        # Basic parameters
        frequency = data.get('frequency', 0)
        wavelength = data.get('wavelength', 0)
        print(f"频率: {frequency/1e9:.1f} GHz")
        print(f"波长: {wavelength:.4f} m")
        
        # Surface currents
        surface_currents = data.get('surface_currents', [])
        if surface_currents:
            currents = [complex(c['real'], c['imag']) for c in surface_currents]
            current_magnitudes = [abs(c) for c in currents]
            print(f"表面电流数量: {len(currents)}")
            print(f"电流范围: {min(current_magnitudes):.2e} - {max(current_magnitudes):.2e} A/m")
            print(f"平均电流: {sum(current_magnitudes)/len(current_magnitudes):.2e} A/m")
        
        # Fields
        incident_fields = data.get('incident_fields', [])
        scattered_fields = data.get('scattered_fields', [])
        total_fields = data.get('total_fields', [])
        
        if incident_fields and scattered_fields and total_fields:
            inc_magnitudes = [abs(complex(f['real'], f['imag'])) for f in incident_fields]
            scat_magnitudes = [abs(complex(f['real'], f['imag'])) for f in scattered_fields]
            
            print(f"观测点数量: {len(incident_fields)}")
            print(f"入射场范围: {min(inc_magnitudes):.2e} - {max(inc_magnitudes):.2e} V/m")
            print(f"散射场范围: {min(scat_magnitudes):.2e} - {max(scat_magnitudes):.2e} V/m")
            
            if max(inc_magnitudes) > 0:
                scattering_ratio = max(scat_magnitudes) / max(inc_magnitudes) * 100
                print(f"散射比例: {scattering_ratio:.2f}%")
        
        # Check for C solver results
        if 'c_solver_report' in data:
            print(f"\n🔧 C求解器报告:")
            c_report = data['c_solver_report']
            print(f"首选后端: {c_report.get('preferred_backend', 'N/A')}")
            
            ctypes_info = c_report.get('ctypes', {})
            print(f"ctypes后端:")
            print(f"  MoM可用: {ctypes_info.get('mom_available', False)}")
            print(f"  PEEC可用: {ctypes_info.get('peec_available', False)}")
            print(f"  网格引擎可用: {ctypes_info.get('mesh_available', False)}")
            
            subprocess_info = c_report.get('subprocess', {})
            print(f"subprocess后端:")
            executables = subprocess_info.get('executables_available', [])
            print(f"  可用执行文件: {', '.join(executables) if executables else '无'}")
        
        # Check for solver comparison
        if 'solver_comparison' in data:
            print(f"\n🔍 Python vs C求解器比较:")
            comparison = data['solver_comparison']
            summary = comparison.get('summary', {})
            print(f"整体一致性: {summary.get('overall_agreement', 'N/A')}")
            print(f"MoM求解器类型: {summary.get('mom_c_solver_type', 'N/A')}")
            print(f"PEEC求解器类型: {summary.get('peec_c_solver_type', 'N/A')}")
            
            mom_comparison = comparison.get('mom_comparison', {})
            if mom_comparison:
                print(f"MoM幅度误差: {mom_comparison.get('magnitude_error', 'N/A'):.2e}")
                print(f"MoM相位误差: {mom_comparison.get('phase_error', 'N/A'):.2e}")
            
            peec_comparison = comparison.get('peec_comparison', {})
            if peec_comparison:
                print(f"PEEC电压误差: {peec_comparison.get('voltage_error', 'N/A'):.2e}")
        
        print("\n" + "="*50)
        print("✅ 结果检查完成")
        
    except Exception as e:
        print(f"❌ 检查失败: {e}")
        import traceback
        traceback.print_exc()

if __name__ == '__main__':
    check_simulation_results()