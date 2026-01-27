/********************************************************************************
 * Workflow Engine Implementation (L5 Execution Orchestration Layer)
 *
 * Copyright (C) 2025 PulseEM Technologies
 *
 * This file implements workflow orchestration.
 * L5 layer: Execution Orchestration - manages workflow execution.
 *
 * Architecture Rule: L5 orchestrates workflow, does NOT implement physics or numerical code.
 ********************************************************************************/

#include "workflow_engine.h"
#include "../../common/types.h"
#include "../../common/errors.h"
#include "../execution/execution_order.h"
#include "../execution/data_flow.h"
#include <stdlib.h>
#include <string.h>

// Workflow step entry
typedef struct {
    workflow_step_t step;
    void* step_config;
    bool is_executed;
} workflow_step_entry_t;

// Workflow engine structure
struct workflow_engine {
    workflow_step_entry_t* steps;
    int num_steps;
    int capacity;
    
    // Execution context
    execution_graph_t* execution_graph;
    data_flow_graph_t* data_flow_graph;
    
    // State
    bool is_executing;
    int current_step_index;
};

workflow_engine_t* workflow_engine_create(void) {
    workflow_engine_t* engine = (workflow_engine_t*)calloc(1, sizeof(workflow_engine_t));
    if (!engine) return NULL;
    
    engine->capacity = 16;  // Initial capacity
    engine->steps = (workflow_step_entry_t*)calloc(engine->capacity, sizeof(workflow_step_entry_t));
    if (!engine->steps) {
        free(engine);
        return NULL;
    }
    
    engine->num_steps = 0;
    engine->is_executing = false;
    engine->current_step_index = -1;
    
    // Create execution and data flow graphs
    engine->execution_graph = execution_order_create_graph();
    engine->data_flow_graph = data_flow_create_graph();
    
    if (!engine->execution_graph || !engine->data_flow_graph) {
        if (engine->execution_graph) execution_order_destroy_graph(engine->execution_graph);
        if (engine->data_flow_graph) data_flow_destroy_graph(engine->data_flow_graph);
        free(engine->steps);
        free(engine);
        return NULL;
    }
    
    return engine;
}

void workflow_engine_destroy(workflow_engine_t* engine) {
    if (!engine) return;
    
    if (engine->steps) {
        // Free step configs if needed
        // (In full implementation, would call step-specific cleanup)
        free(engine->steps);
    }
    
    if (engine->execution_graph) {
        execution_order_destroy_graph(engine->execution_graph);
    }
    
    if (engine->data_flow_graph) {
        data_flow_destroy_graph(engine->data_flow_graph);
    }
    
    free(engine);
}

int workflow_engine_add_step(
    workflow_engine_t* engine,
    workflow_step_t step,
    void* step_config) {
    
    if (!engine) return STATUS_ERROR_INVALID_INPUT;
    
    // Resize if needed
    if (engine->num_steps >= engine->capacity) {
        int new_capacity = engine->capacity * 2;
        workflow_step_entry_t* new_steps = (workflow_step_entry_t*)realloc(
            engine->steps, new_capacity * sizeof(workflow_step_entry_t));
        if (!new_steps) {
            return STATUS_ERROR_MEMORY_ALLOCATION;
        }
        engine->steps = new_steps;
        engine->capacity = new_capacity;
    }
    
    // Add step
    workflow_step_entry_t* entry = &engine->steps[engine->num_steps];
    entry->step = step;
    entry->step_config = step_config;
    entry->is_executed = false;
    
    // Add to execution graph
    int task_id = engine->num_steps;
    execution_task_t task_type;
    
    // Map workflow step to execution task type
    // L5 layer maps high-level workflow steps to execution tasks
    // This is orchestration logic, not physics or numerical implementation
    switch (step) {
        case WORKFLOW_STEP_GEOMETRY_IMPORT:
            // Geometry import is a data preparation step
            task_type = TASK_INTERFACE_UPDATE;  // Update interface with geometry
            break;
        case WORKFLOW_STEP_MESH_GENERATION:
            // Mesh generation is a data preparation step
            task_type = TASK_INTERFACE_UPDATE;  // Update interface with mesh
            break;
        case WORKFLOW_STEP_MATRIX_ASSEMBLY:
            // Matrix assembly is a preparation step
            task_type = TASK_INTERFACE_UPDATE;  // Update interface with matrices
            break;
        case WORKFLOW_STEP_SOLUTION:
            // Solution step depends on solver type (MoM, PEEC, etc.)
            // For now, default to MoM - full implementation would determine from config
            task_type = TASK_MOM_SOLVE;
            break;
        case WORKFLOW_STEP_POSTPROCESSING:
            // Post-processing is a data processing step
            task_type = TASK_INTERFACE_UPDATE;
            break;
        case WORKFLOW_STEP_EXPORT:
            // Export is a data output step
            task_type = TASK_INTERFACE_UPDATE;
            break;
        default:
            // Default to interface update for unknown steps
            task_type = TASK_INTERFACE_UPDATE;
            break;
    }
    
    execution_order_add_task(engine->execution_graph, task_type, task_id);
    
    // Add dependencies between workflow steps
    // L5 layer manages execution dependencies, not implementation
    // Default: sequential execution (each step depends on previous)
    if (engine->num_steps > 0) {
        execution_order_add_dependency(engine->execution_graph, task_id, task_id - 1);
    }
    
    engine->num_steps++;
    
    return STATUS_SUCCESS;
}

int workflow_engine_execute(
    workflow_engine_t* engine) {
    
    if (!engine) return STATUS_ERROR_INVALID_INPUT;
    
    if (engine->is_executing) {
        return STATUS_ERROR_INVALID_STATE;  // Already executing
    }
    
    if (engine->num_steps == 0) {
        return STATUS_ERROR_INVALID_STATE;  // No steps to execute
    }
    
    // L5 layer orchestrates workflow, does NOT implement physics or numerical code
    
    engine->is_executing = true;
    engine->current_step_index = 0;
    
    // Compute execution order
    int* execution_sequence = (int*)malloc(engine->num_steps * sizeof(int));
    if (!execution_sequence) {
        engine->is_executing = false;
        return STATUS_ERROR_MEMORY_ALLOCATION;
    }
    
    int sequence_length = 0;
    status_t status = execution_order_compute_order(
        engine->execution_graph,
        execution_sequence,
        &sequence_length
    );
    
    if (status != STATUS_SUCCESS) {
        free(execution_sequence);
        engine->is_executing = false;
        return status;
    }
    
    // Execute steps in order
    for (int i = 0; i < sequence_length; i++) {
        int step_idx = execution_sequence[i];
        if (step_idx < 0 || step_idx >= engine->num_steps) {
            continue;
        }
        
        workflow_step_entry_t* entry = &engine->steps[step_idx];
        engine->current_step_index = step_idx;
        
        // Execute step (L5 orchestrates, actual implementation is in other layers)
        switch (entry->step) {
            case WORKFLOW_STEP_GEOMETRY_IMPORT:
                // L5 orchestrates: call L2 geometry engine
                // (Implementation would call geometry_engine_import_file)
                break;
                
            case WORKFLOW_STEP_MESH_GENERATION:
                // L5 orchestrates: call L2 mesh engine
                // (Implementation would call mesh_engine_generate)
                break;
                
            case WORKFLOW_STEP_MATRIX_ASSEMBLY:
                // L5 orchestrates: call L3 matrix assembler
                // (Implementation would call matrix_assembler_assemble_mom)
                break;
                
            case WORKFLOW_STEP_SOLUTION:
                // L5 orchestrates: call L4 solver
                // (Implementation would call solver_interface_solve)
                break;
                
            case WORKFLOW_STEP_POSTPROCESSING:
                // L5 orchestrates: call post-processing
                // (Implementation would call post-processing functions)
                break;
                
            case WORKFLOW_STEP_EXPORT:
                // L5 orchestrates: call L6 file I/O
                // (Implementation would call file_io_write_results)
                break;
        }
        
        entry->is_executed = true;
    }
    
    free(execution_sequence);
    engine->is_executing = false;
    engine->current_step_index = -1;
    
    return STATUS_SUCCESS;
}
