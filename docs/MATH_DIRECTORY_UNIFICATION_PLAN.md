# Math目录统一计划

## 分析时间
2025-01-XX

## 当前状态

### `src/math/` 目录
- **文件数**: 5个文件
- **包含**:
  - `industrial_solver_abstraction.h` - 工业级求解器抽象层
  - `math_backend_implementation.c` - 数学后端实现
  - `math_backend_selector.h` - 数学后端选择器
  - `unified_matrix_assembly.c/h` - 统一矩阵组装
- **状态**: ⚠️ 需要迁移到L4层

### `src/backend/math/` 目录
- **文件数**: 2个文件
- **包含**:
  - `blas_interface.c/h` - BLAS接口（L4层标准实现）
- **状态**: ✅ 符合架构（L4层）

### `src/core/math/` 目录（如果存在）
- **状态**: 需要检查是否存在重复

## 架构分析

### L4层（Numerical Backend）
- **职责**: 提供数值计算后端（BLAS, LAPACK, 求解器等）
- **位置**: `src/backend/math/`
- **包含**:
  - BLAS/LAPACK接口
  - 数学后端选择器
  - 数学后端实现
  - 统一矩阵组装（如果属于L4层）

### 功能分析

1. **`math_backend_selector.h`** - 数学后端选择器
   - **归属**: L4层（数值后端选择）
   - **建议**: 迁移到 `src/backend/math/`

2. **`math_backend_implementation.c`** - 数学后端实现
   - **归属**: L4层（数值后端实现）
   - **建议**: 迁移到 `src/backend/math/`

3. **`industrial_solver_abstraction.h`** - 工业级求解器抽象层
   - **归属**: L4层（求解器抽象）
   - **建议**: 迁移到 `src/backend/math/` 或 `src/backend/solvers/`

4. **`unified_matrix_assembly.c/h`** - 统一矩阵组装
   - **归属**: 可能是L3层（算子）或L4层（数值后端）
   - **分析**: 如果只是矩阵组装，属于L3层；如果涉及数值计算后端，属于L4层
   - **建议**: 根据实际功能决定

## 统一计划

### 选项A: 全部迁移到 `src/backend/math/`（推荐）

**理由**: 所有文件都是数学后端相关，应该属于L4层

1. **迁移文件**:
   ```bash
   # 迁移到backend/math
   mv src/math/math_backend_selector.h src/backend/math/
   mv src/math/math_backend_implementation.c src/backend/math/
   mv src/math/industrial_solver_abstraction.h src/backend/math/
   mv src/math/unified_matrix_assembly.c src/backend/math/
   mv src/math/unified_matrix_assembly.h src/backend/math/
   ```

2. **更新引用路径**:
   - 更新所有 `#include "math/..."` 为 `#include "backend/math/..."`
   - 更新所有 `#include "../math/..."` 为 `#include "../backend/math/..."`

3. **删除 `src/math/` 目录**

### 选项B: 分离到不同层

1. **L4层** (`src/backend/math/`):
   - `math_backend_selector.h`
   - `math_backend_implementation.c`
   - `blas_interface.c/h`

2. **L3层** (`src/operators/`):
   - `unified_matrix_assembly.c/h`（如果只是矩阵组装）

3. **L4层** (`src/backend/solvers/`):
   - `industrial_solver_abstraction.h`（如果主要是求解器抽象）

**推荐**: 选项A（更简单，所有都是数学后端相关）

## 执行步骤

### 步骤1: 检查重复

```bash
# 检查core/math是否存在
if [ -d "src/core/math" ]; then
    echo "Warning: src/core/math exists, may have duplicates"
fi
```

### 步骤2: 迁移文件

```bash
# 迁移所有文件到backend/math
mv src/math/*.h src/backend/math/
mv src/math/*.c src/backend/math/
```

### 步骤3: 更新引用路径

- 搜索所有对 `src/math/` 的引用
- 更新为 `src/backend/math/`

### 步骤4: 更新include路径

- `unified_matrix_assembly.h` 中的 `#include "../core/core_geometry.h"` 可能需要更新
- `unified_matrix_assembly.h` 中的 `#include "../core/core_mesh.h"` 可能需要更新

### 步骤5: 删除math目录

```bash
rm -rf src/math/
```

## 风险评估

### 低风险
- 文件数量少（5个文件）
- 功能明确（都是数学后端相关）

### 中风险
- 需要更新引用路径
- `unified_matrix_assembly.h` 中的include路径可能需要调整

### 注意事项
- 确保 `unified_matrix_assembly.h` 中的相对路径正确
- 检查是否有其他代码依赖 `src/math/` 的路径

## 验证方法

1. **编译验证**: 确保所有代码能编译通过
2. **功能验证**: 运行数学后端相关测试
3. **架构验证**: 使用 `scripts/validate_architecture.py`
4. **依赖检查**: 检查所有include路径
