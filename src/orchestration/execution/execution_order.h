/********************************************************************************
 * Execution Order Management (L5 Execution Orchestration Layer)
 *
 * Copyright (C) 2025 PulseEM Technologies
 *
 * This file manages execution order and dependency graph.
 * L5 layer: Execution Orchestration - manages execution sequencing.
 *
 * Architecture Rule: L5 manages execution order, explicit dependency graph.
 ********************************************************************************/

#ifndef EXECUTION_ORDER_H
#define EXECUTION_ORDER_H

#include "../../common/types.h"
#include "../../common/constants.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Execution Task Types
// ============================================================================

/**
 * Execution Task
 */
typedef enum {
    TASK_MOM_SOLVE = 1,           // MoM solve task
    TASK_PEEC_SOLVE = 2,          // PEEC solve task
    TASK_MTL_SOLVE = 3,           // MTL solve task
    TASK_COUPLING_TRANSFER = 4,   // Coupling transfer task
    TASK_INTERFACE_UPDATE = 5,    // Interface update task
    TASK_CONVERGENCE_CHECK = 6    // Convergence check task
} execution_task_t;

/**
 * Task Dependency
 */
typedef struct {
    int task_id;
    int* dependencies;            // Array of dependent task IDs
    int num_dependencies;
} task_dependency_t;

/**
 * Execution Graph
 * 
 * L5 layer defines explicit dependency graph
 */
typedef struct {
    execution_task_t* tasks;
    task_dependency_t* dependencies;
    int* task_ids;              // Array of task IDs (parallel to tasks array)
    int num_tasks;
    int num_dependencies;
    int capacity;               // Capacity of arrays (for dynamic resizing)
} execution_graph_t;

// ============================================================================
// Execution Order Interface
// ============================================================================

/**
 * Create execution graph
 */
execution_graph_t* execution_order_create_graph(void);

/**
 * Destroy execution graph
 */
void execution_order_destroy_graph(execution_graph_t* graph);

/**
 * Add task to graph
 */
int execution_order_add_task(
    execution_graph_t* graph,
    execution_task_t task_type,
    int task_id
);

/**
 * Add dependency
 */
int execution_order_add_dependency(
    execution_graph_t* graph,
    int task_id,
    int dependency_id
);

/**
 * Compute execution order
 * 
 * L5 layer computes topological sort
 */
int execution_order_compute_order(
    const execution_graph_t* graph,
    int* execution_sequence,
    int* sequence_length
);

/**
 * Validate execution graph
 */
bool execution_order_validate(
    const execution_graph_t* graph
);

#ifdef __cplusplus
}
#endif

#endif // EXECUTION_ORDER_H
