# Pipeline 实现策略分析

## 问题

Pipeline 应该在 C 语言中实现（作为 L5 层 workflow_engine），还是在 Python 中自行组装？

## 当前状态

### C 语言中的实现

**已有组件：**
- ✅ `src/orchestration/workflow/workflow_engine.c` - L5 层 workflow engine
- ✅ `src/orchestration/execution/execution_order.c` - 执行顺序管理
- ✅ `src/orchestration/execution/data_flow.c` - 数据流管理

**问题：**
- `workflow_engine_execute()` 中的 switch case 只是占位符，没有实际调用
- 需要完善各个步骤的实际调用

### Python 中的实现

**当前实现：**
- `python/tools/simulation_pipeline.py` - 直接在 Python 中组装调用各个步骤
- 直接调用 `mom_solver_*` 等函数

## 架构分析

### 架构规则（来自 skills）

1. **L5 层（Execution Orchestration）**：
   - ✅ 负责编排工作流
   - ✅ 管理执行顺序和数据流
   - ❌ 不实现数值逻辑
   - ❌ 不实现物理逻辑

2. **L6 层（IO/API）**：
   - ✅ 提供 API 接口
   - ✅ Python 接口属于 L6 层
   - ❌ 不应该直接调用 L2/L3/L4 层

3. **分层原则**：
   - 上层可以调用下层
   - 下层不能依赖上层
   - 每层职责明确

## 推荐方案：在 C 中完善 workflow_engine

### 理由

1. **符合架构分层**：
   - L5 层负责编排，应该调用 L2/L3/L4 层
   - L6 层（Python）应该调用 L5 层，而不是直接调用底层

2. **代码复用**：
   - C 的 workflow_engine 可以被多个接口使用（C API、Python、CLI）
   - 避免在多个地方重复实现相同的逻辑

3. **维护性**：
   - 工作流逻辑集中在一个地方
   - 修改工作流只需要修改 C 代码

4. **性能**：
   - C 中的调用开销更小
   - 减少 Python-C 边界调用次数

### 实现方案

#### 1. 完善 C 中的 workflow_engine_execute()

```c
// src/orchestration/workflow/workflow_engine.c

int workflow_engine_execute(workflow_engine_t* engine) {
    // ... 现有代码 ...
    
    // Execute steps in order
    for (int i = 0; i < sequence_length; i++) {
        workflow_step_entry_t* entry = &engine->steps[step_idx];
        
        switch (entry->step) {
            case WORKFLOW_STEP_GEOMETRY_IMPORT: {
                // L5 调用 L2 层
                geometry_import_config_t* config = (geometry_import_config_t*)entry->step_config;
                status = geometry_engine_import_file(
                    engine->geometry_context,
                    config->filename,
                    config->format
                );
                break;
            }
            
            case WORKFLOW_STEP_MESH_GENERATION: {
                // L5 调用 L2 层
                mesh_config_t* config = (mesh_config_t*)entry->step_config;
                status = mesh_engine_generate(
                    engine->mesh_context,
                    engine->geometry_context,
                    config
                );
                break;
            }
            
            case WORKFLOW_STEP_MATRIX_ASSEMBLY: {
                // L5 调用 L3 层
                matrix_assembly_config_t* config = (matrix_assembly_config_t*)entry->step_config;
                status = matrix_assembler_assemble(
                    engine->assembler_context,
                    engine->mesh_context,
                    config
                );
                break;
            }
            
            case WORKFLOW_STEP_SOLUTION: {
                // L5 调用 L4 层
                solver_config_t* config = (solver_config_t*)entry->step_config;
                if (config->solver_type == SOLVER_TYPE_MOM) {
                    status = mom_solver_solve(engine->mom_solver);
                } else if (config->solver_type == SOLVER_TYPE_PEEC) {
                    status = peec_solver_solve(engine->peec_solver);
                }
                break;
            }
            
            case WORKFLOW_STEP_POSTPROCESSING: {
                // L5 调用 L6 层（后处理）
                status = postprocessing_compute_fields(
                    engine->postprocessing_context,
                    engine->solver_results
                );
                break;
            }
            
            case WORKFLOW_STEP_EXPORT: {
                // L5 调用 L6 层（文件 I/O）
                export_config_t* config = (export_config_t*)entry->step_config;
                status = file_io_export_results(
                    engine->results_context,
                    config->filename,
                    config->format
                );
                break;
            }
        }
        
        if (status != STATUS_SUCCESS) {
            // Handle error
            break;
        }
        
        entry->is_executed = true;
    }
    
    return status;
}
```

#### 2. Python Pipeline 调用 C 的 workflow_engine

```python
# python/tools/simulation_pipeline.py

class SimulationPipeline:
    def _run_simulation_ctypes(self, config: SimulationConfig) -> SimulationResult:
        interface = self.ctypes_interface
        
        # 创建 workflow engine（通过 C API）
        workflow = interface.create_workflow_engine()
        
        # 添加工作流步骤
        interface.workflow_add_step(workflow, WORKFLOW_STEP_GEOMETRY_IMPORT, {
            'filename': config.geometry_file,
            'format': config.geometry_format
        })
        
        interface.workflow_add_step(workflow, WORKFLOW_STEP_MESH_GENERATION, {
            'density': config.mesh_density
        })
        
        interface.workflow_add_step(workflow, WORKFLOW_STEP_MATRIX_ASSEMBLY, {
            'solver_type': config.solver_type
        })
        
        interface.workflow_add_step(workflow, WORKFLOW_STEP_SOLUTION, {
            'frequency': config.frequency,
            'tolerance': config.tolerance
        })
        
        interface.workflow_add_step(workflow, WORKFLOW_STEP_POSTPROCESSING, {})
        
        # 执行工作流（C 中完成所有步骤）
        status = interface.workflow_execute(workflow)
        
        # 获取结果
        if status == 0:
            result = self._get_workflow_results(workflow)
        else:
            result = self._handle_error(workflow, status)
        
        interface.destroy_workflow_engine(workflow)
        return result
```

### 需要添加的 C API 函数

在 `python_interface/mom_peec_ctypes.py` 中添加：

```python
# Workflow engine functions
self.lib.workflow_engine_create.restype = ctypes.c_void_p
self.lib.workflow_engine_create.argtypes = []

self.lib.workflow_engine_destroy.restype = None
self.lib.workflow_engine_destroy.argtypes = [ctypes.c_void_p]

self.lib.workflow_engine_add_step.restype = ctypes.c_int
self.lib.workflow_engine_add_step.argtypes = [
    ctypes.c_void_p,  # engine
    ctypes.c_int,     # step type
    ctypes.c_void_p   # step config
]

self.lib.workflow_engine_execute.restype = ctypes.c_int
self.lib.workflow_engine_execute.argtypes = [ctypes.c_void_p]
```

## 对比方案

### 方案 A：在 C 中完善 workflow_engine（推荐）

**优点：**
- ✅ 符合架构分层
- ✅ 代码复用性好
- ✅ 维护简单
- ✅ 性能更好

**缺点：**
- ⚠️ 需要完善 C 代码
- ⚠️ 需要添加更多 C API 绑定

### 方案 B：在 Python 中自行组装（当前实现）

**优点：**
- ✅ 实现快速
- ✅ Python 代码灵活

**缺点：**
- ❌ 不符合架构分层（L6 直接调用 L2/L3/L4）
- ❌ 代码重复（如果多个接口都需要）
- ❌ 维护困难（逻辑分散）

## 实施建议

### 短期（保持当前实现）

1. **保持 Python pipeline 的当前实现**（作为临时方案）
2. **添加注释说明**这是临时实现，未来会迁移到 C workflow_engine

### 中期（完善 C 实现）

1. **完善 `workflow_engine_execute()`**，添加实际调用
2. **添加 workflow_engine 的 Python ctypes 绑定**
3. **创建测试用例**验证 C workflow_engine

### 长期（迁移到 C）

1. **重构 Python pipeline**，改为调用 C workflow_engine
2. **移除 Python 中的直接调用逻辑**
3. **统一所有接口**（C API、Python、CLI）都使用 workflow_engine

## 结论

**推荐在 C 中完善 workflow_engine**，原因：
1. 符合架构设计原则
2. 更好的代码组织和复用
3. 更易于维护和扩展

Python pipeline 可以作为临时实现保留，但应该逐步迁移到调用 C 的 workflow_engine。
