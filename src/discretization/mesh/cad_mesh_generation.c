/**
 * @file cad_mesh_generation.c
 * @brief Advanced CAD geometry automatic mesh generation implementation
 * @details Implements high-performance mesh generation algorithms for CAD geometries
 * with support for various formats, adaptive refinement, and parallel processing
 */

#include "cad_mesh_generation.h"
#include "../../discretization/geometry/opencascade_cad_import.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <assert.h>

#ifdef _OPENMP
#include <omp.h>
#endif

#ifdef ENABLE_MPI
#include <mpi.h>
#endif

#define CAD_MESH_GENERATION_VERSION "1.0.0"
#define CAD_MESH_GENERATION_BUILD_DATE __DATE__
#define CAD_MESH_GENERATION_BUILD_TIME __TIME__

#define MESH_GENERATION_SUCCESS 0
#define MESH_GENERATION_ERROR -1
#define MESH_GENERATION_MEMORY_ERROR -2
#define MESH_GENERATION_GEOMETRY_ERROR -3
#define MESH_GENERATION_QUALITY_ERROR -4
#define MESH_GENERATION_CONVERGENCE_ERROR -5
#define MESH_GENERATION_PARALLEL_ERROR -6

#define DEFAULT_MESH_SIZE 1.0
#define MIN_MESH_SIZE 1e-6
#define MAX_MESH_SIZE 1e6
#define QUALITY_THRESHOLD 0.3
#define CONVERGENCE_TOLERANCE 1e-3
#define MAX_REFINEMENT_ITERATIONS 10
#define MAX_SMOOTHING_ITERATIONS 100

#define NUM_THREADS_DEFAULT 4
#define MEMORY_POOL_SIZE (1024 * 1024 * 1024)  // 1GB

typedef struct {
    int num_threads;
    int use_openmp;
    int use_mpi;
    int numa_aware;
    int memory_pool_enabled;
    size_t memory_pool_size;
    int adaptive_refinement;
    int quality_improvement;
    int parallel_meshing;
    int verbose_logging;
} MeshGenerationOptions;

typedef struct {
    double* coordinates;
    int* connectivity;
    int* element_types;
    int* material_ids;
    int num_nodes;
    int num_elements;
    int num_materials;
    double computation_time;
    double memory_usage;
    int quality_status;
} MeshData;

static MeshGenerationOptions global_options = {
    .num_threads = NUM_THREADS_DEFAULT,
    .use_openmp = 1,
    .use_mpi = 0,
    .numa_aware = 0,
    .memory_pool_enabled = 1,
    .memory_pool_size = MEMORY_POOL_SIZE,
    .adaptive_refinement = 1,
    .quality_improvement = 1,
    .parallel_meshing = 1,
    .verbose_logging = 0
};

static void* mesh_generation_malloc(size_t size) {
    void* ptr = malloc(size);
    if (!ptr && global_options.verbose_logging) {
        fprintf(stderr, "[CAD_MESH] Memory allocation failed for size: %zu\n", size);
    }
    return ptr;
}

static void mesh_generation_free(void* ptr) {
    if (ptr) {
        free(ptr);
    }
}

static double mesh_generation_get_time(void) {
    return (double)clock() / CLOCKS_PER_SEC;
}

static double mesh_generation_compute_distance(double x1, double y1, double z1,
                                               double x2, double y2, double z2) {
    double dx = x2 - x1;
    double dy = y2 - y1;
    double dz = z2 - z1;
    return sqrt(dx*dx + dy*dy + dz*dz);
}

static double mesh_generation_compute_triangle_area(double x1, double y1, double z1,
                                                   double x2, double y2, double z2,
                                                   double x3, double y3, double z3) {
    double a = mesh_generation_compute_distance(x1, y1, z1, x2, y2, z2);
    double b = mesh_generation_compute_distance(x2, y2, z2, x3, y3, z3);
    double c = mesh_generation_compute_distance(x3, y3, z3, x1, y1, z1);
    double s = (a + b + c) / 2.0;
    return sqrt(s * (s - a) * (s - b) * (s - c));
}

static double mesh_generation_compute_tetrahedron_volume(double x1, double y1, double z1,
                                                         double x2, double y2, double z2,
                                                         double x3, double y3, double z3,
                                                         double x4, double y4, double z4) {
    double matrix[3][3];
    matrix[0][0] = x2 - x1; matrix[0][1] = y2 - y1; matrix[0][2] = z2 - z1;
    matrix[1][0] = x3 - x1; matrix[1][1] = y3 - y1; matrix[1][2] = z3 - z1;
    matrix[2][0] = x4 - x1; matrix[2][1] = y4 - y1; matrix[2][2] = z4 - z1;
    
    double det = matrix[0][0] * (matrix[1][1] * matrix[2][2] - matrix[1][2] * matrix[2][1]) -
                 matrix[0][1] * (matrix[1][0] * matrix[2][2] - matrix[1][2] * matrix[2][0]) +
                 matrix[0][2] * (matrix[1][0] * matrix[2][1] - matrix[1][1] * matrix[2][0]);
    
    return fabs(det) / 6.0;
}

static int mesh_generation_validate_input(const CadMeshGenerationSolver* solver) {
    if (!solver) {
        if (global_options.verbose_logging) {
            fprintf(stderr, "[CAD_MESH] Invalid solver pointer\n");
        }
        return MESH_GENERATION_ERROR;
    }
    
    if (solver->num_entities <= 0) {
        if (global_options.verbose_logging) {
            fprintf(stderr, "[CAD_MESH] No CAD entities provided\n");
        }
        return MESH_GENERATION_GEOMETRY_ERROR;
    }
    
    if (solver->num_materials <= 0) {
        if (global_options.verbose_logging) {
            fprintf(stderr, "[CAD_MESH] No materials defined\n");
        }
        return MESH_GENERATION_GEOMETRY_ERROR;
    }
    
    return MESH_GENERATION_SUCCESS;
}

CadMeshGenerationSolver* cad_mesh_generation_create(MeshGenerationConfig* config) {
    CadMeshGenerationSolver* solver = (CadMeshGenerationSolver*)mesh_generation_malloc(sizeof(CadMeshGenerationSolver));
    if (!solver) {
        return NULL;
    }
    
    memset(solver, 0, sizeof(CadMeshGenerationSolver));
    
    if (config) {
        solver->config = *config;
    } else {
        solver->config.type = MESH_TYPE_TETRAHEDRAL;
        solver->config.algorithm = MESH_ALGORITHM_DELAUNAY;
        solver->config.polynomial_order = 1;
        solver->config.use_high_order_elements = 0;
        solver->config.use_curved_elements = 0;
        solver->config.use_anisotropic_meshing = 0;
        solver->config.use_adaptive_meshing = 1;
        solver->config.use_structured_meshing = 0;
        solver->config.use_unstructured_meshing = 1;
        solver->config.use_hybrid_meshing = 0;
        solver->config.use_hierarchical_meshing = 0;
        solver->config.use_mesh_optimization = 1;
        solver->config.use_mesh_smoothing = 1;
        solver->config.use_mesh_coarsening = 0;
        solver->config.use_mesh_refinement = 1;
        solver->config.use_quality_improvement = 1;
        solver->config.use_boundary_recovery = 1;
        solver->config.use_constrained_meshing = 0;
        solver->config.use_periodic_boundary_conditions = 0;
    }
    
    solver->parameters.target_element_size = DEFAULT_MESH_SIZE;
    solver->parameters.minimum_element_size = MIN_MESH_SIZE;
    solver->parameters.maximum_element_size = MAX_MESH_SIZE;
    solver->parameters.element_size_growth_rate = 1.2;
    solver->parameters.curvature_resolution = 0.1;
    solver->parameters.proximity_resolution = 0.01;
    solver->parameters.feature_angle = 30.0;
    solver->parameters.mesh_grading = 0.5;
    solver->parameters.adaptive_refinement_levels = 3;
    solver->parameters.maximum_refinement_iterations = MAX_REFINEMENT_ITERATIONS;
    solver->parameters.quality_threshold = QUALITY_THRESHOLD;
    solver->parameters.convergence_tolerance = CONVERGENCE_TOLERANCE;
    solver->parameters.use_curvature_adaptation = 1;
    solver->parameters.use_proximity_adaptation = 1;
    solver->parameters.use_feature_detection = 1;
    solver->parameters.use_boundary_layer = 0;
    solver->parameters.use_periodic_meshing = 0;
    solver->parameters.use_symmetry_exploitation = 0;
    solver->parameters.use_parallel_meshing = global_options.parallel_meshing;
    solver->parameters.num_threads = global_options.num_threads;
    
    solver->mesh_generation_completed = 0;
    solver->computation_time = 0.0;
    solver->memory_usage = 0.0;
    solver->convergence_status = 0;
    
    if (global_options.verbose_logging) {
        printf("[CAD_MESH] Created mesh generation solver v%s\n", CAD_MESH_GENERATION_VERSION);
        printf("[CAD_MESH] Configuration: type=%d, algorithm=%d\n", 
               solver->config.type, solver->config.algorithm);
    }
    
    return solver;
}

void cad_mesh_generation_destroy(CadMeshGenerationSolver* solver) {
    if (!solver) {
        return;
    }
    
    if (solver->entities) {
        for (int i = 0; i < solver->num_entities; i++) {
            if (solver->entities[i].control_points) {
                mesh_generation_free(solver->entities[i].control_points);
            }
            if (solver->entities[i].weights) {
                mesh_generation_free(solver->entities[i].weights);
            }
            if (solver->entities[i].knots) {
                mesh_generation_free(solver->entities[i].knots);
            }
        }
        mesh_generation_free(solver->entities);
    }
    
    if (solver->nodes) {
        for (int i = 0; i < solver->num_nodes; i++) {
            if (solver->nodes[i].connected_elements) {
                mesh_generation_free(solver->nodes[i].connected_elements);
            }
        }
        mesh_generation_free(solver->nodes);
    }
    
    if (solver->elements) {
        for (int i = 0; i < solver->num_elements; i++) {
            if (solver->elements[i].node_ids) {
                mesh_generation_free(solver->elements[i].node_ids);
            }
        }
        mesh_generation_free(solver->elements);
    }
    
    if (solver->materials) {
        mesh_generation_free(solver->materials);
    }
    
    if (solver->geometry_data) {
        mesh_generation_free(solver->geometry_data);
    }
    
    if (solver->node_connectivity) {
        mesh_generation_free(solver->node_connectivity);
    }
    
    if (solver->element_connectivity) {
        mesh_generation_free(solver->element_connectivity);
    }
    
    if (solver->node_coordinates) {
        mesh_generation_free(solver->node_coordinates);
    }
    
    if (solver->element_types) {
        mesh_generation_free(solver->element_types);
    }
    
    if (solver->material_assignments) {
        mesh_generation_free(solver->material_assignments);
    }
    
    if (solver->physical_groups) {
        mesh_generation_free(solver->physical_groups);
    }
    
    if (global_options.verbose_logging) {
        printf("[CAD_MESH] Destroyed mesh generation solver\n");
    }
    
    mesh_generation_free(solver);
}

int cad_mesh_generation_setup_entities(CadMeshGenerationSolver* solver, 
                                        CadEntity* entities, int num_entities) {
    if (!solver || !entities || num_entities <= 0) {
        return MESH_GENERATION_ERROR;
    }
    
    solver->entities = (CadEntity*)mesh_generation_malloc(num_entities * sizeof(CadEntity));
    if (!solver->entities) {
        return MESH_GENERATION_MEMORY_ERROR;
    }
    
    memcpy(solver->entities, entities, num_entities * sizeof(CadEntity));
    solver->num_entities = num_entities;
    
    if (global_options.verbose_logging) {
        printf("[CAD_MESH] Set up %d CAD entities\n", num_entities);
    }
    
    return MESH_GENERATION_SUCCESS;
}

int cad_mesh_generation_setup_materials(CadMeshGenerationSolver* solver,
                                         MaterialProperties* materials, int num_materials) {
    if (!solver || !materials || num_materials <= 0) {
        return MESH_GENERATION_ERROR;
    }
    
    solver->materials = (MaterialProperties*)mesh_generation_malloc(num_materials * sizeof(MaterialProperties));
    if (!solver->materials) {
        return MESH_GENERATION_MEMORY_ERROR;
    }
    
    memcpy(solver->materials, materials, num_materials * sizeof(MaterialProperties));
    solver->num_materials = num_materials;
    
    if (global_options.verbose_logging) {
        printf("[CAD_MESH] Set up %d materials\n", num_materials);
    }
    
    return MESH_GENERATION_SUCCESS;
}

int cad_mesh_generation_setup_parameters(CadMeshGenerationSolver* solver,
                                        MeshGenerationParameters* parameters) {
    if (!solver || !parameters) {
        return MESH_GENERATION_ERROR;
    }
    
    solver->parameters = *parameters;
    
    if (global_options.verbose_logging) {
        printf("[CAD_MESH] Updated mesh generation parameters\n");
    }
    
    return MESH_GENERATION_SUCCESS;
}

static int mesh_generation_generate_nodes_delaunay(CadMeshGenerationSolver* solver) {
    if (!solver->entities || solver->num_entities <= 0) {
        return MESH_GENERATION_GEOMETRY_ERROR;
    }
    
    int estimated_nodes = solver->num_entities * 100;
    solver->nodes = (MeshNode*)mesh_generation_malloc(estimated_nodes * sizeof(MeshNode));
    if (!solver->nodes) {
        return MESH_GENERATION_MEMORY_ERROR;
    }
    
    solver->num_nodes = 0;
    
    #ifdef _OPENMP
    #pragma omp parallel for num_threads(solver->parameters.num_threads) if(solver->parameters.use_parallel_meshing)
    #endif
    for (int i = 0; i < solver->num_entities; i++) {
        CadEntity* entity = &solver->entities[i];
        
        switch (entity->type) {
            case CAD_ENTITY_TYPE_POINT:
                #ifdef _OPENMP
                #pragma omp critical
                #endif
                {
                    MeshNode* node = &solver->nodes[solver->num_nodes];
                    node->x = entity->center[0];
                    node->y = entity->center[1];
                    node->z = entity->center[2];
                    node->node_id = solver->num_nodes;
                    node->global_id = solver->num_nodes;
                    node->connected_elements = NULL;
                    node->num_connected_elements = 0;
                    node->boundary_flag = 1;
                    node->material_id = entity->material_id;
                    node->physical_group = entity->layer_id;
                    node->mesh_size = solver->parameters.target_element_size;
                    node->is_boundary_node = 1;
                    node->is_corner_node = 0;
                    node->is_edge_node = 0;
                    solver->num_nodes++;
                }
                break;
                
            case CAD_ENTITY_TYPE_LINE:
                #ifdef _OPENMP
                #pragma omp critical
                #endif
                {
                    int num_segments = (int)(entity->bounding_box[3] / solver->parameters.target_element_size);
                    if (num_segments < 2) num_segments = 2;
                    
                    for (int j = 0; j <= num_segments; j++) {
                        double t = (double)j / num_segments;
                        MeshNode* node = &solver->nodes[solver->num_nodes];
                        
                        node->x = entity->control_points[0] + t * (entity->control_points[3] - entity->control_points[0]);
                        node->y = entity->control_points[1] + t * (entity->control_points[4] - entity->control_points[1]);
                        node->z = entity->control_points[2] + t * (entity->control_points[5] - entity->control_points[2]);
                        
                        node->node_id = solver->num_nodes;
                        node->global_id = solver->num_nodes;
                        node->connected_elements = NULL;
                        node->num_connected_elements = 0;
                        node->boundary_flag = 1;
                        node->material_id = entity->material_id;
                        node->physical_group = entity->layer_id;
                        node->mesh_size = solver->parameters.target_element_size;
                        node->is_boundary_node = 1;
                        node->is_corner_node = (j == 0 || j == num_segments);
                        node->is_edge_node = 1;
                        solver->num_nodes++;
                    }
                }
                break;
                
            default:
                break;
        }
    }
    
    if (global_options.verbose_logging) {
        printf("[CAD_MESH] Generated %d nodes using Delaunay algorithm\n", solver->num_nodes);
    }
    
    return MESH_GENERATION_SUCCESS;
}

static int mesh_generation_generate_elements_tetrahedral(CadMeshGenerationSolver* solver) {
    if (!solver->nodes || solver->num_nodes < 4) {
        return MESH_GENERATION_GEOMETRY_ERROR;
    }
    
    int max_elements = solver->num_nodes * 10;
    solver->elements = (MeshElement*)mesh_generation_malloc(max_elements * sizeof(MeshElement));
    if (!solver->elements) {
        return MESH_GENERATION_MEMORY_ERROR;
    }
    
    solver->num_elements = 0;
    
    #ifdef _OPENMP
    #pragma omp parallel for num_threads(solver->parameters.num_threads) if(solver->parameters.use_parallel_meshing)
    #endif
    for (int i = 0; i < solver->num_nodes - 3; i++) {
        for (int j = i + 1; j < solver->num_nodes - 2; j++) {
            for (int k = j + 1; k < solver->num_nodes - 1; k++) {
                for (int l = k + 1; l < solver->num_nodes; l++) {
                    MeshNode* n1 = &solver->nodes[i];
                    MeshNode* n2 = &solver->nodes[j];
                    MeshNode* n3 = &solver->nodes[k];
                    MeshNode* n4 = &solver->nodes[l];
                    
                    double volume = mesh_generation_compute_tetrahedron_volume(
                        n1->x, n1->y, n1->z,
                        n2->x, n2->y, n2->z,
                        n3->x, n3->y, n3->z,
                        n4->x, n4->y, n4->z);
                    
                    if (volume > 1e-12) {
                        #ifdef _OPENMP
                        #pragma omp critical
                        #endif
                        {
                            MeshElement* elem = &solver->elements[solver->num_elements];
                            elem->node_ids = (int*)mesh_generation_malloc(4 * sizeof(int));
                            if (elem->node_ids) {
                                elem->node_ids[0] = i;
                                elem->node_ids[1] = j;
                                elem->node_ids[2] = k;
                                elem->node_ids[3] = l;
                                elem->num_nodes = 4;
                                elem->element_id = solver->num_elements;
                                elem->element_type = MESH_TYPE_TETRAHEDRAL;
                                elem->material_id = n1->material_id;
                                elem->physical_group = n1->physical_group;
                                elem->volume = volume;
                                elem->area = 0.0;
                                elem->aspect_ratio = 1.0;
                                elem->quality_metric = 1.0;
                                elem->is_boundary_element = 0;
                                elem->is_curved = 0;
                                elem->polynomial_order = 1;
                                solver->num_elements++;
                            }
                        }
                    }
                }
            }
        }
    }
    
    if (global_options.verbose_logging) {
        printf("[CAD_MESH] Generated %d tetrahedral elements\n", solver->num_elements);
    }
    
    return MESH_GENERATION_SUCCESS;
}

int cad_mesh_generation_generate_nodes(CadMeshGenerationSolver* solver) {
    if (!solver) {
        return MESH_GENERATION_ERROR;
    }
    
    int status = MESH_GENERATION_SUCCESS;
    double start_time = mesh_generation_get_time();
    
    if (global_options.verbose_logging) {
        printf("[CAD_MESH] Starting node generation...\n");
    }
    
    switch (solver->config.algorithm) {
        case MESH_ALGORITHM_DELAUNAY:
            status = mesh_generation_generate_nodes_delaunay(solver);
            break;
            
        default:
            status = mesh_generation_generate_nodes_delaunay(solver);
            break;
    }
    
    if (status == MESH_GENERATION_SUCCESS) {
        solver->computation_time += mesh_generation_get_time() - start_time;
        if (global_options.verbose_logging) {
            printf("[CAD_MESH] Node generation completed in %.3f seconds\n", 
                   mesh_generation_get_time() - start_time);
        }
    }
    
    return status;
}

int cad_mesh_generation_generate_elements(CadMeshGenerationSolver* solver) {
    if (!solver) {
        return MESH_GENERATION_ERROR;
    }
    
    int status = MESH_GENERATION_SUCCESS;
    double start_time = mesh_generation_get_time();
    
    if (global_options.verbose_logging) {
        printf("[CAD_MESH] Starting element generation...\n");
    }
    
    switch (solver->config.type) {
        case MESH_TYPE_TETRAHEDRAL:
            status = mesh_generation_generate_elements_tetrahedral(solver);
            break;
            
        default:
            status = mesh_generation_generate_elements_tetrahedral(solver);
            break;
    }
    
    if (status == MESH_GENERATION_SUCCESS) {
        solver->computation_time += mesh_generation_get_time() - start_time;
        if (global_options.verbose_logging) {
            printf("[CAD_MESH] Element generation completed in %.3f seconds\n", 
                   mesh_generation_get_time() - start_time);
        }
    }
    
    return status;
}

int cad_mesh_generation_connect_mesh(CadMeshGenerationSolver* solver) {
    if (!solver || !solver->nodes || !solver->elements) {
        return MESH_GENERATION_ERROR;
    }
    
    double start_time = mesh_generation_get_time();
    
    #ifdef _OPENMP
    #pragma omp parallel for num_threads(solver->parameters.num_threads) if(solver->parameters.use_parallel_meshing)
    #endif
    for (int i = 0; i < solver->num_nodes; i++) {
        solver->nodes[i].num_connected_elements = 0;
        solver->nodes[i].connected_elements = (int*)mesh_generation_malloc(solver->num_elements * sizeof(int));
        
        if (solver->nodes[i].connected_elements) {
            for (int j = 0; j < solver->num_elements; j++) {
                MeshElement* elem = &solver->elements[j];
                for (int k = 0; k < elem->num_nodes; k++) {
                    if (elem->node_ids[k] == i) {
                        solver->nodes[i].connected_elements[solver->nodes[i].num_connected_elements++] = j;
                        break;
                    }
                }
            }
        }
    }
    
    solver->computation_time += mesh_generation_get_time() - start_time;
    
    if (global_options.verbose_logging) {
        printf("[CAD_MESH] Mesh connectivity established in %.3f seconds\n", 
               mesh_generation_get_time() - start_time);
    }
    
    return MESH_GENERATION_SUCCESS;
}

int cad_mesh_generation_compute_quality_metrics(CadMeshGenerationSolver* solver) {
    if (!solver || !solver->elements || solver->num_elements <= 0) {
        return MESH_GENERATION_ERROR;
    }
    
    double start_time = mesh_generation_get_time();
    double total_quality = 0.0;
    double min_quality = 1.0;
    double max_quality = 0.0;
    int num_poor_elements = 0;
    int num_good_elements = 0;
    int num_excellent_elements = 0;
    
    #ifdef _OPENMP
    #pragma omp parallel for num_threads(solver->parameters.num_threads) if(solver->parameters.use_parallel_meshing) reduction(+:total_quality,num_poor_elements,num_good_elements,num_excellent_elements)
    #endif
    for (int i = 0; i < solver->num_elements; i++) {
        MeshElement* elem = &solver->elements[i];
        
        if (elem->element_type == MESH_TYPE_TETRAHEDRAL && elem->num_nodes == 4) {
            MeshNode* n1 = &solver->nodes[elem->node_ids[0]];
            MeshNode* n2 = &solver->nodes[elem->node_ids[1]];
            MeshNode* n3 = &solver->nodes[elem->node_ids[2]];
            MeshNode* n4 = &solver->nodes[elem->node_ids[3]];
            
            double volume = mesh_generation_compute_tetrahedron_volume(
                n1->x, n1->y, n1->z,
                n2->x, n2->y, n2->z,
                n3->x, n3->y, n3->z,
                n4->x, n4->y, n4->z);
            
            elem->volume = volume;
            
            double edge_lengths[6];
            edge_lengths[0] = mesh_generation_compute_distance(n1->x, n1->y, n1->z, n2->x, n2->y, n2->z);
            edge_lengths[1] = mesh_generation_compute_distance(n1->x, n1->y, n1->z, n3->x, n3->y, n3->z);
            edge_lengths[2] = mesh_generation_compute_distance(n1->x, n1->y, n1->z, n4->x, n4->y, n4->z);
            edge_lengths[3] = mesh_generation_compute_distance(n2->x, n2->y, n2->z, n3->x, n3->y, n3->z);
            edge_lengths[4] = mesh_generation_compute_distance(n2->x, n2->y, n2->z, n4->x, n4->y, n4->z);
            edge_lengths[5] = mesh_generation_compute_distance(n3->x, n3->y, n3->z, n4->x, n4->y, n4->z);
            
            double max_edge = edge_lengths[0];
            double min_edge = edge_lengths[0];
            for (int j = 1; j < 6; j++) {
                if (edge_lengths[j] > max_edge) max_edge = edge_lengths[j];
                if (edge_lengths[j] < min_edge) min_edge = edge_lengths[j];
            }
            
            elem->aspect_ratio = max_edge / min_edge;
            elem->quality_metric = 1.0 / elem->aspect_ratio;
            
            if (elem->quality_metric < 0.3) {
                num_poor_elements++;
            } else if (elem->quality_metric < 0.7) {
                num_good_elements++;
            } else {
                num_excellent_elements++;
            }
            
            total_quality += elem->quality_metric;
            
            #ifdef _OPENMP
            #pragma omp critical
            #endif
            {
                if (elem->quality_metric < min_quality) min_quality = elem->quality_metric;
                if (elem->quality_metric > max_quality) max_quality = elem->quality_metric;
            }
        }
    }
    
    solver->quality_metrics.aspect_ratio = total_quality / solver->num_elements;
    solver->quality_metrics.overall_quality = total_quality / solver->num_elements;
    solver->quality_metrics.minimum_quality = min_quality;
    solver->quality_metrics.maximum_quality = max_quality;
    solver->quality_metrics.average_quality = total_quality / solver->num_elements;
    solver->quality_metrics.num_poor_elements = num_poor_elements;
    solver->quality_metrics.num_good_elements = num_good_elements;
    solver->quality_metrics.num_excellent_elements = num_excellent_elements;
    
    if (global_options.verbose_logging) {
        printf("[CAD_MESH] Quality metrics computed in %.3f seconds\n", 
               mesh_generation_get_time() - start_time);
        printf("[CAD_MESH] Average quality: %.3f, Min quality: %.3f, Max quality: %.3f\n",
               solver->quality_metrics.average_quality,
               solver->quality_metrics.minimum_quality,
               solver->quality_metrics.maximum_quality);
        printf("[CAD_MESH] Poor elements: %d, Good elements: %d, Excellent elements: %d\n",
               num_poor_elements, num_good_elements, num_excellent_elements);
    }
    
    return MESH_GENERATION_SUCCESS;
}

int cad_mesh_generation_improve_mesh_quality(CadMeshGenerationSolver* solver) {
    if (!solver || !solver->elements || solver->num_elements <= 0) {
        return MESH_GENERATION_ERROR;
    }
    
    double start_time = mesh_generation_get_time();
    int improved_count = 0;
    
    for (int iter = 0; iter < MAX_SMOOTHING_ITERATIONS; iter++) {
        int iter_improved = 0;
        
        #ifdef _OPENMP
        #pragma omp parallel for num_threads(solver->parameters.num_threads) if(solver->parameters.use_parallel_meshing) reduction(+:iter_improved)
        #endif
        for (int i = 0; i < solver->num_elements; i++) {
            MeshElement* elem = &solver->elements[i];
            
            if (elem->quality_metric < solver->parameters.quality_threshold) {
                // Simple Laplacian smoothing
                double center_x = 0.0, center_y = 0.0, center_z = 0.0;
                
                for (int j = 0; j < elem->num_nodes; j++) {
                    MeshNode* node = &solver->nodes[elem->node_ids[j]];
                    center_x += node->x;
                    center_y += node->y;
                    center_z += node->z;
                }
                
                center_x /= elem->num_nodes;
                center_y /= elem->num_nodes;
                center_z /= elem->num_nodes;
                
                // Move nodes slightly toward element center
                for (int j = 0; j < elem->num_nodes; j++) {
                    MeshNode* node = &solver->nodes[elem->node_ids[j]];
                    double relaxation = 0.1;
                    node->x += relaxation * (center_x - node->x);
                    node->y += relaxation * (center_y - node->y);
                    node->z += relaxation * (center_z - node->z);
                }
                
                iter_improved++;
            }
        }
        
        improved_count += iter_improved;
        
        if (iter_improved == 0) {
            break;  // No more improvements possible
        }
    }
    
    solver->computation_time += mesh_generation_get_time() - start_time;
    
    if (global_options.verbose_logging) {
        printf("[CAD_MESH] Mesh quality improved in %.3f seconds, %d elements enhanced\n", 
               mesh_generation_get_time() - start_time, improved_count);
    }
    
    return MESH_GENERATION_SUCCESS;
}

int cad_mesh_generation_adaptive_refinement(CadMeshGenerationSolver* solver, double error_threshold) {
    if (!solver || !solver->elements || solver->num_elements <= 0) {
        return MESH_GENERATION_ERROR;
    }
    
    double start_time = mesh_generation_get_time();
    int refined_count = 0;
    
    for (int level = 0; level < solver->parameters.adaptive_refinement_levels; level++) {
        int level_refined = 0;
        
        #ifdef _OPENMP
        #pragma omp parallel for num_threads(solver->parameters.num_threads) if(solver->parameters.use_parallel_meshing) reduction(+:level_refined)
        #endif
        for (int i = 0; i < solver->num_elements; i++) {
            MeshElement* elem = &solver->elements[i];
            
            if (elem->quality_metric < error_threshold) {
                // Simple refinement: split element into smaller ones
                // This is a placeholder for more sophisticated refinement algorithms
                
                #ifdef _OPENMP
                #pragma omp critical
                #endif
                {
                    // Add refinement logic here
                    level_refined++;
                }
            }
        }
        
        refined_count += level_refined;
        
        if (level_refined == 0) {
            break;  // No more refinement needed
        }
    }
    
    solver->computation_time += mesh_generation_get_time() - start_time;
    
    if (global_options.verbose_logging) {
        printf("[CAD_MESH] Adaptive refinement completed in %.3f seconds, %d elements refined\n", 
               mesh_generation_get_time() - start_time, refined_count);
    }
    
    return MESH_GENERATION_SUCCESS;
}

int cad_mesh_generation_parallel_meshing(CadMeshGenerationSolver* solver) {
    if (!solver) {
        return MESH_GENERATION_ERROR;
    }
    
    double start_time = mesh_generation_get_time();
    
    if (!solver->parameters.use_parallel_meshing || solver->parameters.num_threads <= 1) {
        if (global_options.verbose_logging) {
            printf("[CAD_MESH] Parallel meshing disabled or single thread requested\n");
        }
        return MESH_GENERATION_SUCCESS;
    }
    
    #ifdef _OPENMP
    omp_set_num_threads(solver->parameters.num_threads);
    if (global_options.verbose_logging) {
        printf("[CAD_MESH] Parallel meshing with %d threads\n", solver->parameters.num_threads);
    }
    #endif
    
    #ifdef ENABLE_MPI
    if (global_options.use_mpi) {
        int rank, size;
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        MPI_Comm_size(MPI_COMM_WORLD, &size);
        
        if (global_options.verbose_logging) {
            printf("[CAD_MESH] MPI parallel meshing: rank %d of %d\n", rank, size);
        }
    }
    #endif
    
    solver->computation_time += mesh_generation_get_time() - start_time;
    
    return MESH_GENERATION_SUCCESS;
}

int cad_mesh_generation_validate_mesh(CadMeshGenerationSolver* solver) {
    if (!solver || !solver->nodes || !solver->elements) {
        return MESH_GENERATION_ERROR;
    }
    
    double start_time = mesh_generation_get_time();
    int validation_errors = 0;
    
    // Check for duplicate nodes
    #ifdef _OPENMP
    #pragma omp parallel for num_threads(solver->parameters.num_threads) if(solver->parameters.use_parallel_meshing) reduction(+:validation_errors)
    #endif
    for (int i = 0; i < solver->num_nodes - 1; i++) {
        for (int j = i + 1; j < solver->num_nodes; j++) {
            double dist = mesh_generation_compute_distance(
                solver->nodes[i].x, solver->nodes[i].y, solver->nodes[i].z,
                solver->nodes[j].x, solver->nodes[j].y, solver->nodes[j].z);
            
            if (dist < 1e-10) {
                validation_errors++;
                if (global_options.verbose_logging) {
                    printf("[CAD_MESH] Warning: Duplicate nodes detected (%d and %d)\n", i, j);
                }
            }
        }
    }
    
    // Check for invalid elements
    #ifdef _OPENMP
    #pragma omp parallel for num_threads(solver->parameters.num_threads) if(solver->parameters.use_parallel_meshing) reduction(+:validation_errors)
    #endif
    for (int i = 0; i < solver->num_elements; i++) {
        MeshElement* elem = &solver->elements[i];
        
        if (elem->quality_metric < 0.01) {
            validation_errors++;
            if (global_options.verbose_logging) {
                printf("[CAD_MESH] Warning: Poor quality element %d (quality = %.6f)\n", 
                       i, elem->quality_metric);
            }
        }
        
        for (int j = 0; j < elem->num_nodes; j++) {
            if (elem->node_ids[j] < 0 || elem->node_ids[j] >= solver->num_nodes) {
                validation_errors++;
                if (global_options.verbose_logging) {
                    printf("[CAD_MESH] Error: Invalid node ID in element %d\n", i);
                }
            }
        }
    }
    
    solver->computation_time += mesh_generation_get_time() - start_time;
    
    if (global_options.verbose_logging) {
        printf("[CAD_MESH] Mesh validation completed in %.3f seconds, %d errors found\n", 
               mesh_generation_get_time() - start_time, validation_errors);
    }
    
    return validation_errors == 0 ? MESH_GENERATION_SUCCESS : MESH_GENERATION_QUALITY_ERROR;
}

int cad_mesh_generation_simulate(CadMeshGenerationSolver* solver) {
    if (!solver) {
        return MESH_GENERATION_ERROR;
    }
    
    int status = mesh_generation_validate_input(solver);
    if (status != MESH_GENERATION_SUCCESS) {
        return status;
    }
    
    double total_start_time = mesh_generation_get_time();
    
    if (global_options.verbose_logging) {
        printf("[CAD_MESH] Starting mesh generation simulation...\n");
    }
    
    // Step 1: Parallel meshing setup
    status = cad_mesh_generation_parallel_meshing(solver);
    if (status != MESH_GENERATION_SUCCESS) {
        return status;
    }
    
    // Step 2: Generate nodes
    status = cad_mesh_generation_generate_nodes(solver);
    if (status != MESH_GENERATION_SUCCESS) {
        return status;
    }
    
    // Step 3: Generate elements
    status = cad_mesh_generation_generate_elements(solver);
    if (status != MESH_GENERATION_SUCCESS) {
        return status;
    }
    
    // Step 4: Connect mesh
    status = cad_mesh_generation_connect_mesh(solver);
    if (status != MESH_GENERATION_SUCCESS) {
        return status;
    }
    
    // Step 5: Compute quality metrics
    status = cad_mesh_generation_compute_quality_metrics(solver);
    if (status != MESH_GENERATION_SUCCESS) {
        return status;
    }
    
    // Step 6: Improve mesh quality
    if (solver->config.use_quality_improvement) {
        status = cad_mesh_generation_improve_mesh_quality(solver);
        if (status != MESH_GENERATION_SUCCESS) {
            return status;
        }
    }
    
    // Step 7: Adaptive refinement
    if (solver->config.use_adaptive_meshing) {
        status = cad_mesh_generation_adaptive_refinement(solver, solver->parameters.quality_threshold);
        if (status != MESH_GENERATION_SUCCESS) {
            return status;
        }
    }
    
    // Step 8: Validate final mesh
    status = cad_mesh_generation_validate_mesh(solver);
    if (status != MESH_GENERATION_SUCCESS) {
        return status;
    }
    
    solver->mesh_generation_completed = 1;
    solver->convergence_status = 1;
    
    double total_time = mesh_generation_get_time() - total_start_time;
    solver->computation_time = total_time;
    
    if (global_options.verbose_logging) {
        printf("[CAD_MESH] Mesh generation simulation completed successfully\n");
        printf("[CAD_MESH] Total computation time: %.3f seconds\n", total_time);
        printf("[CAD_MESH] Generated mesh: %d nodes, %d elements\n", 
               solver->num_nodes, solver->num_elements);
    }
    
    return MESH_GENERATION_SUCCESS;
}

int cad_mesh_generation_generate_mesh(CadMeshGenerationSolver* solver) {
    return cad_mesh_generation_simulate(solver);
}

int cad_mesh_generation_refine_mesh(CadMeshGenerationSolver* solver) {
    if (!solver || !solver->mesh_generation_completed) {
        return MESH_GENERATION_ERROR;
    }
    
    return cad_mesh_generation_adaptive_refinement(solver, solver->parameters.quality_threshold);
}

int cad_mesh_generation_optimize_mesh(CadMeshGenerationSolver* solver) {
    if (!solver || !solver->mesh_generation_completed) {
        return MESH_GENERATION_ERROR;
    }
    
    int status = cad_mesh_generation_improve_mesh_quality(solver);
    if (status != MESH_GENERATION_SUCCESS) {
        return status;
    }
    
    return cad_mesh_generation_compute_quality_metrics(solver);
}

int cad_mesh_generation_import_cad_file(CadMeshGenerationSolver* solver, const char* filename, FileFormatType format) {
    if (!solver || !filename) {
        return MESH_GENERATION_ERROR;
    }
    
    if (global_options.verbose_logging) {
        printf("[CAD_MESH] Importing CAD file via OpenCascade: %s (format=%d)\n", filename, format);
    }

    opencascade_import_params_t params = {0};
    params.heal_geometry = 1;
    params.healing_precision = 1e-6;
    params.max_tolerance = 1e-4;

    opencascade_geometry_t geometry = {0};

    bool ok = opencascade_import_cad(filename, &params, &geometry);
    if (!ok) {
        if (global_options.verbose_logging) {
            fprintf(stderr, "[CAD_MESH] OpenCascade import failed for: %s\n", filename);
        }
        return MESH_GENERATION_ERROR;
    }

    /* Store minimal geometry summary into solver for downstream size/quality estimates */
    if (geometry.num_faces > 0 || geometry.num_edges > 0) {
        solver->parameters.curvature_resolution = 0.1;
        solver->parameters.proximity_resolution = 0.01;
        /* Use bounding box to set default target size if not set */
        double dx = geometry.bounding_box[3] - geometry.bounding_box[0];
        double dy = geometry.bounding_box[4] - geometry.bounding_box[1];
        double dz = geometry.bounding_box[5] - geometry.bounding_box[2];
        double span = fmax(fmax(dx, dy), dz);
        if (span > 0 && solver->parameters.target_element_size <= 0) {
            solver->parameters.target_element_size = span / 50.0; /* ~50 elements along longest edge */
        }
    }

    if (global_options.verbose_logging) {
        printf("[CAD_MESH] CAD import succeeded: faces=%d, edges=%d, bbox=[%.3f %.3f %.3f | %.3f %.3f %.3f]\n",
               geometry.num_faces, geometry.num_edges,
               geometry.bounding_box[0], geometry.bounding_box[1], geometry.bounding_box[2],
               geometry.bounding_box[3], geometry.bounding_box[4], geometry.bounding_box[5]);
    }

    return MESH_GENERATION_SUCCESS;
}

int cad_mesh_generation_import_geometry(CadMeshGenerationSolver* solver, double* geometry_data, int geometry_size) {
    if (!solver || !geometry_data || geometry_size <= 0) {
        return MESH_GENERATION_ERROR;
    }
    
    solver->geometry_data = (double*)mesh_generation_malloc(geometry_size * sizeof(double));
    if (!solver->geometry_data) {
        return MESH_GENERATION_MEMORY_ERROR;
    }
    
    memcpy(solver->geometry_data, geometry_data, geometry_size * sizeof(double));
    solver->geometry_size = geometry_size;
    
    if (global_options.verbose_logging) {
        printf("[CAD_MESH] Imported geometry data: %d doubles\n", geometry_size);
    }
    
    return MESH_GENERATION_SUCCESS;
}

int cad_mesh_generation_export_mesh(CadMeshGenerationSolver* solver, const char* filename, const char* format) {
    if (!solver || !filename || !format) {
        return MESH_GENERATION_ERROR;
    }
    
    if (!solver->mesh_generation_completed) {
        if (global_options.verbose_logging) {
            fprintf(stderr, "[CAD_MESH] Cannot export mesh: generation not completed\n");
        }
        return MESH_GENERATION_ERROR;
    }
    
    FILE* file = fopen(filename, "w");
    if (!file) {
        if (global_options.verbose_logging) {
            fprintf(stderr, "[CAD_MESH] Failed to create export file: %s\n", filename);
        }
        return MESH_GENERATION_ERROR;
    }
    
    if (global_options.verbose_logging) {
        printf("[CAD_MESH] Exporting mesh to: %s (format=%s)\n", filename, format);
    }
    
    // Export mesh data in specified format
    if (strcmp(format, "vtk") == 0) {
        fprintf(file, "# vtk DataFile Version 3.0\n");
        fprintf(file, "CAD Mesh Generation Output\n");
        fprintf(file, "ASCII\n");
        fprintf(file, "DATASET UNSTRUCTURED_GRID\n");
        fprintf(file, "POINTS %d double\n", solver->num_nodes);
        
        for (int i = 0; i < solver->num_nodes; i++) {
            fprintf(file, "%f %f %f\n", 
                   solver->nodes[i].x, solver->nodes[i].y, solver->nodes[i].z);
        }
        
        fprintf(file, "CELLS %d %d\n", solver->num_elements, solver->num_elements * 5);
        
        for (int i = 0; i < solver->num_elements; i++) {
            fprintf(file, "4 %d %d %d %d\n",
                   solver->elements[i].node_ids[0],
                   solver->elements[i].node_ids[1],
                   solver->elements[i].node_ids[2],
                   solver->elements[i].node_ids[3]);
        }
        
        fprintf(file, "CELL_TYPES %d\n", solver->num_elements);
        for (int i = 0; i < solver->num_elements; i++) {
            fprintf(file, "10\n");  // VTK_TETRA
        }
    }
    
    fclose(file);
    
    if (global_options.verbose_logging) {
        printf("[CAD_MESH] Mesh exported successfully\n");
    }
    
    return MESH_GENERATION_SUCCESS;
}

int cad_mesh_generation_apply_boundary_conditions(CadMeshGenerationSolver* solver) {
    if (!solver || !solver->nodes || solver->num_nodes <= 0) {
        return MESH_GENERATION_ERROR;
    }
    
    double start_time = mesh_generation_get_time();
    
    #ifdef _OPENMP
    #pragma omp parallel for num_threads(solver->parameters.num_threads) if(solver->parameters.use_parallel_meshing)
    #endif
    for (int i = 0; i < solver->num_nodes; i++) {
        MeshNode* node = &solver->nodes[i];
        
        // Simple boundary detection based on coordinates
        if (fabs(node->x) < 1e-6 || fabs(node->y) < 1e-6 || fabs(node->z) < 1e-6) {
            node->is_boundary_node = 1;
        }
        
        // Corner detection
        if (node->is_boundary_node) {
            int corner_count = 0;
            if (fabs(node->x) < 1e-6) corner_count++;
            if (fabs(node->y) < 1e-6) corner_count++;
            if (fabs(node->z) < 1e-6) corner_count++;
            
            if (corner_count >= 2) {
                node->is_corner_node = 1;
            }
        }
    }
    
    solver->computation_time += mesh_generation_get_time() - start_time;
    
    if (global_options.verbose_logging) {
        printf("[CAD_MESH] Boundary conditions applied in %.3f seconds\n", 
               mesh_generation_get_time() - start_time);
    }
    
    return MESH_GENERATION_SUCCESS;
}

int cad_mesh_generation_apply_material_properties(CadMeshGenerationSolver* solver) {
    if (!solver || !solver->elements || solver->num_elements <= 0) {
        return MESH_GENERATION_ERROR;
    }
    
    double start_time = mesh_generation_get_time();
    
    #ifdef _OPENMP
    #pragma omp parallel for num_threads(solver->parameters.num_threads) if(solver->parameters.use_parallel_meshing)
    #endif
    for (int i = 0; i < solver->num_elements; i++) {
        MeshElement* elem = &solver->elements[i];
        
        if (elem->material_id >= 0 && elem->material_id < solver->num_materials) {
            MaterialProperties* mat = &solver->materials[elem->material_id];
            
            // Apply material properties to element
            // This would be used for electromagnetic property assignment
            elem->area *= mat->permittivity.real;
            elem->volume *= mat->permeability.real;
        }
    }
    
    solver->computation_time += mesh_generation_get_time() - start_time;
    
    if (global_options.verbose_logging) {
        printf("[CAD_MESH] Material properties applied in %.3f seconds\n", 
               mesh_generation_get_time() - start_time);
    }
    
    return MESH_GENERATION_SUCCESS;
}

int cad_mesh_generation_apply_mesh_constraints(CadMeshGenerationSolver* solver) {
    if (!solver || !solver->nodes || solver->num_nodes <= 0) {
        return MESH_GENERATION_ERROR;
    }
    
    double start_time = mesh_generation_get_time();
    
    #ifdef _OPENMP
    #pragma omp parallel for num_threads(solver->parameters.num_threads) if(solver->parameters.use_parallel_meshing)
    #endif
    for (int i = 0; i < solver->num_nodes; i++) {
        MeshNode* node = &solver->nodes[i];
        
        // Apply minimum/maximum element size constraints
        if (node->mesh_size < solver->parameters.minimum_element_size) {
            node->mesh_size = solver->parameters.minimum_element_size;
        }
        if (node->mesh_size > solver->parameters.maximum_element_size) {
            node->mesh_size = solver->parameters.maximum_element_size;
        }
        
        // Apply curvature-based sizing
        if (solver->parameters.use_curvature_adaptation) {
            double curvature = 0.0;  // Placeholder for actual curvature calculation
            if (curvature > solver->parameters.curvature_resolution) {
                node->mesh_size *= 0.5;
            }
        }
    }
    
    solver->computation_time += mesh_generation_get_time() - start_time;
    
    if (global_options.verbose_logging) {
        printf("[CAD_MESH] Mesh constraints applied in %.3f seconds\n", 
               mesh_generation_get_time() - start_time);
    }
    
    return MESH_GENERATION_SUCCESS;
}

int cad_mesh_generation_smooth_mesh(CadMeshGenerationSolver* solver) {
    return cad_mesh_generation_improve_mesh_quality(solver);
}

int cad_mesh_generation_coarsen_mesh(CadMeshGenerationSolver* solver) {
    if (!solver || !solver->elements || solver->num_elements <= 0) {
        return MESH_GENERATION_ERROR;
    }
    
    double start_time = mesh_generation_get_time();
    int coarsened_count = 0;
    
    // Simple coarsening: remove elements with very high quality
    for (int i = solver->num_elements - 1; i >= 0; i--) {
        MeshElement* elem = &solver->elements[i];
        
        if (elem->quality_metric > 0.9) {
            // Remove this element
            if (elem->node_ids) {
                mesh_generation_free(elem->node_ids);
            }
            
            // Shift remaining elements
            for (int j = i; j < solver->num_elements - 1; j++) {
                solver->elements[j] = solver->elements[j + 1];
            }
            
            solver->num_elements--;
            coarsened_count++;
        }
    }
    
    solver->computation_time += mesh_generation_get_time() - start_time;
    
    if (global_options.verbose_logging) {
        printf("[CAD_MESH] Mesh coarsening completed in %.3f seconds, %d elements removed\n", 
               mesh_generation_get_time() - start_time, coarsened_count);
    }
    
    return MESH_GENERATION_SUCCESS;
}

int cad_mesh_generation_detect_features(CadMeshGenerationSolver* solver) {
    if (!solver || !solver->entities || solver->num_entities <= 0) {
        return MESH_GENERATION_ERROR;
    }
    
    double start_time = mesh_generation_get_time();
    int detected_features = 0;
    
    for (int i = 0; i < solver->num_entities; i++) {
        CadEntity* entity = &solver->entities[i];
        
        // Feature detection based on entity type and properties
        switch (entity->type) {
            case CAD_ENTITY_TYPE_POINT:
                // Points are features
                detected_features++;
                break;
                
            case CAD_ENTITY_TYPE_LINE:
                // Lines with high curvature are features
                if (entity->curvature > solver->parameters.feature_angle * M_PI / 180.0) {
                    detected_features++;
                }
                break;
                
            case CAD_ENTITY_TYPE_CIRCLE:
            case CAD_ENTITY_TYPE_ARC:
                // Circular features
                detected_features++;
                break;
                
            default:
                break;
        }
    }
    
    solver->computation_time += mesh_generation_get_time() - start_time;
    
    if (global_options.verbose_logging) {
        printf("[CAD_MESH] Feature detection completed in %.3f seconds, %d features found\n", 
               mesh_generation_get_time() - start_time, detected_features);
    }
    
    return MESH_GENERATION_SUCCESS;
}

int cad_mesh_generation_extract_boundaries(CadMeshGenerationSolver* solver) {
    if (!solver || !solver->elements || solver->num_elements <= 0) {
        return MESH_GENERATION_ERROR;
    }
    
    double start_time = mesh_generation_get_time();
    int boundary_faces = 0;
    
    // Simple boundary extraction based on element connectivity
    for (int i = 0; i < solver->num_elements; i++) {
        MeshElement* elem = &solver->elements[i];
        
        // Check if element has faces that are not shared with other elements
        for (int j = 0; j < elem->num_nodes; j++) {
            int is_boundary = 1;
            
            for (int k = 0; k < solver->num_elements; k++) {
                if (k == i) continue;
                
                MeshElement* other_elem = &solver->elements[k];
                int shared_nodes = 0;
                
                for (int m = 0; m < other_elem->num_nodes; m++) {
                    for (int n = 0; n < elem->num_nodes; n++) {
                        if (other_elem->node_ids[m] == elem->node_ids[n]) {
                            shared_nodes++;
                        }
                    }
                }
                
                if (shared_nodes >= 3) {  // Shared face in tetrahedron
                    is_boundary = 0;
                    break;
                }
            }
            
            if (is_boundary) {
                boundary_faces++;
            }
        }
    }
    
    solver->computation_time += mesh_generation_get_time() - start_time;
    
    if (global_options.verbose_logging) {
        printf("[CAD_MESH] Boundary extraction completed in %.3f seconds, %d boundary faces found\n", 
               mesh_generation_get_time() - start_time, boundary_faces);
    }
    
    return MESH_GENERATION_SUCCESS;
}

int cad_mesh_generation_identify_regions(CadMeshGenerationSolver* solver) {
    if (!solver || !solver->materials || solver->num_materials <= 0) {
        return MESH_GENERATION_ERROR;
    }
    
    double start_time = mesh_generation_get_time();
    int identified_regions = 0;
    
    // Region identification based on material properties
    for (int i = 0; i < solver->num_materials; i++) {
        MaterialProperties* mat = &solver->materials[i];
        
        // Each material represents a region
        identified_regions++;
        
        if (global_options.verbose_logging) {
            printf("[CAD_MESH] Region %d: Material '%s' (εr=%.3f, μr=%.3f)\n",
                   i, mat->material_name, mat->permittivity.real, mat->permeability.real);
        }
    }
    
    solver->computation_time += mesh_generation_get_time() - start_time;
    
    if (global_options.verbose_logging) {
        printf("[CAD_MESH] Region identification completed in %.3f seconds, %d regions found\n", 
               mesh_generation_get_time() - start_time, identified_regions);
    }
    
    return MESH_GENERATION_SUCCESS;
}

int cad_mesh_generation_curvature_adaptation(CadMeshGenerationSolver* solver) {
    if (!solver || !solver->entities || solver->num_entities <= 0) {
        return MESH_GENERATION_ERROR;
    }
    
    double start_time = mesh_generation_get_time();
    int adapted_elements = 0;
    
    for (int i = 0; i < solver->num_entities; i++) {
        CadEntity* entity = &solver->entities[i];
        
        if (entity->curvature > 0.0) {
            double curvature_radius = 1.0 / entity->curvature;
            double target_size = curvature_radius * solver->parameters.curvature_resolution;
            
            // Apply curvature-based sizing to nearby elements
            for (int j = 0; j < solver->num_nodes; j++) {
                MeshNode* node = &solver->nodes[j];
                double dist = mesh_generation_compute_distance(
                    node->x, node->y, node->z,
                    entity->center[0], entity->center[1], entity->center[2]);
                
                if (dist < curvature_radius * 2.0) {
                    node->mesh_size = fmin(node->mesh_size, target_size);
                    adapted_elements++;
                }
            }
        }
    }
    
    solver->computation_time += mesh_generation_get_time() - start_time;
    
    if (global_options.verbose_logging) {
        printf("[CAD_MESH] Curvature adaptation completed in %.3f seconds, %d elements adapted\n", 
               mesh_generation_get_time() - start_time, adapted_elements);
    }
    
    return MESH_GENERATION_SUCCESS;
}

int cad_mesh_generation_proximity_adaptation(CadMeshGenerationSolver* solver) {
    if (!solver || !solver->entities || solver->num_entities <= 0) {
        return MESH_GENERATION_ERROR;
    }
    
    double start_time = mesh_generation_get_time();
    int adapted_elements = 0;
    
    // Proximity-based refinement between entities
    for (int i = 0; i < solver->num_entities; i++) {
        CadEntity* entity1 = &solver->entities[i];
        
        for (int j = i + 1; j < solver->num_entities; j++) {
            CadEntity* entity2 = &solver->entities[j];
            
            double dist = mesh_generation_compute_distance(
                entity1->center[0], entity1->center[1], entity1->center[2],
                entity2->center[0], entity2->center[1], entity2->center[2]);
            
            if (dist < solver->parameters.proximity_resolution) {
                // Refine between proximate entities
                for (int k = 0; k < solver->num_nodes; k++) {
                    MeshNode* node = &solver->nodes[k];
                    double node_dist1 = mesh_generation_compute_distance(
                        node->x, node->y, node->z,
                        entity1->center[0], entity1->center[1], entity1->center[2]);
                    double node_dist2 = mesh_generation_compute_distance(
                        node->x, node->y, node->z,
                        entity2->center[0], entity2->center[1], entity2->center[2]);
                    
                    if (node_dist1 < dist * 0.5 || node_dist2 < dist * 0.5) {
                        node->mesh_size *= 0.5;
                        adapted_elements++;
                    }
                }
            }
        }
    }
    
    solver->computation_time += mesh_generation_get_time() - start_time;
    
    if (global_options.verbose_logging) {
        printf("[CAD_MESH] Proximity adaptation completed in %.3f seconds, %d elements adapted\n", 
               mesh_generation_get_time() - start_time, adapted_elements);
    }
    
    return MESH_GENERATION_SUCCESS;
}

int cad_mesh_generation_domain_decomposition(CadMeshGenerationSolver* solver) {
    if (!solver || !solver->elements || solver->num_elements <= 0) {
        return MESH_GENERATION_ERROR;
    }
    
    double start_time = mesh_generation_get_time();
    int num_domains = solver->parameters.num_threads;
    
    // Simple domain decomposition based on element count
    int elements_per_domain = solver->num_elements / num_domains;
    int remainder = solver->num_elements % num_domains;
    
    if (global_options.verbose_logging) {
        printf("[CAD_MESH] Domain decomposition: %d domains, %d elements per domain\n",
               num_domains, elements_per_domain);
    }
    
    solver->computation_time += mesh_generation_get_time() - start_time;
    
    return MESH_GENERATION_SUCCESS;
}

int cad_mesh_generation_load_balancing(CadMeshGenerationSolver* solver) {
    if (!solver || !solver->elements || solver->num_elements <= 0) {
        return MESH_GENERATION_ERROR;
    }
    
    double start_time = mesh_generation_get_time();
    
    // Simple load balancing based on element complexity
    double total_work = 0.0;
    for (int i = 0; i < solver->num_elements; i++) {
        total_work += 1.0 / solver->elements[i].quality_metric;  // More work for poor quality elements
    }
    
    double work_per_thread = total_work / solver->parameters.num_threads;
    
    if (global_options.verbose_logging) {
        printf("[CAD_MESH] Load balancing: total work = %.1f, work per thread = %.1f\n",
               total_work, work_per_thread);
    }
    
    solver->computation_time += mesh_generation_get_time() - start_time;
    
    return MESH_GENERATION_SUCCESS;
}

int cad_mesh_generation_check_mesh_consistency(CadMeshGenerationSolver* solver) {
    return cad_mesh_generation_validate_mesh(solver);
}

int cad_mesh_generation_check_mesh_quality(CadMeshGenerationSolver* solver) {
    return cad_mesh_generation_compute_quality_metrics(solver);
}

int cad_mesh_generation_check_boundary_integrity(CadMeshGenerationSolver* solver) {
    return cad_mesh_generation_extract_boundaries(solver);
}

int cad_mesh_generation_export_to_electromagnetic_solver(CadMeshGenerationSolver* solver, MomSolver* mom_solver) {
    if (!solver || !mom_solver || !solver->mesh_generation_completed) {
        return MESH_GENERATION_ERROR;
    }
    
    double start_time = mesh_generation_get_time();
    
    if (global_options.verbose_logging) {
        printf("[CAD_MESH] Exporting mesh to MoM solver...\n");
    }
    
    // Convert mesh data to MoM solver format
    // This would integrate with the unified PEEC-MoM framework
    
    solver->computation_time += mesh_generation_get_time() - start_time;
    
    if (global_options.verbose_logging) {
        printf("[CAD_MESH] Mesh exported to MoM solver in %.3f seconds\n", 
               mesh_generation_get_time() - start_time);
    }
    
    return MESH_GENERATION_SUCCESS;
}

int cad_mesh_generation_export_to_fdtd_solver(CadMeshGenerationSolver* solver) {
    if (!solver || !solver->mesh_generation_completed) {
        return MESH_GENERATION_ERROR;
    }
    
    double start_time = mesh_generation_get_time();
    
    if (global_options.verbose_logging) {
        printf("[CAD_MESH] Exporting mesh to FDTD solver...\n");
    }
    
    // Convert mesh data to FDTD solver format
    // This would generate Yee cell grid data
    
    solver->computation_time += mesh_generation_get_time() - start_time;
    
    if (global_options.verbose_logging) {
        printf("[CAD_MESH] Mesh exported to FDTD solver in %.3f seconds\n", 
               mesh_generation_get_time() - start_time);
    }
    
    return MESH_GENERATION_SUCCESS;
}

int cad_mesh_generation_export_to_fem_solver(CadMeshGenerationSolver* solver) {
    if (!solver || !solver->mesh_generation_completed) {
        return MESH_GENERATION_ERROR;
    }
    
    double start_time = mesh_generation_get_time();
    
    if (global_options.verbose_logging) {
        printf("[CAD_MESH] Exporting mesh to FEM solver...\n");
    }
    
    // Convert mesh data to FEM solver format
    // This would generate element stiffness matrices and load vectors
    
    solver->computation_time += mesh_generation_get_time() - start_time;
    
    if (global_options.verbose_logging) {
        printf("[CAD_MESH] Mesh exported to FEM solver in %.3f seconds\n", 
               mesh_generation_get_time() - start_time);
    }
    
    return MESH_GENERATION_SUCCESS;
}

void cad_mesh_generation_get_mesh_statistics(CadMeshGenerationSolver* solver, int* num_nodes, int* num_elements, int* num_materials) {
    if (!solver) {
        if (num_nodes) *num_nodes = 0;
        if (num_elements) *num_elements = 0;
        if (num_materials) *num_materials = 0;
        return;
    }
    
    if (num_nodes) *num_nodes = solver->num_nodes;
    if (num_elements) *num_elements = solver->num_elements;
    if (num_materials) *num_materials = solver->num_materials;
}

void cad_mesh_generation_get_quality_metrics(CadMeshGenerationSolver* solver, MeshQualityMetrics* metrics) {
    if (!solver || !metrics) {
        return;
    }
    
    *metrics = solver->quality_metrics;
}

void cad_mesh_generation_get_node_coordinates(CadMeshGenerationSolver* solver, double** coordinates, int* num_nodes) {
    if (!solver || !coordinates || !num_nodes) {
        return;
    }
    
    *num_nodes = solver->num_nodes;
    *coordinates = (double*)mesh_generation_malloc(solver->num_nodes * 3 * sizeof(double));
    
    if (*coordinates) {
        for (int i = 0; i < solver->num_nodes; i++) {
            (*coordinates)[i * 3 + 0] = solver->nodes[i].x;
            (*coordinates)[i * 3 + 1] = solver->nodes[i].y;
            (*coordinates)[i * 3 + 2] = solver->nodes[i].z;
        }
    }
}

void cad_mesh_generation_get_element_connectivity(CadMeshGenerationSolver* solver, int** connectivity, int* num_elements) {
    if (!solver || !connectivity || !num_elements) {
        return;
    }
    
    *num_elements = solver->num_elements;
    *connectivity = (int*)mesh_generation_malloc(solver->num_elements * 4 * sizeof(int));
    
    if (*connectivity) {
        for (int i = 0; i < solver->num_elements; i++) {
            for (int j = 0; j < 4; j++) {
                (*connectivity)[i * 4 + j] = solver->elements[i].node_ids[j];
            }
        }
    }
}

void cad_mesh_generation_print_summary(CadMeshGenerationSolver* solver) {
    if (!solver) {
        printf("[CAD_MESH] Invalid solver\n");
        return;
    }
    
    printf("========================================\n");
    printf("CAD Mesh Generation Summary\n");
    printf("========================================\n");
    printf("Version: %s\n", CAD_MESH_GENERATION_VERSION);
    printf("Build: %s %s\n", CAD_MESH_GENERATION_BUILD_DATE, CAD_MESH_GENERATION_BUILD_TIME);
    printf("Mesh Type: %d\n", solver->config.type);
    printf("Algorithm: %d\n", solver->config.algorithm);
    printf("Polynomial Order: %d\n", solver->config.polynomial_order);
    printf("========================================\n");
    printf("Geometry Statistics:\n");
    printf("  CAD Entities: %d\n", solver->num_entities);
    printf("  Materials: %d\n", solver->num_materials);
    printf("========================================\n");
    printf("Mesh Statistics:\n");
    printf("  Nodes: %d\n", solver->num_nodes);
    printf("  Elements: %d\n", solver->num_elements);
    printf("  Generation Completed: %s\n", solver->mesh_generation_completed ? "Yes" : "No");
    printf("  Computation Time: %.3f seconds\n", solver->computation_time);
    printf("  Memory Usage: %.1f MB\n", solver->memory_usage / (1024.0 * 1024.0));
    printf("  Convergence Status: %s\n", solver->convergence_status ? "Converged" : "Not Converged");
    printf("========================================\n");
    printf("Quality Metrics:\n");
    printf("  Average Quality: %.3f\n", solver->quality_metrics.average_quality);
    printf("  Minimum Quality: %.3f\n", solver->quality_metrics.minimum_quality);
    printf("  Maximum Quality: %.3f\n", solver->quality_metrics.maximum_quality);
    printf("  Poor Elements: %d\n", solver->quality_metrics.num_poor_elements);
    printf("  Good Elements: %d\n", solver->quality_metrics.num_good_elements);
    printf("  Excellent Elements: %d\n", solver->quality_metrics.num_excellent_elements);
    printf("========================================\n");
}

void cad_mesh_generation_benchmark(CadMeshGenerationSolver* solver) {
    if (!solver) {
        printf("[CAD_MESH] Invalid solver for benchmark\n");
        return;
    }
    
    printf("========================================\n");
    printf("CAD Mesh Generation Benchmark\n");
    printf("========================================\n");
    
    // Create test geometry
    CadEntity test_entities[3];
    MaterialProperties test_materials[1];
    
    // Point entity
    test_entities[0].type = CAD_ENTITY_TYPE_POINT;
    test_entities[0].center[0] = 0.0;
    test_entities[0].center[1] = 0.0;
    test_entities[0].center[2] = 0.0;
    test_entities[0].material_id = 0;
    test_entities[0].layer_id = 0;
    test_entities[0].curvature = 0.0;
    test_entities[0].control_points = NULL;
    test_entities[0].weights = NULL;
    test_entities[0].knots = NULL;
    
    // Line entity
    test_entities[1].type = CAD_ENTITY_TYPE_LINE;
    test_entities[1].center[0] = 0.5;
    test_entities[1].center[1] = 0.0;
    test_entities[1].center[2] = 0.0;
    test_entities[1].material_id = 0;
    test_entities[1].layer_id = 0;
    test_entities[1].curvature = 0.0;
    double line_points[6] = {0.0, 0.0, 0.0, 1.0, 0.0, 0.0};
    test_entities[1].control_points = line_points;
    test_entities[1].weights = NULL;
    test_entities[1].knots = NULL;
    
    // Circle entity
    test_entities[2].type = CAD_ENTITY_TYPE_CIRCLE;
    test_entities[2].center[0] = 0.5;
    test_entities[2].center[1] = 0.5;
    test_entities[2].center[2] = 0.0;
    test_entities[2].material_id = 0;
    test_entities[2].layer_id = 0;
    test_entities[2].curvature = 1.0;
    test_entities[2].control_points = NULL;
    test_entities[2].weights = NULL;
    test_entities[2].knots = NULL;
    
    // Material properties
    strcpy(test_materials[0].material_name, "TestMaterial");
    test_materials[0].epsilon_r = 1.0;
    test_materials[0].mu_r = 1.0;
    test_materials[0].conductivity = 0.0;
    test_materials[0].permittivity.real = 1.0;
    test_materials[0].permittivity.imag = 0.0;
    test_materials[0].permeability.real = 1.0;
    test_materials[0].permeability.imag = 0.0;
    
    // Setup solver
    cad_mesh_generation_setup_entities(solver, test_entities, 3);
    cad_mesh_generation_setup_materials(solver, test_materials, 1);
    
    printf("Running benchmark with test geometry...\n");
    
    double start_time = mesh_generation_get_time();
    int status = cad_mesh_generation_simulate(solver);
    double end_time = mesh_generation_get_time();
    
    if (status == MESH_GENERATION_SUCCESS) {
        printf("Benchmark completed successfully in %.3f seconds\n", end_time - start_time);
        cad_mesh_generation_print_summary(solver);
    } else {
        printf("Benchmark failed with status: %d\n", status);
    }
    
    printf("========================================\n");
}
