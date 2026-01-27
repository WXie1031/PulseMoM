#!/usr/bin/env python3
"""
材料测试脚本

测试不同材料属性的设置和验证
"""

import sys
import os
from pathlib import Path

project_root = Path(__file__).parent.parent
sys.path.insert(0, str(project_root))

from python_interface.mom_peec_ctypes import MoMPEECInterface


def test_pec_material():
    """测试PEC材料"""
    print("测试1: PEC材料...")
    try:
        interface = MoMPEECInterface()
        sim_handle = interface.create_simulation()
        
        material = {
            'name': 'PEC',
            'eps_r': 1.0,
            'mu_r': 1.0,
            'sigma': 1e20,
            'tan_delta': 0.0,
            'material_id': 1
        }
        
        error = interface.set_material(sim_handle, material)
        if error == 0:
            print("  ✓ PEC材料设置成功")
            interface.destroy_simulation(sim_handle)
            return True
        else:
            print(f"  ✗ PEC材料设置失败: {interface.get_error_string(error)}")
            interface.destroy_simulation(sim_handle)
            return False
    except Exception as e:
        print(f"  ✗ 测试失败: {e}")
        return False


def test_metal_materials():
    """测试金属材料"""
    print("\n测试2: 金属材料...")
    materials = [
        {'name': 'Copper', 'sigma': 5.8e7},
        {'name': 'Aluminum', 'sigma': 3.5e7},
        {'name': 'Gold', 'sigma': 4.1e7}
    ]
    
    try:
        interface = MoMPEECInterface()
        sim_handle = interface.create_simulation()
        
        for mat in materials:
            material = {
                'name': mat['name'],
                'eps_r': 1.0,
                'mu_r': 1.0,
                'sigma': mat['sigma'],
                'tan_delta': 0.0,
                'material_id': 1
            }
            
            error = interface.set_material(sim_handle, material)
            if error == 0:
                print(f"  ✓ {mat['name']}材料设置成功")
            else:
                print(f"  ✗ {mat['name']}材料设置失败")
                interface.destroy_simulation(sim_handle)
                return False
        
        interface.destroy_simulation(sim_handle)
        return True
    except Exception as e:
        print(f"  ✗ 测试失败: {e}")
        return False


def test_dielectric_materials():
    """测试介电材料"""
    print("\n测试3: 介电材料...")
    materials = [
        {'name': 'FR4', 'eps_r': 4.4, 'tan_delta': 0.02},
        {'name': 'Air', 'eps_r': 1.0, 'tan_delta': 0.0},
        {'name': 'Teflon', 'eps_r': 2.1, 'tan_delta': 0.0002}
    ]
    
    try:
        interface = MoMPEECInterface()
        sim_handle = interface.create_simulation()
        
        for mat in materials:
            material = {
                'name': mat['name'],
                'eps_r': mat['eps_r'],
                'mu_r': 1.0,
                'sigma': 0.0,
                'tan_delta': mat['tan_delta'],
                'material_id': 1
            }
            
            error = interface.set_material(sim_handle, material)
            if error == 0:
                print(f"  ✓ {mat['name']}材料设置成功")
            else:
                print(f"  ✗ {mat['name']}材料设置失败")
                interface.destroy_simulation(sim_handle)
                return False
        
        interface.destroy_simulation(sim_handle)
        return True
    except Exception as e:
        print(f"  ✗ 测试失败: {e}")
        return False


def main():
    """主函数"""
    print("=" * 80)
    print("材料测试")
    print("=" * 80)
    
    results = []
    results.append(("PEC材料", test_pec_material()))
    results.append(("金属材料", test_metal_materials()))
    results.append(("介电材料", test_dielectric_materials()))
    
    print("\n" + "=" * 80)
    print("测试结果汇总")
    print("=" * 80)
    
    passed = sum(1 for _, result in results if result)
    total = len(results)
    
    for name, result in results:
        status = "✓ 通过" if result else "✗ 失败"
        print(f"{name}: {status}")
    
    print(f"\n总计: {passed}/{total} 通过")
    
    return 0 if passed == total else 1


if __name__ == "__main__":
    sys.exit(main())
