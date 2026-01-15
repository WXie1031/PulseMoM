/********************************************************************************
 *  PulseEM - Unified Mesh Platform Migration Guide
 *
 *  Copyright (C) 2025 PulseEM Technologies
 *
 *  Step-by-step migration from fragmented mesh generation to unified platform
 ********************************************************************************/

#ifndef MESH_MIGRATION_H
#define MESH_MIGRATION_H

#include "mesh_engine.h"
#include "../solvers/mom/mom_solver.h"
#include "../solvers/peec/peec_solver.h"

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************
 * LEGACY INTERFACE COMPATIBILITY LAYER
 * 
 * Provides backward compatibility while migrating to unified platform
 *********************************************************************/

/* Legacy MoM triangular mesh function (from tri_mesh.c) */
typedef struct {
    surface_t* surface;
    mesh_params_t* params;
    int num_triangles;
    int num_vertices;
    double* vertices;
    int* triangles;
    double* quality_metrics;
} legacy_mom_mesh_t;

/* Legacy PEEC Manhattan mesh function (from manhattan_mesh.c) */
typedef struct {
    surface_t* surface;
    mesh_params_t* params;
    int num_rectangles;
    int num_vertices;
    double* vertices;
    int* rectangles;
    int* via_connections;
    int num_vias;
} legacy_peec_mesh_t;

/*********************************************************************
 * MIGRATION FUNCTIONS
 *********************************************************************/

/**
 * @brief Migrate MoM solver to use unified mesh engine
 * 
 * Replaces calls to tri_mesh_generate_surface() with unified engine
 * while maintaining the same interface for backward compatibility
 */
legacy_mom_mesh_t* mesh_migrate_mom_mesh(surface_t* surface, mesh_params_t* params);

/**
 * @brief Migrate PEEC solver to use unified mesh engine
 * 
 * Replaces calls to manhattan_mesh_generate() with unified engine
 * while maintaining the same interface for backward compatibility
 */
legacy_peec_mesh_t* mesh_migrate_peec_mesh(surface_t* surface, mesh_params_t* params);

/**
 * @brief Convert legacy mesh to unified format
 * 
 * Converts existing legacy meshes to unified mesh format
 * for use with new optimization and coupling features
 */
mesh_mesh_t* mesh_migrate_legacy_to_unified(const legacy_mom_mesh_t* legacy_mesh);
mesh_mesh_t* mesh_migrate_legacy_peec_to_unified(const legacy_peec_mesh_t* legacy_mesh);

/**
 * @brief Update solver integration to use unified mesh
 * 
 * Updates MoM and PEEC solvers to work with unified mesh engine
 * while preserving existing solver-specific optimizations
 */
int mesh_migrate_solver_integration(mom_solver_t* mom_solver, peec_solver_t* peec_solver);

/*********************************************************************
 * GRADUAL MIGRATION STRATEGY
 *********************************************************************/

typedef enum {
    MIGRATION_PHASE_1,      /* Use unified engine, legacy interface */
    MIGRATION_PHASE_2,      /* Use unified interface, legacy solvers */
    MIGRATION_PHASE_3,      /* Full unified platform integration */
} migration_phase_t;

/**
 * @brief Configure migration phase
 * 
 * Controls the level of migration:
 * Phase 1: Transparent replacement of mesh generation
 * Phase 2: Update to unified mesh interface
 * Phase 3: Complete integration with new features
 */
int mesh_migrate_set_phase(migration_phase_t phase);
migration_phase_t mesh_migrate_get_phase(void);

/**
 * @brief Migration status and reporting
 */
typedef struct {
    int mom_meshes_migrated;
    int peec_meshes_migrated;
    int hybrid_meshes_created;
    double migration_time;
    int errors_encountered;
    migration_phase_t current_phase;
    bool migration_complete;
} migration_status_t;

migration_status_t mesh_migrate_get_status(void);
int mesh_migrate_print_report(const char* filename);

/*********************************************************************
 * FEATURE COMPARISON AND VALIDATION
 *********************************************************************/

/**
 * @brief Compare legacy vs unified mesh quality
 * 
 * Validates that unified engine produces equivalent or better
 * quality meshes compared to legacy implementations
 */
int mesh_migrate_compare_quality(const legacy_mom_mesh_t* legacy_mesh, 
                                const mesh_mesh_t* unified_mesh,
                                double* quality_improvement);

/**
 * @brief Performance comparison
 * 
 * Compares generation speed and memory usage between
 * legacy and unified implementations
 */
int mesh_migrate_compare_performance(const surface_t* surface,
                                   double* legacy_time, double* unified_time,
                                   double* legacy_memory, double* unified_memory);

/**
 * @brief Solution accuracy validation
 * 
 * Ensures that unified meshes produce equivalent
 * electromagnetic simulation results
 */
int mesh_migrate_validate_accuracy(mom_solver_t* solver,
                                 const legacy_mom_mesh_t* legacy_mesh,
                                 const mesh_mesh_t* unified_mesh,
                                 double* max_difference, double* rms_difference);

/*********************************************************************
 * ROLLBACK AND RECOVERY
 *********************************************************************/

/**
 * @brief Rollback to legacy mesh generation
 * 
 * Provides safe rollback capability if issues are encountered
 * during migration to unified platform
 */
int mesh_migrate_rollback_to_legacy(void);

/**
 * @brief Validate migration readiness
 * 
 * Checks that all prerequisites are met for successful migration
 */
int mesh_migrate_validate_readiness(char* error_msg, int max_msg_len);

#ifdef __cplusplus
}
#endif

#endif /* MESH_MIGRATION_H */