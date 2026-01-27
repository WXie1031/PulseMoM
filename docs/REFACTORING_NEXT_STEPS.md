# 重构下一步工作指南

## 当前状态

### ✅ 已完成
- **架构设计**: 100% 完成
- **接口文件**: 47个文件 ✅
- **实现文件**: 13个文件（L1完成，L3大部分完成）🔄
- **构建系统**: 新CMakeLists.txt已创建 ✅
- **验证工具**: 架构验证脚本已创建 ✅

### 🔄 进行中
- L2离散层实现迁移
- L3算子层实现迁移（大部分完成）
- L4数值后端实现迁移
- L5编排层实现迁移
- L6 IO层实现迁移

## 下一步工作优先级

### 优先级1: 完成L3算子层（几乎完成）
- [x] 格林函数实现
- [x] 积分工具实现
- [x] 奇异积分实现
- [x] MoM积分核实现
- [x] PEEC积分核实现
- [x] 矩阵向量积实现
- [x] 矩阵组装实现
- [ ] 耦合算子实现（待完成）

### 优先级2: GPU代码分离（关键任务）
**问题**: `gpu_acceleration.c` 和 `gpu_parallelization_optimized.cu` 中混合了物理公式和数值实现

**步骤**:
1. 分析GPU代码，识别物理公式部分
2. 物理公式 → `L3_operators/kernels/` (算子层)
3. GPU kernel调用 → `L4_backend/gpu/` (数值后端)
4. 确保GPU代码无物理语义

**文件**:
- `src/core/gpu_acceleration.c` → 需要分离
- `src/core/gpu_parallelization_optimized.cu` → 需要分离

### 优先级3: 混合求解器重构（关键任务）
**问题**: `hybrid_solver.h/c` 可能包含物理或数值代码

**步骤**:
1. 分析 `hybrid_solver.h/c`
2. 识别物理代码 → 移到 `L1_physics/hybrid/`
3. 识别数值代码 → 移到 `L4_backend/`
4. 只保留编排逻辑在 `L5_orchestration/hybrid_solver/`

**文件**:
- `src/solvers/hybrid/hybrid_solver.h` → 需要重构
- `src/solvers/hybrid/hybrid_solver.c` → 需要重构

### 优先级4: 继续实现迁移

#### L2离散层
- [x] 几何处理实现 (`geometry_engine.c`)
- [x] RWG基函数实现 (`rwg_basis.c`)
- [ ] 网格生成实现 (`mesh_engine.c`)
- [ ] 高阶基函数实现 (`higher_order_basis.c`)
- [ ] Rooftop基函数实现 (`rooftop_basis.c`)

#### L4数值后端
- [ ] 求解器实现 (`solver_interface.c`, `direct_solver.c`, `iterative_solver.c`)
- [ ] H-矩阵实现 (`hmatrix.c`)
- [ ] ACA实现 (`aca.c`)
- [ ] GPU代码实现（分离后）
- [ ] 内存池实现 (`memory_pool.c`)
- [ ] BLAS接口实现 (`blas_interface.c`)

#### L5编排层
- [ ] 混合求解器实现（重构后）
- [ ] 耦合管理器实现
- [ ] 执行顺序实现
- [ ] 数据流实现
- [ ] 工作流引擎实现

#### L6 IO层
- [ ] 文件I/O实现
- [ ] C API实现
- [ ] CLI实现

## 迁移工作流程

### 对于每个文件：

1. **识别文件所属层**
   - 根据功能确定应该属于哪一层
   - 参考 `REFACTORING_MIGRATION_GUIDE.md`

2. **提取实现代码**
   - 从现有代码中提取实现
   - 移除跨层依赖
   - 更新include路径

3. **更新数据结构**
   - 使用新架构中定义的数据结构
   - 移除旧的数据结构引用

4. **验证架构合规性**
   - 运行 `python scripts/validate_architecture.py`
   - 检查是否有跨层依赖
   - 检查是否违反禁止模式

5. **测试**
   - 编译通过
   - 运行单元测试
   - 验证功能正确性

## 关键注意事项

### 1. Include路径更新
所有迁移的文件需要更新include路径：
```c
// 旧路径
#include "../core/core_common.h"
#include "core_geometry.h"

// 新路径
#include "../../common/types.h"
#include "../geometry/geometry_engine.h"
```

### 2. 数据结构更新
使用新架构中定义的数据结构：
```c
// 使用 common/types.h 中的类型
#include "../../common/types.h"

// 使用层特定的数据结构
#include "../mesh/mesh_engine.h"
```

### 3. 函数名更新
保持功能不变，但可能需要调整函数名以符合新架构。

### 4. 架构合规性
每个文件迁移后都要检查：
- [ ] 文件属于且仅属于一层
- [ ] 没有跨层依赖
- [ ] 没有违反禁止模式
- [ ] 接口清晰明确

## 验证清单

每个实现文件迁移后必须验证：

- [ ] 文件属于且仅属于一层
- [ ] 没有跨层依赖（通过grep检查include）
- [ ] 没有违反禁止模式（见 ARCHITECTURE_GUARD.md）
- [ ] 接口清晰明确
- [ ] 编译通过
- [ ] 测试通过
- [ ] 架构验证脚本通过

## 工具和脚本

1. **架构验证脚本**: `scripts/validate_architecture.py`
   - 检查跨层依赖
   - 检查禁止模式
   - 验证include路径

2. **迁移指南**: `docs/REFACTORING_MIGRATION_GUIDE.md`
   - 详细的文件映射表
   - 迁移步骤说明

3. **构建系统**: `CMakeLists_refactored.txt`
   - 新架构CMake配置
   - 层间依赖关系

## 预计时间

- **L3算子层剩余**: 1-2小时
- **GPU代码分离**: 4-6小时（关键任务）
- **混合求解器重构**: 4-6小时（关键任务）
- **其他层实现迁移**: 20-30小时

## 总结

当前重构进度良好，架构设计已完成，接口文件已创建，部分实现已迁移。下一步重点是：

1. 完成L3算子层剩余实现
2. **GPU代码分离**（关键）
3. **混合求解器重构**（关键）
4. 继续其他层的实现迁移

所有工具和文档已就绪，可以继续完成剩余的重构工作。

---

**最后更新**: 2025-01-18
