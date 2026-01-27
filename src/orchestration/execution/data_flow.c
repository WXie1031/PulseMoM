/********************************************************************************
 * Data Flow Management Implementation (L5 Execution Orchestration Layer)
 *
 * Copyright (C) 2025 PulseEM Technologies
 *
 * This file manages data flow between tasks.
 * L5 layer: Execution Orchestration - manages data flow.
 *
 * Architecture Rule: L5 manages data flow, explicit data dependencies.
 ********************************************************************************/

#include "data_flow.h"
#include "../../common/types.h"
#include "../../common/errors.h"
#include <stdlib.h>
#include <string.h>

data_flow_graph_t* data_flow_create_graph(void) {
    data_flow_graph_t* graph = (data_flow_graph_t*)calloc(1, sizeof(data_flow_graph_t));
    if (!graph) return NULL;
    
    graph->capacity = 16;  // Initial capacity
    graph->data_handles = (data_handle_t*)calloc(graph->capacity, sizeof(data_handle_t));
    if (!graph->data_handles) {
        free(graph);
        return NULL;
    }
    
    graph->num_data_handles = 0;
    
    return graph;
}

void data_flow_destroy_graph(data_flow_graph_t* graph) {
    if (!graph) return;
    
    if (graph->data_handles) {
        for (int i = 0; i < graph->num_data_handles; i++) {
            if (graph->data_handles[i].consumer_task_ids) {
                free(graph->data_handles[i].consumer_task_ids);
            }
        }
        free(graph->data_handles);
    }
    
    free(graph);
}

int data_flow_register_data(
    data_flow_graph_t* graph,
    int data_id,
    data_type_t type,
    void* data_ptr,
    size_t data_size,
    int producer_task_id) {
    
    if (!graph || !data_ptr || data_id < 0) {
        return STATUS_ERROR_INVALID_INPUT;
    }
    
    // Check if data_id already exists
    for (int i = 0; i < graph->num_data_handles; i++) {
        if (graph->data_handles[i].data_id == data_id) {
            return STATUS_ERROR_INVALID_INPUT;  // Duplicate data ID
        }
    }
    
    // Resize if needed
    if (graph->num_data_handles >= graph->capacity) {
        int new_capacity = graph->capacity * 2;
        data_handle_t* new_handles = (data_handle_t*)realloc(
            graph->data_handles, new_capacity * sizeof(data_handle_t));
        if (!new_handles) {
            return STATUS_ERROR_MEMORY_ALLOCATION;
        }
        graph->data_handles = new_handles;
        graph->capacity = new_capacity;
    }
    
    // Add data handle
    data_handle_t* handle = &graph->data_handles[graph->num_data_handles];
    handle->data_id = data_id;
    handle->type = type;
    handle->data_ptr = data_ptr;
    handle->data_size = data_size;
    handle->producer_task_id = producer_task_id;
    handle->consumer_task_ids = NULL;
    handle->num_consumers = 0;
    
    graph->num_data_handles++;
    
    return STATUS_SUCCESS;
}

int data_flow_add_consumer(
    data_flow_graph_t* graph,
    int data_id,
    int consumer_task_id) {
    
    if (!graph || data_id < 0 || consumer_task_id < 0) {
        return STATUS_ERROR_INVALID_INPUT;
    }
    
    // Find data handle
    data_handle_t* handle = NULL;
    for (int i = 0; i < graph->num_data_handles; i++) {
        if (graph->data_handles[i].data_id == data_id) {
            handle = &graph->data_handles[i];
            break;
        }
    }
    
    if (!handle) {
        return STATUS_ERROR_INVALID_INPUT;  // Data not found
    }
    
    // Check if consumer already exists
    for (int i = 0; i < handle->num_consumers; i++) {
        if (handle->consumer_task_ids[i] == consumer_task_id) {
            return STATUS_SUCCESS;  // Already registered
        }
    }
    
    // Add consumer
    if (handle->num_consumers == 0) {
        handle->consumer_task_ids = (int*)malloc(sizeof(int));
    } else {
        handle->consumer_task_ids = (int*)realloc(
            handle->consumer_task_ids, (handle->num_consumers + 1) * sizeof(int));
    }
    
    if (!handle->consumer_task_ids) {
        return STATUS_ERROR_MEMORY_ALLOCATION;
    }
    
    handle->consumer_task_ids[handle->num_consumers] = consumer_task_id;
    handle->num_consumers++;
    
    return STATUS_SUCCESS;
}

const data_handle_t* data_flow_get_handle(
    const data_flow_graph_t* graph,
    int data_id) {
    
    if (!graph || data_id < 0) return NULL;
    
    for (int i = 0; i < graph->num_data_handles; i++) {
        if (graph->data_handles[i].data_id == data_id) {
            return &graph->data_handles[i];
        }
    }
    
    return NULL;
}
