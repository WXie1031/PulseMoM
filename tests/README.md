# 测试文件说明

## 概述

本目录包含PulseMoM项目的测试文件，包括Python测试和C测试。

## Python测试文件

### 基础测试

1. **test_basic_functionality.py** - 基础功能测试
   - 库加载
   - 仿真实例生命周期
   - 错误处理

2. **test_materials.py** - 材料测试
   - PEC材料
   - 金属材料（铜、铝、金）
   - 介电材料（FR4、空气、特氟龙）

3. **test_cad_import_simulation.py** - CAD导入和仿真测试
   - STL文件导入
   - 网格生成
   - MoM仿真
   - 结果分析

### 运行Python测试

```bash
# 基础功能测试
python tests/test_basic_functionality.py

# 材料测试
python tests/test_materials.py

# CAD导入和仿真测试
python tests/test_cad_import_simulation.py [stl_file] [frequency]
```

## C测试文件

### 核心功能测试

1. **test_cst_materials_parser.c** - CST材料解析器测试
2. **test_mtl_implementation.c** - MTL实现测试
3. **test_pulseem_compilation.c** - 编译测试
4. **test_pulseem_simple.c** - 简单功能测试
5. **test_satellite_multimaterial.c** - 多材料测试
6. **test_typedef.c** - 类型定义测试

### 工具和演示

1. **satellite_coordinate_correction.c** - 坐标校正工具
2. **satellite_peec_test.c** - PEEC测试
3. **comprehensive_mesh_demo.cpp** - 网格生成演示

### 编译和运行C测试

C测试文件需要编译后运行。参考各文件的编译说明。

## 测试覆盖

### 已覆盖

- ✅ 库加载和初始化
- ✅ 仿真实例管理
- ✅ 材料属性设置
- ✅ CAD文件导入（STL）
- ✅ 网格生成
- ✅ 求解器配置
- ✅ 激励设置
- ✅ 仿真执行
- ✅ 结果获取

### 待补充

- ⏳ 多频率扫描
- ⏳ 不同求解器类型对比
- ⏳ 复杂几何测试
- ⏳ 性能基准测试
- ⏳ 边界条件测试

## 使用Python Interface

所有Python测试都使用统一的Python interface：

```python
from python_interface.mom_peec_ctypes import (
    MoMPEECInterface,
    run_simulation
)
```

详细文档请参考：`python_interface/README.md`

## 注意事项

1. 确保C库已编译并可用
2. STL测试文件需要存在
3. 某些测试需要较长时间运行
4. 测试结果保存在 `test_output/` 目录
