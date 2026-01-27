# Workflow Rules

## Scope
High-level simulation workflows. These belong to **L5 Execution Orchestration Layer**.

## Architecture Rules

### L5 Layer (Execution Orchestration)
- ✅ Orchestrates high-level simulation workflows
- ✅ Manages workflow steps and sequencing
- ✅ Uses declarative workflow definitions
- ✅ Coordinates execution and data flow graphs
- ❌ Does NOT implement numerical logic
- ❌ Does NOT implement physics
- ❌ Does NOT make solver decisions

## Core Constraints
- **Declarative sequencing**: Workflows are defined declaratively, not procedurally
- **No numerical logic**: Workflow engine does not contain numerical code
- **Explicit dependencies**: All step dependencies are explicit
- **Layer separation**: L5 orchestrates, L4 implements, L3 defines operators

## Workflow Components

### Workflow Steps
- **Geometry Import**: Import CAD files
- **Mesh Generation**: Generate discretization mesh
- **Basis Function Setup**: Set up basis functions
- **Matrix Assembly**: Assemble operator matrices
- **Solver Execution**: Execute numerical solvers
- **Post-Processing**: Process and export results

### Execution Graph
- Defines task dependencies
- Ensures correct execution order
- Detects circular dependencies

### Data Flow Graph
- Tracks data dependencies
- Manages data transfer between steps
- Ensures data availability

## Implementation Status

| Feature | Status | Location | Notes |
|---------|--------|----------|-------|
| Workflow engine | ✅ | `workflow_engine.c` | Fully implemented |
| Step management | ✅ | `workflow_engine.c` | Fully implemented |
| Execution graph | ✅ | `execution_order.c` | Fully implemented |
| Data flow graph | ✅ | `data_flow.c` | Fully implemented |
| Workflow validation | ✅ | `workflow_engine.c` | Fully implemented |

## File Locations

- **L5 Interface**: `src/orchestration/workflow/workflow_engine.h`
- **L5 Implementation**: `src/orchestration/workflow/workflow_engine.c`
- **Execution Graph**: `src/orchestration/execution/execution_order.h`
- **Data Flow**: `src/orchestration/execution/data_flow.h`

## Usage

### Create Workflow Engine
```c
workflow_engine_t* engine = workflow_engine_create();
if (!engine) {
    // Handle error
}
```

### Add Workflow Step
```c
workflow_step_t step;
step.type = WORKFLOW_STEP_MESH_GENERATION;
step.config = &mesh_config;

int status = workflow_engine_add_step(engine, &step);
```

### Execute Workflow
```c
int status = workflow_engine_execute(engine);
```

## Core Constraints

- **Declarative**: Workflows are defined, not implemented
- **No numerical logic**: All numerical code is in L4
- **Explicit dependencies**: All dependencies are explicit
- **Layer separation**: L5 orchestrates, other layers implement
