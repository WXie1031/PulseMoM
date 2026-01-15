#!/usr/bin/env python3
"""
HDF5文件结构检查工具
"""

import h5py
import os

def inspect_hdf5_file(filename):
    """检查HDF5文件结构"""
    print(f"🔍 检查HDF5文件: {filename}")
    
    try:
        with h5py.File(filename, 'r') as f:
            def print_structure(name, obj):
                if isinstance(obj, h5py.Dataset):
                    print(f"  📊 数据集: {name}")
                    print(f"     形状: {obj.shape}")
                    print(f"     数据类型: {obj.dtype}")
                    if obj.shape == ():  # 标量
                        print(f"     值: {obj[()]}")
                    elif obj.shape[0] < 10:  # 小数组，显示部分数据
                        print(f"     示例数据: {obj[...]}")
                elif isinstance(obj, h5py.Group):
                    print(f"  📁 组: {name}")
            
            print("文件结构:")
            f.visititems(print_structure)
            
    except Exception as e:
        print(f"错误: {e}")

if __name__ == "__main__":
    h5_file = "output_time_domain/time_domain_results.h5"
    if os.path.exists(h5_file):
        inspect_hdf5_file(h5_file)
    else:
        print(f"文件不存在: {h5_file}")