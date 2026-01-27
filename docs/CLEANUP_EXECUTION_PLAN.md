# 代码清理执行计划

## 执行时间
2025-01-XX

## 执行摘要

根据 `CODE_DUPLICATION_ANALYSIS.md` 的分析，本计划将系统性地清理重复代码、旧代码和架构违规代码。

## 阶段1: 删除重复目录（高优先级）

### 1.1 删除 `src/cad/` 目录
- **原因**: 与 `src/mesh/cad_mesh_generation` 完全重复
- **检查**: 确认无其他文件引用
- **状态**: ⏳ 待执行

### 1.2 删除 `src/api/` 目录
- **原因**: 与 `src/io/api/` 重复
- **检查**: 确认 `api_generator.c/h` 未被使用
- **状态**: ⏳ 待执行

### 1.3 删除 `src/performance/` 目录
- **原因**: 与 `src/utils/performance/` 完全重复
- **检查**: 确认无其他文件引用
- **状态**: ⏳ 待执行

### 1.4 删除 `src/evaluation/` 目录
- **原因**: 与 `src/utils/evaluation/` 完全重复
- **检查**: 确认无其他文件引用
- **状态**: ⏳ 待执行

### 1.5 删除 `src/validation/` 目录
- **原因**: 与 `src/utils/validation/` 完全重复
- **检查**: 确认无其他文件引用
- **状态**: ⏳ 待执行

## 阶段2: 删除临时文件

### 2.1 删除 `src/core/core_solver_header_fix.c`
- **原因**: 临时修复文件
- **状态**: ⏳ 待执行

### 2.2 删除 `src/core/compatibility_adapter.h`（迁移完成后）
- **原因**: 临时兼容性适配器
- **状态**: ⏳ 待执行（需要先完成迁移）

## 阶段3: 移动文件到正确位置

### 3.1 移动Python文件
- `src/core/advanced_electromagnetic_modeler.py` → `python/`
- `src/core/petsc_solver_backend.py` → `python/`
- **状态**: ⏳ 待执行

### 3.2 移动文档文件
- `src/core/积分代码模块化说明.md` → `docs/`
- **状态**: ⏳ 待执行

## 阶段4: 分析并重构Core目录

### 4.1 分析Core目录文件归属
- 列出所有文件及其应该归属的层
- **状态**: ⏳ 待执行

### 4.2 迁移Core目录文件
- 按层迁移到对应目录
- **状态**: ⏳ 待执行

## 阶段5: 统一工作流和网格代码

### 5.1 统一工作流代码
- 迁移 `src/workflows/` 到 `src/orchestration/workflow/`
- **状态**: ⏳ 待执行

### 5.2 统一网格代码
- 迁移 `src/mesh/` 到 `src/discretization/mesh/`
- **状态**: ⏳ 待执行

## 阶段6: 清理IO层

### 6.1 识别非IO代码
- 分析 `src/io/` 中的文件
- **状态**: ⏳ 待执行

### 6.2 迁移非IO代码
- 将非IO代码迁移到对应层
- **状态**: ⏳ 待执行

## 验证步骤

1. **编译验证**: 确保所有代码能编译通过
2. **功能验证**: 运行测试套件
3. **架构验证**: 使用 `scripts/validate_architecture.py`
4. **依赖检查**: 检查所有include路径

## 风险评估

### 高风险
- 重构Core目录可能影响大量代码
- 需要仔细测试每个迁移

### 中风险
- 删除重复代码需要确认无依赖
- 工作流迁移需要验证功能

### 低风险
- 删除临时文件
- 移动Python文件
