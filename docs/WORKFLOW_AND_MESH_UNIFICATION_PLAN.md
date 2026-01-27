# 工作流和网格代码统一计划

## 分析时间
2025-01-XX

## 工作流代码分析

### `src/workflows/pcb/pcb_simulation_workflow.c/h`

**功能**: PCB特定的仿真工作流实现
- 包含完整的PCB仿真流程（加载、网格生成、端口设置、仿真、后处理）
- 使用 `pcb_electromagnetic_modeling` 和 `pcb_file_io`
- 包含具体的实现代码

**架构归属**: 应该属于L5编排层
- 但包含了一些实现细节，可能需要重构

**建议**: 
- 移动到 `src/orchestration/workflow/pcb/pcb_simulation_workflow.c/h`
- 或重构为使用 `workflow_engine` 的步骤实现

### `src/orchestration/workflow/workflow_engine.c/h`

**功能**: 通用工作流引擎（L5层标准实现）
- 提供工作流编排框架
- 定义工作流步骤接口
- 管理执行顺序和数据流

**架构归属**: ✅ 正确（L5编排层）

**建议**: 保留，作为标准工作流引擎

## 网格代码分析

### `src/mesh/mesh_engine.c/h`

**功能**: 旧的mesh引擎实现
- 依赖 `core/core_common.h` 和 `core/core_mesh.h`
- 包含21个文件（C++和C混合）
- 包含CAD导入、CGAL集成、Gmsh集成等

**架构归属**: 应该属于L2离散层，但依赖core目录

**状态**: ⚠️ 旧代码，需要迁移

### `src/discretization/mesh/mesh_engine.c/h`

**功能**: 新的符合架构的mesh引擎（L2层标准实现）
- 符合六层架构
- 依赖 `common/types.h` 和 `geometry/geometry_engine.h`
- 只包含mesh引擎接口

**架构归属**: ✅ 正确（L2离散层）

**建议**: 保留，作为标准mesh引擎

## 统一计划

### 阶段1: 工作流代码统一

#### 选项A: 移动到orchestration目录（推荐）
1. 移动 `src/workflows/pcb/pcb_simulation_workflow.c/h` → `src/orchestration/workflow/pcb/pcb_simulation_workflow.c/h`
2. 更新所有引用路径
3. 删除 `src/workflows/` 目录

#### 选项B: 重构为workflow步骤
1. 将 `pcb_simulation_workflow` 重构为使用 `workflow_engine` 的步骤
2. 移动到 `src/orchestration/workflow/steps/pcb_step.c/h`
3. 删除 `src/workflows/` 目录

**推荐**: 选项A（更简单，风险更低）

### 阶段2: 网格代码统一

#### 选项A: 迁移mesh目录到discretization（推荐）
1. 分析 `src/mesh/` 中的文件
2. 迁移到 `src/discretization/mesh/` 或相关子目录
3. 更新所有引用路径
4. 删除 `src/mesh/` 目录

#### 选项B: 保留mesh目录但重构
1. 重构 `src/mesh/` 中的代码，移除对core的依赖
2. 更新为符合L2层架构
3. 与 `src/discretization/mesh/` 合并

**推荐**: 选项A（更符合架构）

## 执行步骤

### 步骤1: 统一工作流代码

1. **检查引用**
   ```bash
   # 查找所有对 workflows/pcb/pcb_simulation_workflow 的引用
   ```

2. **移动文件**
   ```bash
   # 创建目录
   mkdir -p src/orchestration/workflow/pcb
   
   # 移动文件
   mv src/workflows/pcb/pcb_simulation_workflow.c src/orchestration/workflow/pcb/
   mv src/workflows/pcb/pcb_simulation_workflow.h src/orchestration/workflow/pcb/
   ```

3. **更新引用路径**
   - 更新所有 `#include "../../workflows/pcb/pcb_simulation_workflow.h"` 
   - 改为 `#include "../../orchestration/workflow/pcb/pcb_simulation_workflow.h"`

4. **删除旧目录**
   ```bash
   rm -rf src/workflows/
   ```

### 步骤2: 统一网格代码

1. **分析mesh目录文件**
   - 分类文件（CAD导入、网格算法、mesh引擎等）
   - 确定每个文件的归属

2. **迁移文件**
   ```bash
   # CAD导入 → discretization/geometry/
   mv src/mesh/opencascade_cad_import.* src/discretization/geometry/
   
   # 网格算法 → discretization/mesh/
   mv src/mesh/mesh_algorithms.c src/discretization/mesh/
   mv src/mesh/cgal_surface_mesh.* src/discretization/mesh/
   mv src/mesh/gmsh_surface_mesh.* src/discretization/mesh/
   
   # CAD网格生成 → discretization/mesh/
   mv src/mesh/cad_mesh_generation.* src/discretization/mesh/
   ```

3. **更新引用路径**
   - 更新所有对 `src/mesh/` 的引用

4. **删除旧目录**
   ```bash
   rm -rf src/mesh/
   ```

## 风险评估

### 工作流统一
- **风险**: 低-中
- **影响**: 需要更新引用路径
- **测试**: 需要验证工作流功能

### 网格统一
- **风险**: 中-高
- **影响**: 大量文件需要迁移，可能影响编译
- **测试**: 需要验证网格生成功能

## 验证方法

1. **编译验证**: 确保所有代码能编译通过
2. **功能验证**: 运行测试套件
3. **架构验证**: 使用 `scripts/validate_architecture.py`
4. **依赖检查**: 检查所有include路径
