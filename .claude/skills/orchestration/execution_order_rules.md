# Execution Order Rules

## Scope
Execution sequencing and dependency management. These belong to **L5 Execution Orchestration Layer**.

## Architecture Rules

### L5 Layer (Execution Orchestration)
- ✅ Manages execution order and dependencies
- ✅ Provides explicit dependency graph
- ✅ Detects circular dependencies
- ✅ Ensures correct task sequencing
- ❌ Does NOT implement task execution
- ❌ Does NOT contain numerical logic

## Core Constraints
- **Explicit dependency graph**: All dependencies are explicit
- **No implicit side effects**: Dependencies are explicit, not hidden
- **Circular dependency detection**: Prevents invalid dependency cycles
- **Deterministic ordering**: Same dependencies → same execution order

## Execution Task Types

### Supported Tasks
- **TASK_MOM_SOLVE**: MoM solver execution
- **TASK_PEEC_SOLVE**: PEEC solver execution
- **TASK_MTL_SOLVE**: MTL solver execution
- **TASK_COUPLING_TRANSFER**: Coupling data transfer
- **TASK_INTERFACE_UPDATE**: Interface condition update
- **TASK_CONVERGENCE_CHECK**: Convergence checking

## Dependency Management

### Dependency Graph
- **Structure**: Directed acyclic graph (DAG)
- **Nodes**: Execution tasks
- **Edges**: Dependencies (task A depends on task B)
- **Validation**: Circular dependency detection

### Circular Dependency Detection
- **Self-dependency**: Task depends on itself (detected)
- **Direct cycle**: A → B → A (detected)
- **Indirect cycle**: A → B → C → A (basic detection, full DFS in future)

## Implementation Status

| Feature | Status | Location | Notes |
|---------|--------|----------|-------|
| Execution graph creation | ✅ | `execution_order.c` | Fully implemented |
| Task addition | ✅ | `execution_order.c` | Fully implemented |
| Dependency addition | ✅ | `execution_order.c` | Fully implemented |
| Circular dependency check | ✅ | `execution_order.c` | Basic (direct cycles) |
| Topological sort | ⚠️ | Interface defined | For execution order |
| Graph validation | ✅ | `execution_order.c` | Fully implemented |

## File Locations

- **L5 Interface**: `src/orchestration/execution/execution_order.h`
- **L5 Implementation**: `src/orchestration/execution/execution_order.c`

## Usage

### Create Execution Graph
```c
execution_graph_t* graph = execution_order_create_graph();
if (!graph) {
    // Handle error
}
```

### Add Task
```c
int task_id;
int status = execution_order_add_task(graph, TASK_MOM_SOLVE, &task_id);
```

### Add Dependency
```c
int status = execution_order_add_dependency(graph, task_id, dependency_id);
// Returns error if circular dependency detected
```

### Validate Graph
```c
bool is_valid = execution_order_validate_graph(graph);
if (!is_valid) {
    // Graph has issues (circular dependencies, etc.)
}
```

## Core Constraints

- **Explicit dependencies**: All dependencies must be explicit
- **No implicit side effects**: Dependencies are visible
- **Circular dependency prevention**: Invalid cycles are detected
- **Layer separation**: L5 manages order, other layers execute tasks
