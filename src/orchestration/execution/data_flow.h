/********************************************************************************
 * Data Flow Management (L5 Execution Orchestration Layer)
 *
 * Copyright (C) 2025 PulseEM Technologies
 *
 * This file manages data flow between tasks.
 * L5 layer: Execution Orchestration - manages data flow.
 *
 * Architecture Rule: L5 manages data flow, explicit data dependencies.
 ********************************************************************************/

#ifndef DATA_FLOW_H
#define DATA_FLOW_H

#include "../../common/types.h"
#include "../../common/constants.h"
#include "../../operators/assembler/matrix_assembler.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Data Flow Types
// ============================================================================

/**
 * Data Type
 */
typedef enum {
    DATA_TYPE_MATRIX = 1,          // Operator matrix
    DATA_TYPE_VECTOR = 2,          // Operator vector
    DATA_TYPE_SOLUTION = 3,        // Solution vector
    DATA_TYPE_INTERFACE = 4        // Interface data
} data_type_t;

/**
 * Data Handle
 */
typedef struct {
    int data_id;
    data_type_t type;
    void* data_ptr;                // Opaque pointer to data
    size_t data_size;
    int producer_task_id;          // Task that produces this data
    int* consumer_task_ids;        // Tasks that consume this data
    int num_consumers;
} data_handle_t;

/**
 * Data Flow Graph
 */
typedef struct {
    data_handle_t* data_handles;
    int num_data_handles;
    int capacity;               // Capacity of data_handles array (for dynamic resizing)
} data_flow_graph_t;

// ============================================================================
// Data Flow Interface
// ============================================================================

/**
 * Create data flow graph
 */
data_flow_graph_t* data_flow_create_graph(void);

/**
 * Destroy data flow graph
 */
void data_flow_destroy_graph(data_flow_graph_t* graph);

/**
 * Register data
 */
int data_flow_register_data(
    data_flow_graph_t* graph,
    int data_id,
    data_type_t type,
    void* data_ptr,
    size_t data_size,
    int producer_task_id
);

/**
 * Add data consumer
 */
int data_flow_add_consumer(
    data_flow_graph_t* graph,
    int data_id,
    int consumer_task_id
);

/**
 * Get data handle
 */
const data_handle_t* data_flow_get_handle(
    const data_flow_graph_t* graph,
    int data_id
);

#ifdef __cplusplus
}
#endif

#endif // DATA_FLOW_H
