/********************************************************************************
 * Workflow Engine (L5 Execution Orchestration Layer)
 *
 * Copyright (C) 2025 PulseEM Technologies
 *
 * This file defines workflow orchestration.
 * L5 layer: Execution Orchestration - manages workflow execution.
 *
 * Architecture Rule: L5 orchestrates workflow, does NOT implement physics or numerical code.
 ********************************************************************************/

#ifndef WORKFLOW_ENGINE_H
#define WORKFLOW_ENGINE_H

#include "../../common/types.h"
#include "../../common/constants.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Workflow Step
 */
typedef enum {
    WORKFLOW_STEP_GEOMETRY_IMPORT = 1,
    WORKFLOW_STEP_MESH_GENERATION = 2,
    WORKFLOW_STEP_MATRIX_ASSEMBLY = 3,
    WORKFLOW_STEP_SOLUTION = 4,
    WORKFLOW_STEP_POSTPROCESSING = 5,
    WORKFLOW_STEP_EXPORT = 6
} workflow_step_t;

/**
 * Workflow Engine
 */
typedef struct workflow_engine workflow_engine_t;

/**
 * Create workflow engine
 */
workflow_engine_t* workflow_engine_create(void);

/**
 * Destroy workflow engine
 */
void workflow_engine_destroy(workflow_engine_t* engine);

/**
 * Add workflow step
 */
int workflow_engine_add_step(
    workflow_engine_t* engine,
    workflow_step_t step,
    void* step_config
);

/**
 * Execute workflow
 */
int workflow_engine_execute(
    workflow_engine_t* engine
);

#ifdef __cplusplus
}
#endif

#endif // WORKFLOW_ENGINE_H
