/********************************************************************************
 * Execution Order Management Implementation (L5 Execution Orchestration Layer)
 *
 * Copyright (C) 2025 PulseEM Technologies
 *
 * This file manages execution order and dependency graph.
 * L5 layer: Execution Orchestration - manages execution sequencing.
 *
 * Architecture Rule: L5 manages execution order, explicit dependency graph.
 ********************************************************************************/

#include "execution_order.h"
#include "../../common/types.h"
#include "../../common/errors.h"
#include <stdlib.h>
#include <string.h>

execution_graph_t* execution_order_create_graph(void) {
    execution_graph_t* graph = (execution_graph_t*)calloc(1, sizeof(execution_graph_t));
    if (!graph) return NULL;
    
    graph->capacity = 16;  // Initial capacity
    graph->tasks = (execution_task_t*)calloc(graph->capacity, sizeof(execution_task_t));
    graph->task_ids = (int*)calloc(graph->capacity, sizeof(int));
    graph->dependencies = (task_dependency_t*)calloc(graph->capacity, sizeof(task_dependency_t));
    
    if (!graph->tasks || !graph->task_ids || !graph->dependencies) {
        if (graph->tasks) free(graph->tasks);
        if (graph->task_ids) free(graph->task_ids);
        if (graph->dependencies) free(graph->dependencies);
        free(graph);
        return NULL;
    }
    
    graph->num_tasks = 0;
    graph->num_dependencies = 0;
    
    return graph;
}

void execution_order_destroy_graph(execution_graph_t* graph) {
    if (!graph) return;
    
    // Free dependencies
    if (graph->dependencies) {
        for (int i = 0; i < graph->num_dependencies; i++) {
            if (graph->dependencies[i].dependencies) {
                free(graph->dependencies[i].dependencies);
            }
        }
        free(graph->dependencies);
    }
    
    if (graph->tasks) free(graph->tasks);
    if (graph->task_ids) free(graph->task_ids);
    free(graph);
}

int execution_order_add_task(
    execution_graph_t* graph,
    execution_task_t task_type,
    int task_id) {
    
    if (!graph) return STATUS_ERROR_INVALID_INPUT;
    
    // Check if task_id already exists
    for (int i = 0; i < graph->num_tasks; i++) {
        if (graph->task_ids[i] == task_id) {
            return STATUS_ERROR_INVALID_INPUT;  // Duplicate task ID
        }
    }
    
    // Resize if needed
    if (graph->num_tasks >= graph->capacity) {
        int new_capacity = graph->capacity * 2;
        execution_task_t* new_tasks = (execution_task_t*)realloc(graph->tasks, new_capacity * sizeof(execution_task_t));
        int* new_task_ids = (int*)realloc(graph->task_ids, new_capacity * sizeof(int));
        
        if (!new_tasks || !new_task_ids) {
            return STATUS_ERROR_MEMORY_ALLOCATION;
        }
        
        graph->tasks = new_tasks;
        graph->task_ids = new_task_ids;
        graph->capacity = new_capacity;
    }
    
    // Add task
    graph->tasks[graph->num_tasks] = task_type;
    graph->task_ids[graph->num_tasks] = task_id;
    graph->num_tasks++;
    
    // Initialize dependency for this task
    if (graph->num_dependencies < graph->num_tasks) {
        // Resize dependencies array
        task_dependency_t* new_deps = (task_dependency_t*)realloc(
            graph->dependencies, graph->num_tasks * sizeof(task_dependency_t));
        if (!new_deps) {
            return STATUS_ERROR_MEMORY_ALLOCATION;
        }
        graph->dependencies = new_deps;
        
        // Initialize new dependency entry
        graph->dependencies[graph->num_tasks - 1].task_id = task_id;
        graph->dependencies[graph->num_tasks - 1].dependencies = NULL;
        graph->dependencies[graph->num_tasks - 1].num_dependencies = 0;
    }
    
    return STATUS_SUCCESS;
}

int execution_order_add_dependency(
    execution_graph_t* graph,
    int task_id,
    int dependency_id) {
    
    if (!graph) return STATUS_ERROR_INVALID_INPUT;
    
    // Find task
    int task_idx = -1;
    for (int i = 0; i < graph->num_tasks; i++) {
        if (graph->task_ids[i] == task_id) {
            task_idx = i;
            break;
        }
    }
    
    if (task_idx < 0) {
        return STATUS_ERROR_INVALID_INPUT;  // Task not found
    }
    
    // Verify dependency exists
    bool dependency_exists = false;
    for (int i = 0; i < graph->num_tasks; i++) {
        if (graph->task_ids[i] == dependency_id) {
            dependency_exists = true;
            break;
        }
    }
    
    if (!dependency_exists) {
        return STATUS_ERROR_INVALID_INPUT;  // Dependency not found
    }
    
    // Check for circular dependency
    // First check: self-dependency
    if (task_id == dependency_id) {
        return STATUS_ERROR_INVALID_INPUT;  // Self-dependency
    }
    
    // Second check: would adding this dependency create a cycle?
    // Use DFS to check if dependency_id can reach task_id
    // If yes, adding task_id -> dependency_id would create a cycle
    // This is a simplified check - full implementation would use proper DFS
    // For now, we only check direct dependencies
    for (int i = 0; i < graph->num_tasks; i++) {
        if (graph->task_ids[i] == dependency_id) {
            task_dependency_t* dep = &graph->dependencies[i];
            for (int j = 0; j < dep->num_dependencies; j++) {
                if (dep->dependencies[j] == task_id) {
                    return STATUS_ERROR_INVALID_INPUT;  // Circular dependency detected
                }
            }
        }
    }
    
    // Add dependency
    task_dependency_t* dep = &graph->dependencies[task_idx];
    
    // Resize dependencies array if needed
    if (dep->num_dependencies == 0) {
        dep->dependencies = (int*)malloc(sizeof(int));
    } else {
        dep->dependencies = (int*)realloc(dep->dependencies, 
                                          (dep->num_dependencies + 1) * sizeof(int));
    }
    
    if (!dep->dependencies) {
        return STATUS_ERROR_MEMORY_ALLOCATION;
    }
    
    dep->dependencies[dep->num_dependencies] = dependency_id;
    dep->num_dependencies++;
    
    return STATUS_SUCCESS;
}

// Helper: Topological sort (Kahn's algorithm)
static int topological_sort(
    const execution_graph_t* graph,
    int* execution_sequence,
    int* sequence_length) {
    
    if (!graph || !execution_sequence || !sequence_length) {
        return STATUS_ERROR_INVALID_INPUT;
    }
    
    // Compute in-degree for each task
    int* in_degree = (int*)calloc(graph->num_tasks, sizeof(int));
    if (!in_degree) return STATUS_ERROR_MEMORY_ALLOCATION;
    
    for (int i = 0; i < graph->num_tasks; i++) {
        task_dependency_t* dep = &graph->dependencies[i];
        for (int j = 0; j < dep->num_dependencies; j++) {
            // Find dependency task index
            for (int k = 0; k < graph->num_tasks; k++) {
                if (graph->task_ids[k] == dep->dependencies[j]) {
                    in_degree[i]++;
                    break;
                }
            }
        }
    }
    
    // Find tasks with no dependencies
    int* queue = (int*)malloc(graph->num_tasks * sizeof(int));
    if (!queue) {
        free(in_degree);
        return STATUS_ERROR_MEMORY_ALLOCATION;
    }
    
    int queue_front = 0;
    int queue_back = 0;
    
    for (int i = 0; i < graph->num_tasks; i++) {
        if (in_degree[i] == 0) {
            queue[queue_back++] = i;
        }
    }
    
    // Process queue
    int seq_idx = 0;
    while (queue_front < queue_back) {
        int task_idx = queue[queue_front++];
        execution_sequence[seq_idx++] = graph->task_ids[task_idx];
        
        // Decrease in-degree of dependent tasks
        for (int i = 0; i < graph->num_tasks; i++) {
            task_dependency_t* dep = &graph->dependencies[i];
            for (int j = 0; j < dep->num_dependencies; j++) {
                if (graph->task_ids[task_idx] == dep->dependencies[j]) {
                    in_degree[i]--;
                    if (in_degree[i] == 0) {
                        queue[queue_back++] = i;
                    }
                    break;
                }
            }
        }
    }
    
    free(in_degree);
    free(queue);
    
    *sequence_length = seq_idx;
    
    // Check for cycles (if seq_idx < num_tasks, there's a cycle)
    if (seq_idx < graph->num_tasks) {
        return STATUS_ERROR_INVALID_STATE;  // Circular dependency
    }
    
    return STATUS_SUCCESS;
}

int execution_order_compute_order(
    const execution_graph_t* graph,
    int* execution_sequence,
    int* sequence_length) {
    
    if (!graph || !execution_sequence || !sequence_length) {
        return STATUS_ERROR_INVALID_INPUT;
    }
    
    // L5 layer computes topological sort
    return topological_sort(graph, execution_sequence, sequence_length);
}

bool execution_order_validate(
    const execution_graph_t* graph) {
    
    if (!graph) return false;
    
    // Check for duplicate task IDs
    for (int i = 0; i < graph->num_tasks; i++) {
        for (int j = i + 1; j < graph->num_tasks; j++) {
            if (graph->task_ids[i] == graph->task_ids[j]) {
                return false;  // Duplicate task ID
            }
        }
    }
    
    // Check for circular dependencies
    // This is a basic check - full implementation would use DFS/BFS
    // For now, check for:
    // 1. Self-dependencies
    // 2. Direct circular dependencies (A -> B -> A)
    for (int i = 0; i < graph->num_tasks; i++) {
        task_dependency_t* dep = &graph->dependencies[i];
        for (int j = 0; j < dep->num_dependencies; j++) {
            int dep_id = dep->dependencies[j];
            
            // Check for self-dependency
            if (dep_id == graph->task_ids[i]) {
                return false;  // Self-dependency
            }
            
            // Check for direct circular dependency
            // Find the task with dep_id and check if it depends on task_ids[i]
            for (int k = 0; k < graph->num_tasks; k++) {
                if (graph->task_ids[k] == dep_id) {
                    task_dependency_t* dep2 = &graph->dependencies[k];
                    for (int l = 0; l < dep2->num_dependencies; l++) {
                        if (dep2->dependencies[l] == graph->task_ids[i]) {
                            return false;  // Circular dependency: task_ids[i] -> dep_id -> task_ids[i]
                        }
                    }
                    break;
                }
            }
        }
    }
    
    return true;
}
