# 编排逻辑迁移状态

## 已完成 ✅

### MoM 编排逻辑

1. ✅ **算法选择器**
   - `orchestration/mom_orchestration/mom_algorithm_selector.h`
   - `orchestration/mom_orchestration/mom_algorithm_selector.c`
   - 从 `solvers/mom/mom_solver_unified.c` 提取：
     - `select_algorithm()` 函数
     - `compute_problem_characteristics()` 函数
     - `detect_curved_surfaces()` 函数
     - `count_different_materials()` 函数

2. ✅ **时域分析**
   - `orchestration/mom_orchestration/mom_time_domain.h`
   - `orchestration/mom_orchestration/mom_time_domain.c`
   - 从 `solvers/mom/mom_time_domain.c/h` 迁移

### PEEC 编排逻辑

1. ✅ **算法选择器**
   - `orchestration/peec_orchestration/peec_algorithm_selector.h`
   - `orchestration/peec_orchestration/peec_algorithm_selector.c`
   - 从 `solvers/peec/peec_solver_unified.c` 提取：
     - `select_peec_algorithm()` 函数
     - `compute_peec_problem_characteristics()` 函数

2. ✅ **时域分析**
   - `orchestration/peec_orchestration/peec_time_domain.h`
   - `orchestration/peec_orchestration/peec_time_domain.c`
   - 从 `solvers/peec/peec_time_domain.c/h` 迁移

### MTL 编排逻辑

1. ✅ **宽带分析**
   - `orchestration/mtl_orchestration/mtl_wideband.h`
   - `orchestration/mtl_orchestration/mtl_wideband.c`
   - 从 `solvers/mtl/mtl_wideband.c/h` 迁移

2. ✅ **时域分析**
   - `orchestration/mtl_orchestration/mtl_time_domain.h`
   - `orchestration/mtl_orchestration/mtl_time_domain.c`
   - 从 `solvers/mtl/mtl_time_domain.c/h` 迁移

---

## 待完成 ⏳

### 1. 更新引用

需要更新所有引用旧路径的 `#include` 语句：

- `solvers/mom/mom_time_domain.h` → `orchestration/mom_orchestration/mom_time_domain.h`
- `solvers/peec/peec_time_domain.h` → `orchestration/peec_orchestration/peec_time_domain.h`
- `solvers/mtl/mtl_wideband.h` → `orchestration/mtl_orchestration/mtl_wideband.h`
- `solvers/mtl/mtl_time_domain.h` → `orchestration/mtl_orchestration/mtl_time_domain.h`

### 2. 更新 `mom_solver_unified.c` 和 `peec_solver_unified.c`

- 移除算法选择逻辑（已提取到 orchestration）
- 更新引用，使用新的编排模块

### 3. 更新项目文件

- 更新 `.vcxproj` 文件，添加新文件，移除旧引用

### 4. 编译测试

- 使用 task.json 编译项目
- 修复所有编译错误

---

## 文件统计

### 已创建文件（编排层）

- MoM: 4 个文件（算法选择器 + 时域分析）
- PEEC: 4 个文件（算法选择器 + 时域分析）
- MTL: 4 个文件（宽带分析 + 时域分析）

**总计**: 12 个新文件

### 待迁移文件（仍保留在原位置）

- `solvers/mom/mom_time_domain.c/h` - 可删除（已迁移）
- `solvers/peec/peec_time_domain.c/h` - 可删除（已迁移）
- `solvers/mtl/mtl_wideband.c/h` - 可删除（已迁移）
- `solvers/mtl/mtl_time_domain.c/h` - 可删除（已迁移）

---

**下一步**: 更新引用，然后编译测试。
