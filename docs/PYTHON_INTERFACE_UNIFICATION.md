# Python Interface 统一化分析

## 问题分析

### 当前问题

1. **命名问题**: `satellite_mom_peec_ctypes.py` 包含"satellite"字样，但实际上应该是通用的MoM/PEEC接口
2. **接口重复**: 存在多个Python接口，功能可能有重叠：
   - `python_interface/satellite_mom_peec_ctypes.py` - ctypes接口
   - `python/tools/c_solver_interface.py` - C求解器接口（带编译支持）
   - `python/tools/integrated_c_solver_interface.py` - 集成接口（ctypes + subprocess）
   - `python/tools/c_executable_interface.py` - 可执行文件接口（subprocess）

### 接口功能对比

| 接口 | 位置 | 主要功能 | 特点 |
|------|------|----------|------|
| `satellite_mom_peec_ctypes.py` | `python_interface/` | ctypes直接调用C库 | 简单、直接，需要预编译库 |
| `c_solver_interface.py` | `python/tools/` | ctypes + 自动编译 | 支持自动编译，更复杂 |
| `integrated_c_solver_interface.py` | `python/tools/` | 统一接口（ctypes + subprocess） | 多后端支持 |
| `c_executable_interface.py` | `python/tools/` | subprocess调用可执行文件 | 更稳健，但性能较低 |

## 统一方案

### 原则

1. **主接口**: `python_interface/` 目录下的接口应该是通用的、主要的接口
2. **工具接口**: `python/tools/` 目录下的接口作为辅助工具，提供额外功能
3. **命名规范**: 移除"satellite"等特定应用名称，使用通用名称
4. **功能分离**: 
   - 主接口：简单、直接、通用
   - 工具接口：高级功能（编译、多后端、对比等）

### 重命名方案

1. **主接口重命名**:
   - `satellite_mom_peec_ctypes.py` → `mom_peec_ctypes.py`
   - `SatelliteMoMPEECInterface` → `MoMPEECInterface`
   - `run_satellite_c_simulation()` → `run_simulation()`

2. **保持工具接口**:
   - `python/tools/c_solver_interface.py` - 保留（提供编译支持）
   - `python/tools/integrated_c_solver_interface.py` - 保留（多后端支持）
   - `python/tools/c_executable_interface.py` - 保留（subprocess支持）

### 接口职责划分

#### 主接口 (`python_interface/mom_peec_ctypes.py`)

**职责**: 
- 提供简单、直接的ctypes接口
- 调用预编译的C库
- 通用MoM/PEEC仿真接口

**适用场景**:
- 标准测试
- 生产环境使用
- 需要高性能的场景

#### 工具接口 (`python/tools/`)

**职责**:
- 提供高级功能（编译、多后端、对比）
- 开发环境支持
- 调试和验证工具

**适用场景**:
- 开发环境
- 需要自动编译
- 多后端对比测试
- 调试和诊断

## 实施步骤

1. ✅ 重命名主接口文件和类
2. ✅ 更新所有引用
3. ✅ 更新文档
4. ✅ 更新skills配置
