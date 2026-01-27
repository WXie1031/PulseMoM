# common/utils/materials/ 职责边界分析

## 分析目标

明确 `common/`、`utils/` 和 `materials/` 三个目录的职责边界，收敛语义，避免职责漂移。

---

## 一、目录结构

### 1.1 `src/common/` 目录

**文件列表**:
- `compatibility_adapter.h` - 兼容性适配器
- `constants.h` - 物理常量定义
- `core_common.h` - 核心通用定义（类型、常量、工具函数）
- `errors_core.c/h` - 错误处理核心
- `errors.h` - 错误定义
- `layer_interfaces.h` - 层间接口定义
- `types.h` - 类型定义

**当前职责**:
- 类型定义（`types.h`）
- 常量定义（`constants.h`）
- 层间接口定义（`layer_interfaces.h`）
- 错误定义（`errors.h`）
- 通用工具函数（`core_common.h`）

### 1.2 `src/utils/` 目录

**文件列表**:
- `error_handler.h` - 错误处理
- `logger.h` - 日志
- `memory_manager.h` - 内存管理
- `evaluation/quantitative_metrics.c/h` - 定量评估
- `performance/performance_monitor.c/h` - 性能监控
- `validation/commercial_validation.c/h` - 商业验证
- `validation/industrial_validation_benchmarks.h` - 工业验证基准

**当前职责**:
- 日志工具
- 内存管理工具
- 性能监控工具
- 验证工具

### 1.3 `src/materials/` 和 `src/physics/materials/` 目录

**文件列表**:
- `src/materials/material_library.c/h` - 材料库
- `src/materials/cst_materials_parser.c/h` - CST材料解析器
- `src/physics/materials/advanced_models.c/h` - 高级材料模型

**当前职责**:
- 材料数据（材料库）
- 材料本构模型（物理定义）

---

## 二、职责边界定义

### 2.1 `common/` - 只允许

**职责**:
- ✅ 类型定义（`types.h`）
- ✅ 常量定义（`constants.h`）
- ✅ 层间接口定义（`layer_interfaces.h`）
- ✅ 错误定义（`errors.h`）
- ✅ 通用工具函数（`core_common.h`）- 但应限制为最基础的通用函数

**禁止**:
- ❌ 物理语义的工具函数
- ❌ 材料定义
- ❌ 特定层的实现逻辑

### 2.2 `utils/` - 只允许

**职责**:
- ✅ 无物理语义的工具函数
- ✅ 日志、性能监控、验证
- ✅ 内存管理工具
- ✅ 错误处理工具

**禁止**:
- ❌ 物理常数（应放在 `common/constants.h`）
- ❌ 材料定义（应放在 `materials/` 或 `physics/materials/`）
- ❌ 层间接口定义（应放在 `common/layer_interfaces.h`）

### 2.3 `materials/` - 只允许

**职责**:
- ✅ 材料数据（材料库）
- ✅ 材料本构模型（物理定义）
- ✅ 材料解析器（I/O相关）

**禁止**:
- ❌ 通用工具函数
- ❌ 类型定义（应放在 `common/types.h`）

---

## 三、当前问题分析

### 3.1 `common/core_common.h` 可能过厚

**问题**:
- 包含类型定义、常量定义、工具函数
- 可能包含物理语义的工具函数

**建议**:
- 类型定义 → `common/types.h`
- 常量定义 → `common/constants.h`
- 工具函数 → 评估是否需要物理语义
  - 无物理语义 → `utils/`
  - 有物理语义 → 移到相应层

### 3.2 `materials/` 位置问题

**当前状态**:
- `src/materials/` - 材料库和解析器
- `src/physics/materials/` - 高级材料模型

**建议**:
- 材料库和解析器 → `src/materials/` (L6 IO层)
- 材料本构模型 → `src/physics/materials/` (L1 Physics层)

**架构决策**:
- 材料数据（库） → L6 IO层（`materials/`）
- 材料本构模型（物理定义） → L1 Physics层（`physics/materials/`）

### 3.3 `utils/` 职责清晰

**状态**: ✅ 职责清晰
- 日志、性能监控、验证都是无物理语义的工具
- 符合架构要求

---

## 四、优化建议

### 4.1 明确命名规则

**`common/`**:
- 只允许：类型、常量、接口、错误定义
- 命名：`types.h`, `constants.h`, `layer_interfaces.h`, `errors.h`

**`utils/`**:
- 只允许：无物理语义的工具
- 命名：`logger.h`, `memory_manager.h`, `performance_monitor.h`

**`materials/`**:
- 只允许：材料数据、材料本构模型
- 命名：`material_library.h`, `material_models.h`

### 4.2 禁止交叉引用方向

**规则**:
- `common/` 不应引用 `utils/` 或 `materials/`
- `utils/` 可以引用 `common/`，但不应引用 `materials/`
- `materials/` 可以引用 `common/`，但不应引用 `utils/`

### 4.3 文件迁移建议（可选）

**低优先级**:
- 评估 `common/core_common.h` 是否需要拆分
- 评估 `materials/` 和 `physics/materials/` 是否需要合并

**当前状态**: ✅ 职责边界基本清晰，无需立即重构

---

## 五、最终职责边界

### 5.1 `common/` - 公共定义层

**职责**:
- 类型定义（`types.h`）
- 常量定义（`constants.h`）
- 层间接口定义（`layer_interfaces.h`）
- 错误定义（`errors.h`）
- 最基础的通用工具函数（`core_common.h`）

**禁止**:
- ❌ 物理语义的工具函数
- ❌ 材料定义
- ❌ 特定层的实现逻辑

### 5.2 `utils/` - 工具函数层

**职责**:
- 无物理语义的工具函数
- 日志、性能监控、验证
- 内存管理工具
- 错误处理工具

**禁止**:
- ❌ 物理常数
- ❌ 材料定义
- ❌ 层间接口定义

### 5.3 `materials/` - 材料数据层（L6 IO）

**职责**:
- 材料数据（材料库）
- 材料解析器（I/O相关）

**禁止**:
- ❌ 通用工具函数
- ❌ 类型定义

### 5.4 `physics/materials/` - 材料物理定义层（L1 Physics）

**职责**:
- 材料本构模型（物理定义）
- 材料物理方程

**禁止**:
- ❌ 材料数据（应放在 `materials/`）
- ❌ 材料解析器（应放在 `materials/`）

---

## 六、架构决策树应用

### Q8: 这是纯基础设施或工具吗？

**`common/`**:
- ✅ 类型、常量、接口定义 → `common/`

**`utils/`**:
- ✅ 无物理语义的工具 → `utils/`

**`materials/`**:
- ❌ 不是纯工具 → Q1

### Q1: 定义物理方程或本构关系吗？

**`physics/materials/`**:
- ✅ 材料本构模型 → `physics/materials/` (L1)

**`materials/`**:
- ❌ 材料数据（不是物理定义） → Q8 (I/O相关) → `materials/` (L6)

---

## 七、结论

### 当前状态: ✅ 职责边界基本清晰

1. **`common/`**: 职责清晰，主要是类型、常量、接口定义
2. **`utils/`**: 职责清晰，无物理语义的工具函数
3. **`materials/`**: 职责清晰，材料数据和解析器（L6 IO层）
4. **`physics/materials/`**: 职责清晰，材料本构模型（L1 Physics层）

### 优化建议

**低优先级**（无需立即重构）:
- 评估 `common/core_common.h` 是否需要拆分
- 评估是否有物理语义的工具函数混入 `common/` 或 `utils/`

**当前状态**: ✅ 符合架构要求，职责边界清晰

---

**状态**: 分析完成，职责边界已明确。
