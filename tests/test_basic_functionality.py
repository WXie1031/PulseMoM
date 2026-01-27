#!/usr/bin/env python3
"""
基础功能测试脚本

测试Python interface的基本功能，包括：
1. 库加载
2. 基本API调用
3. 错误处理
"""

import sys
import os
from pathlib import Path

project_root = Path(__file__).parent.parent
sys.path.insert(0, str(project_root))

from python_interface.mom_peec_ctypes import MoMPEECInterface


def test_library_loading():
    """测试库加载"""
    print("测试1: 库加载...")
    try:
        interface = SatelliteMoMPEECInterface()
        version = interface.get_version()
        print(f"  ✓ 库加载成功，版本: {version}")
        return True
    except Exception as e:
        print(f"  ✗ 库加载失败: {e}")
        return False


def test_simulation_lifecycle():
    """测试仿真实例生命周期"""
    print("\n测试2: 仿真实例生命周期...")
    try:
        interface = SatelliteMoMPEECInterface()
        sim_handle = interface.create_simulation()
        print(f"  ✓ 仿真实例创建成功，句柄: {sim_handle}")
        interface.destroy_simulation(sim_handle)
        print(f"  ✓ 仿真实例销毁成功")
        return True
    except Exception as e:
        print(f"  ✗ 测试失败: {e}")
        return False


def test_error_handling():
    """测试错误处理"""
    print("\n测试3: 错误处理...")
    try:
        interface = SatelliteMoMPEECInterface()
        sim_handle = interface.create_simulation()
        
        # 尝试加载不存在的文件
        error = interface.load_stl(sim_handle, "nonexistent.stl")
        if error != 0:
            error_msg = interface.get_error_string(error)
            print(f"  ✓ 错误处理正常，错误码: {error}, 消息: {error_msg}")
            interface.destroy_simulation(sim_handle)
            return True
        else:
            print(f"  ✗ 应该返回错误但没有")
            interface.destroy_simulation(sim_handle)
            return False
    except Exception as e:
        print(f"  ✗ 测试失败: {e}")
        return False


def main():
    """主函数"""
    print("=" * 80)
    print("基础功能测试")
    print("=" * 80)
    
    results = []
    results.append(("库加载", test_library_loading()))
    results.append(("仿真实例生命周期", test_simulation_lifecycle()))
    results.append(("错误处理", test_error_handling()))
    
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
