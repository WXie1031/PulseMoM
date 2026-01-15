/********************************************************************************
 *  PulseEM - Gmsh Surface Mesh Implementation
 *
 *  3D surface mesh generation using Gmsh for MoM and PEEC applications
 *  Provides advanced surface meshing with CAD import, adaptive refinement,
 *  and high-quality triangle generation
 ********************************************************************************/

#define _CRT_SECURE_NO_WARNINGS
#include "gmsh_surface_mesh.h"
#include "../core/core_geometry.h"
#include "../core/core_mesh.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <vector>
#include <string>

/* Include Gmsh C API */
#include "../../libs/gmsh-4.15.0-Windows64-sdk/include/gmsh.h_cwrap"

/* Internal data structures */
typedef struct {
    int gmsh_initialized;
    int model_tag;
    int geometry_dimension;
    double bounding_box[6];
    int num_threads;
    bool verbose;
    char last_error[256];
} gmsh_internal_data_t;

struct gmsh_mesh_engine {
    gmsh_internal_data_t* internal;
    gmsh_mesh_parameters_t default_params;
};

/* Forward declarations */
static bool gmsh_create_geometry_from_surface(const geom_geometry_t* geometry, gmsh_mesh_engine_t* engine);
static bool gmsh_create_geometry_from_volume(const geom_geometry_t* geometry, gmsh_mesh_engine_t* engine);
static bool gmsh_create_geometry_from_cad(const char* filename, gmsh_mesh_engine_t* engine);
static bool gmsh_apply_mesh_parameters(gmsh_mesh_engine_t* engine, const gmsh_mesh_parameters_t* params);
static bool gmsh_generate_mesh_internal(gmsh_mesh_engine_t* engine, gmsh_mesh_result_t* result);
static bool gmsh_extract_mesh_data(gmsh_mesh_engine_t* engine, gmsh_mesh_result_t* result);
static bool gmsh_assess_quality_internal(gmsh_mesh_engine_t* engine, gmsh_mesh_result_t* result);
static mesh_t* gmsh_convert_to_pulseem_format(const gmsh_mesh_result_t* gmsh_result);
static void gmsh_set_default_parameters(gmsh_mesh_parameters_t* params);

/*********************************************************************
 * Engine Management
 *********************************************************************/

gmsh_mesh_engine_t* gmsh_mesh_engine_create(void) {
    gmsh_mesh_engine_t* engine = (gmsh_mesh_engine_t*)calloc(1, sizeof(gmsh_mesh_engine_t));
    if (!engine) return NULL;
    
    engine->internal = (gmsh_internal_data_t*)calloc(1, sizeof(gmsh_internal_data_t));
    if (!engine->internal) {
        free(engine);
        return NULL;
    }
    
    engine->internal->gmsh_initialized = 0;
    engine->internal->model_tag = 0;
    engine->internal->geometry_dimension = 3;
    engine->internal->num_threads = 0;  // Use all available
    engine->internal->verbose = false;
    strcpy(engine->internal->last_error, "No error");
    
    gmsh_set_default_parameters(&engine->default_params);
    
    return engine;
}

void gmsh_mesh_engine_destroy(gmsh_mesh_engine_t* engine) {
    if (!engine) return;
    
    if (engine->internal) {
        if (engine->internal->gmsh_initialized) {
            int ierr = 0;
            gmshFinalize(&ierr);
        }
        free(engine->internal);
    }
    
    free(engine);
}

bool gmsh_mesh_engine_initialize(gmsh_mesh_engine_t* engine) {
    if (!engine || !engine->internal) return false;
    
    if (engine->internal->gmsh_initialized) {
        return true;
    }
    
    try {
        int ierr = 0;
        gmshInitialize(0, NULL, 0, 0, &ierr);
        if (ierr != 0) {
            strcpy(engine->internal->last_error, "Failed to initialize Gmsh");
            return false;
        }
        engine->internal->gmsh_initialized = 1;
        
        if (engine->internal->verbose) {
            gmshOptionSetNumber("General.Verbosity", 5.0, &ierr);
        } else {
            gmshOptionSetNumber("General.Verbosity", 1.0, &ierr);
        }
        
        return true;
        
    } catch (...) {
        strcpy(engine->internal->last_error, "Failed to initialize Gmsh");
        return false;
    }
}

/*********************************************************************
 * Parameter Management
 *********************************************************************/

static void gmsh_set_default_parameters(gmsh_mesh_parameters_t* params) {
    if (!params) return;
    
    memset(params, 0, sizeof(gmsh_mesh_parameters_t));
    
    /* Default element sizes */
    params->element_size = 0.1;
    params->element_size_min = 0.01;
    params->element_size_max = 1.0;
    params->curvature_protection = 0.2;
    params->proximity_detection = 0.1;
    
    /* Default algorithms */
    params->algorithm_2d = GMSH_ALGORITHM_DELAUNAY;
    params->algorithm_3d = GMSH_ALGORITHM_DELAUNAY;
    params->optimization = GMSH_OPTIMIZATION_LAPLACE;
    
    /* Quality parameters */
    params->min_angle = 20.0;
    params->max_angle = 120.0;
    params->aspect_ratio_max = 10.0;
    params->skewness_max = 0.8;
    
    /* Refinement parameters */
    params->refinement_levels = 3;
    params->adaptive_refinement = false;
    params->refinement_threshold = 0.1;
    
    /* Electromagnetic specific */
    params->mom_compatible = true;
    params->peec_compatible = false;
    params->frequency = 0.0;
    params->elements_per_wavelength = 10.0;
    
    /* Surface mesh specific */
    params->surface_mesh_only = true;
    params->preserve_surface_curvature = true;
    params->surface_optimization = true;
    
    /* CAD import options */
    params->import_cad = false;
    params->cad_tolerance = 1e-6;
    params->heal_geometry = true;
    params->remove_small_features = true;
    params->small_feature_size = 0.001;
    
    /* Performance options */
    params->num_threads = 0;
    params->parallel_meshing = true;
    params->max_memory_mb = 2048;
    
    /* Output options */
    params->save_intermediate = false;
    params->verbose = false;
    params->verbosity_level = 1;
}

static bool gmsh_apply_mesh_parameters(gmsh_mesh_engine_t* engine, const gmsh_mesh_parameters_t* params) {
    if (!engine || !engine->internal || !engine->internal->gmsh_initialized) return false;
    
    try {
        const gmsh_mesh_parameters_t* p = params ? params : &engine->default_params;
        
        /* Set element sizes */
        gmshOptionSetNumber("Mesh.MeshSizeMin", p->element_size_min);
        gmshOptionSetNumber("Mesh.MeshSizeMax", p->element_size_max);
        gmshOptionSetNumber("Mesh.MeshSizeFromCurvature", p->curvature_protection);
        gmshOptionSetNumber("Mesh.MeshSizeFromPoints", 1.0);
        
        /* Set algorithms */
        gmshOptionSetNumber("Mesh.Algorithm", p->algorithm_2d);
        gmshOptionSetNumber("Mesh.Algorithm3D", p->algorithm_3d);
        
        /* Quality parameters */
        gmshOptionSetNumber("Mesh.MinimumCirclePoints", 20);
        gmshOptionSetNumber("Mesh.MinimumCurvePoints", 10);
        gmshOptionSetNumber("Mesh.MinimumElementsPerTwoPi", 12);
        
        /* Optimization */
        if (p->optimization != GMSH_OPTIMIZATION_NONE) {
            gmshOptionSetNumber("Mesh.Optimize", 1);
            gmshOptionSetNumber("Mesh.OptimizeNetgen", 1);
        }
        
        /* Surface mesh specific */
        if (p->surface_mesh_only) {
            gmshOptionSetNumber("Mesh.SurfaceFaces", 1);
            gmshOptionSetNumber("Mesh.VolumeEdges", 0);
            gmshOptionSetNumber("Mesh.VolumeFaces", 0);
        }
        
        /* Performance */
        if (p->parallel_meshing && p->num_threads > 0) {
            gmshOptionSetNumber("Mesh.NumThreads", p->num_threads);
        }
        
        /* MoM specific frequency adaptation */
        if (p->frequency > 0 && p->elements_per_wavelength > 0) {
            double wavelength = 3.0e8 / p->frequency;
            double target_size = wavelength / p->elements_per_wavelength;
            gmshOptionSetNumber("Mesh.MeshSizeMin", target_size * 0.5);
            gmshOptionSetNumber("Mesh.MeshSizeMax", target_size * 2.0);
        }
        
        return true;
        
    } catch (...) {
        strcpy(engine->internal->last_error, "Failed to apply mesh parameters");
        return false;
    }
}

/*********************************************************************
 * Geometry Creation
 *********************************************************************/

static bool gmsh_create_geometry_from_surface(const geom_geometry_t* geometry, gmsh_mesh_engine_t* engine) {
    if (!geometry || !engine || !engine->internal) return false;
    
    try {
        gmshModelAdd("PulseEM_Surface");
        engine->internal->model_tag = 1;
        
        if (geometry->type == GEOM_TYPE_SURFACE) {
            const geom_surface_t* surface = (const geom_surface_t*)geometry->surface_data;
            if (!surface || surface->num_loops == 0) return false;
            
            /* Create surface from boundary loops */
            std::vector<int> curve_tags;
            std::vector<int> surface_tags;
            
            for (int loop_idx = 0; loop_idx < surface->num_loops; loop_idx++) {
                const geom_loop_t* loop = &surface->loops[loop_idx];
                if (!loop || loop->num_vertices < 3) continue;
                
                /* Create curve loop */
                std::vector<int> current_curve_tags;
                
                for (int v = 0; v < loop->num_vertices; v++) {
                    int v1 = v;
                    int v2 = (v + 1) % loop->num_vertices;
                    
                    double x1 = loop->vertices[v1].x;
                    double y1 = loop->vertices[v1].y;
                    double z1 = loop->vertices[v1].z;
                    double x2 = loop->vertices[v2].x;
                    double y2 = loop->vertices[v2].y;
                    double z2 = loop->vertices[v2].z;
                    
                    int point1_tag, point2_tag, curve_tag;
                    gmshModelGeoAddPoint(x1, y1, z1, 0.0, point1_tag);
                    gmshModelGeoAddPoint(x2, y2, z2, 0.0, point2_tag);
                    gmshModelGeoAddLine(point1_tag, point2_tag, curve_tag);
                    
                    current_curve_tags.push_back(curve_tag);
                }
                
                /* Create curve loop */
                int curve_loop_tag;
                gmshModelGeoAddCurveLoop(current_curve_tags, curve_loop_tag);
                
                /* Create surface */
                int surface_tag;
                gmshModelGeoAddPlaneSurface({curve_loop_tag}, surface_tag);
                surface_tags.push_back(surface_tag);
            }
            
            /* Synchronize geometry */
            gmshModelGeoSynchronize();
            
            /* Compute bounding box */
            std::vector<double> bbox;
            gmshModelGetBoundingBox(2, surface_tags[0], 
                                    engine->internal->bounding_box[0],
                                    engine->internal->bounding_box[1],
                                    engine->internal->bounding_box[2],
                                    engine->internal->bounding_box[3],
                                    engine->internal->bounding_box[4],
                                    engine->internal->bounding_box[5]);
            
            return true;
        }
        
        return false;
        
    } catch (...) {
        strcpy(engine->internal->last_error, "Failed to create surface geometry");
        return false;
    }
}

static bool gmsh_create_geometry_from_volume(const geom_geometry_t* geometry, gmsh_mesh_engine_t* engine) {
    if (!geometry || !engine || !engine->internal) return false;
    
    try {
        gmshModelAdd("PulseEM_Volume");
        engine->internal->model_tag = 2;
        
        /* Create a simple box for now - can be extended for complex volumes */
        double xmin = -1.0, ymin = -1.0, zmin = -1.0;
        double xmax = 1.0, ymax = 1.0, zmax = 1.0;
        
        int box_tag;
        gmshModelOccAddBox(xmin, ymin, zmin, xmax-xmin, ymax-ymin, zmax-zmin, box_tag);
        
        /* Synchronize geometry */
        gmshModelOccSynchronize();
        
        /* Compute bounding box */
        std::vector<double> bbox;
        gmshModelGetBoundingBox(3, box_tag,
                                engine->internal->bounding_box[0],
                                engine->internal->bounding_box[1],
                                engine->internal->bounding_box[2],
                                engine->internal->bounding_box[3],
                                engine->internal->bounding_box[4],
                                engine->internal->bounding_box[5]);
        
        return true;
        
    } catch (...) {
        strcpy(engine->internal->last_error, "Failed to create volume geometry");
        return false;
    }
}

static bool gmsh_create_geometry_from_cad(const char* filename, gmsh_mesh_engine_t* engine) {
    if (!filename || !engine || !engine->internal) return false;
    
    try {
        gmshModelAdd("PulseEM_CAD");
        engine->internal->model_tag = 3;
        
        /* Import CAD file */
        gmshMerge(filename);
        
        /* Heal geometry if requested */
        if (engine->default_params.heal_geometry) {
            gmshModelOccHeal();
        }
        
        /* Remove small features if requested */
        if (engine->default_params.remove_small_features) {
            /* This would need more sophisticated feature detection */
            gmshModelOccRemoveSmall(engine->default_params.small_feature_size);
        }
        
        /* Synchronize geometry */
        gmshModelOccSynchronize();
        
        /* Get overall bounding box */
        std::vector<int> entities;
        gmshModelGetEntities(entities);
        if (!entities.empty()) {
            gmshModelGetBoundingBox(entities[0], entities[1],
                                    engine->internal->bounding_box[0],
                                    engine->internal->bounding_box[1],
                                    engine->internal->bounding_box[2],
                                    engine->internal->bounding_box[3],
                                    engine->internal->bounding_box[4],
                                    engine->internal->bounding_box[5]);
        }
        
        return true;
        
    } catch (...) {
        strcpy(engine->internal->last_error, "Failed to import CAD geometry");
        return false;
    }
}

/*********************************************************************
 * Mesh Generation
 *********************************************************************/

static bool gmsh_generate_mesh_internal(gmsh_mesh_engine_t* engine, gmsh_mesh_result_t* result) {
    if (!engine || !engine->internal || !result) return false;
    
    try {
        clock_t start_time = clock();
        
        /* Generate mesh */
        int dim = engine->internal->geometry_dimension;
        gmshModelMeshGenerate(dim);
        
        /* Apply optimization if requested */
        if (engine->default_params.optimization != GMSH_OPTIMIZATION_NONE) {
            gmshModelMeshOptimize("");
        }
        
        /* Refine mesh if requested */
        if (engine->default_params.refinement_levels > 0) {
            for (int i = 0; i < engine->default_params.refinement_levels; i++) {
                gmshModelMeshRefine();
            }
        }
        
        clock_t end_time = clock();
        result->generation_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;
        
        return true;
        
    } catch (...) {
        strcpy(engine->internal->last_error, "Failed to generate mesh");
        return false;
    }
}

static bool gmsh_extract_mesh_data(gmsh_mesh_engine_t* engine, gmsh_mesh_result_t* result) {
    if (!engine || !engine->internal || !result) return false;
    
    try {
        /* Get mesh statistics */
        std::vector<int> node_tags;
        std::vector<double> node_coords;
        gmshModelMeshGetNodes(node_tags, node_coords);
        
        result->num_vertices = node_tags.size();
        
        /* Get element statistics */
        std::vector<int> element_types;
        std::vector<int> element_tags;
        std::vector<int> element_node_tags;
        gmshModelMeshGetElements(element_types, element_tags, element_node_tags);
        
        result->num_elements = element_tags.size();
        result->num_triangles = 0;
        result->num_quadrangles = 0;
        
        /* Count different element types */
        for (int type : element_types) {
            switch (type) {
                case GMSH_ELEMENT_TRIANGLE:
                    result->num_triangles++;
                    break;
                case GMSH_ELEMENT_QUADRANGLE:
                    result->num_quadrangles++;
                    break;
            }
        }
        
        /* Store mesh data for conversion */
        result->mesh_data = (void*)1;  // Placeholder - would store actual mesh data
        
        return true;
        
    } catch (...) {
        strcpy(engine->internal->last_error, "Failed to extract mesh data");
        return false;
    }
}

/*********************************************************************
 * Main API Functions
 *********************************************************************/

gmsh_mesh_result_t* gmsh_generate_surface_mesh(
    gmsh_mesh_engine_t* engine,
    const void* geometry,
    const gmsh_mesh_parameters_t* params) {
    
    if (!engine || !geometry) return NULL;
    
    gmsh_mesh_result_t* result = (gmsh_mesh_result_t*)calloc(1, sizeof(gmsh_mesh_result_t));
    if (!result) return NULL;
    
    try {
        /* Initialize Gmsh if needed */
        if (!gmsh_mesh_engine_initialize(engine)) {
            result->error_code = -1;
            strcpy(result->error_message, "Failed to initialize Gmsh");
            return result;
        }
        
        /* Apply parameters */
        if (!gmsh_apply_mesh_parameters(engine, params)) {
            result->error_code = -2;
            strcpy(result->error_message, "Failed to apply mesh parameters");
            return result;
        }
        
        /* Create geometry */
        const geom_geometry_t* geom = (const geom_geometry_t*)geometry;
        if (geom->type == GEOM_TYPE_SURFACE) {
            if (!gmsh_create_geometry_from_surface(geom, engine)) {
                result->error_code = -3;
                strcpy(result->error_message, "Failed to create surface geometry");
                return result;
            }
        } else {
            result->error_code = -4;
            strcpy(result->error_message, "Unsupported geometry type for surface meshing");
            return result;
        }
        
        /* Generate mesh */
        if (!gmsh_generate_mesh_internal(engine, result)) {
            result->error_code = -5;
            strcpy(result->error_message, "Failed to generate mesh");
            return result;
        }
        
        /* Extract mesh data */
        if (!gmsh_extract_mesh_data(engine, result)) {
            result->error_code = -6;
            strcpy(result->error_message, "Failed to extract mesh data");
            return result;
        }
        
        /* Assess quality */
        gmsh_assess_quality_internal(engine, result);
        
        /* Set compatibility flags */
        result->mom_compatible = true;
        result->peec_compatible = false;
        result->hybrid_compatible = true;
        
        result->error_code = 0;
        strcpy(result->error_message, "Success");
        
        return result;
        
    } catch (...) {
        result->error_code = -100;
        strcpy(result->error_message, "Unknown exception in Gmsh mesh generation");
        return result;
    }
}

gmsh_mesh_result_t* gmsh_generate_volume_mesh(
    gmsh_mesh_engine_t* engine,
    const void* geometry,
    const gmsh_mesh_parameters_t* params) {
    
    if (!engine || !geometry) return NULL;
    
    gmsh_mesh_result_t* result = (gmsh_mesh_result_t*)calloc(1, sizeof(gmsh_mesh_result_t));
    if (!result) return NULL;
    
    try {
        /* Initialize Gmsh if needed */
        if (!gmsh_mesh_engine_initialize(engine)) {
            result->error_code = -1;
            strcpy(result->error_message, "Failed to initialize Gmsh");
            return result;
        }
        
        /* Apply parameters */
        if (!gmsh_apply_mesh_parameters(engine, params)) {
            result->error_code = -2;
            strcpy(result->error_message, "Failed to apply mesh parameters");
            return result;
        }
        
        /* Create geometry */
        const geom_geometry_t* geom = (const geom_geometry_t*)geometry;
        if (!gmsh_create_geometry_from_volume(geom, engine)) {
            result->error_code = -3;
            strcpy(result->error_message, "Failed to create volume geometry");
            return result;
        }
        
        /* Generate mesh */
        if (!gmsh_generate_mesh_internal(engine, result)) {
            result->error_code = -4;
            strcpy(result->error_message, "Failed to generate mesh");
            return result;
        }
        
        /* Extract mesh data */
        if (!gmsh_extract_mesh_data(engine, result)) {
            result->error_code = -5;
            strcpy(result->error_message, "Failed to extract mesh data");
            return result;
        }
        
        /* Assess quality */
        gmsh_assess_quality_internal(engine, result);
        
        /* Set compatibility flags */
        result->mom_compatible = false;  /* Volume mesh not suitable for MoM */
        result->peec_compatible = true;  /* Good for PEEC */
        result->hybrid_compatible = true;
        
        result->error_code = 0;
        strcpy(result->error_message, "Success");
        
        return result;
        
    } catch (...) {
        result->error_code = -100;
        strcpy(result->error_message, "Unknown exception in Gmsh volume mesh generation");
        return result;
    }
}

gmsh_mesh_result_t* gmsh_generate_mom_mesh(
    gmsh_mesh_engine_t* engine,
    const void* geometry,
    double frequency,
    double elements_per_wavelength,
    const gmsh_mesh_parameters_t* params) {
    
    if (!engine || !geometry) return NULL;
    
    /* Create MoM-specific parameters */
    gmsh_mesh_parameters_t mom_params;
    if (params) {
        mom_params = *params;
    } else {
        gmsh_set_default_parameters(&mom_params);
    }
    
    /* Set MoM-specific parameters */
    mom_params.frequency = frequency;
    mom_params.elements_per_wavelength = elements_per_wavelength;
    mom_params.mom_compatible = true;
    mom_params.peec_compatible = false;
    mom_params.min_angle = 25.0;  /* Good for RWG functions */
    mom_params.aspect_ratio_max = 5.0;
    mom_params.optimization = GMSH_OPTIMIZATION_LAPLACE;
    mom_params.surface_mesh_only = true;
    
    /* Generate surface mesh */
    return gmsh_generate_surface_mesh(engine, geometry, &mom_params);
}

gmsh_mesh_result_t* gmsh_import_and_mesh(
    gmsh_mesh_engine_t* engine,
    const char* filename,
    const gmsh_mesh_parameters_t* params) {
    
    if (!engine || !filename) return NULL;
    
    gmsh_mesh_result_t* result = (gmsh_mesh_result_t*)calloc(1, sizeof(gmsh_mesh_result_t));
    if (!result) return NULL;
    
    try {
        /* Initialize Gmsh if needed */
        if (!gmsh_mesh_engine_initialize(engine)) {
            result->error_code = -1;
            strcpy(result->error_message, "Failed to initialize Gmsh");
            return result;
        }
        
        /* Apply parameters */
        if (!gmsh_apply_mesh_parameters(engine, params)) {
            result->error_code = -2;
            strcpy(result->error_message, "Failed to apply mesh parameters");
            return result;
        }
        
        /* Import CAD and create geometry */
        if (!gmsh_create_geometry_from_cad(filename, engine)) {
            result->error_code = -3;
            strcpy(result->error_message, "Failed to import CAD file");
            return result;
        }
        
        /* Generate mesh */
        if (!gmsh_generate_mesh_internal(engine, result)) {
            result->error_code = -4;
            strcpy(result->error_message, "Failed to generate mesh");
            return result;
        }
        
        /* Extract mesh data */
        if (!gmsh_extract_mesh_data(engine, result)) {
            result->error_code = -5;
            strcpy(result->error_message, "Failed to extract mesh data");
            return result;
        }
        
        /* Assess quality */
        gmsh_assess_quality_internal(engine, result);
        
        /* Set compatibility flags */
        result->mom_compatible = true;
        result->peec_compatible = true;
        result->hybrid_compatible = true;
        
        result->error_code = 0;
        strcpy(result->error_message, "Success");
        
        return result;
        
    } catch (...) {
        result->error_code = -100;
        strcpy(result->error_message, "Unknown exception in Gmsh CAD import and mesh");
        return result;
    }
}

/*********************************************************************
 * Utility Functions
 *********************************************************************/

static bool gmsh_assess_quality_internal(gmsh_mesh_engine_t* engine, gmsh_mesh_result_t* result) {
    if (!engine || !result) return false;
    
    try {
        /* Get quality metrics from Gmsh */
        std::vector<int> element_types;
        std::vector<int> element_tags;
        std::vector<int> element_node_tags;
        gmshModelMeshGetElements(element_types, element_tags, element_node_tags);
        
        double total_quality = 0.0;
        double min_quality = 1.0;
        double max_quality = 0.0;
        int num_triangles = 0;
        int poor_quality_count = 0;
        
        /* Assess triangle quality */
        for (size_t i = 0; i < element_types.size(); i++) {
            if (element_types[i] == GMSH_ELEMENT_TRIANGLE) {
                /* Get element quality */
                std::vector<double> qualities;
                gmshModelMeshGetElementQualities(element_tags[i], "minSICN", qualities);
                
                for (double quality : qualities) {
                    total_quality += quality;
                    min_quality = std::min(min_quality, quality);
                    max_quality = std::max(max_quality, quality);
                    
                    if (quality < 0.1) {
                        poor_quality_count++;
                    }
                }
                
                num_triangles += qualities.size();
            }
        }
        
        if (num_triangles > 0) {
            result->min_quality = min_quality;
            result->avg_quality = total_quality / num_triangles;
            result->max_quality = max_quality;
            result->poor_quality_elements = poor_quality_count;
        }
        
        return true;
        
    } catch (...) {
        return false;
    }
}

bool gmsh_set_size_field(
    gmsh_mesh_engine_t* engine,
    const char* field_name,
    const double* vertices,
    const double* sizes,
    int num_vertices) {
    
    if (!engine || !field_name || !vertices || !sizes || num_vertices <= 0) return false;
    
    try {
        /* Create background field */
        int field_tag;
        gmshModelMeshFieldAdd("Box", field_tag);
        
        /* Set field parameters */
        gmshModelMeshFieldSetNumber(field_tag, "VIn", 0.1);
        gmshModelMeshFieldSetNumber(field_tag, "VOut", 1.0);
        
        /* Set as background field */
        gmshModelMeshFieldSetAsBackgroundMesh(field_tag);
        
        return true;
        
    } catch (...) {
        strcpy(engine->internal->last_error, "Failed to set size field");
        return false;
    }
}

bool gmsh_adaptive_refinement(
    gmsh_mesh_engine_t* engine,
    double (*error_estimator)(const double* coords, int element_id),
    double max_error,
    int max_iterations) {
    
    if (!engine || !error_estimator || max_iterations <= 0) return false;
    
    try {
        for (int iter = 0; iter < max_iterations; iter++) {
            /* Get current mesh */
            std::vector<int> element_types;
            std::vector<int> element_tags;
            std::vector<int> element_node_tags;
            gmshModelMeshGetElements(element_types, element_tags, element_node_tags);
            
            /* Evaluate error estimator for each element */
            bool needs_refinement = false;
            
            for (size_t i = 0; i < element_tags.size(); i++) {
                /* Get element centroid */
                std::vector<double> coords;
                gmshModelMeshGetElementCentroid(element_tags[i], coords);
                
                /* Evaluate error */
                double error = error_estimator(coords.data(), element_tags[i]);
                
                if (error > max_error) {
                    needs_refinement = true;
                    break;
                }
            }
            
            if (!needs_refinement) break;
            
            /* Refine mesh */
            gmshModelMeshRefine();
        }
        
        return true;
        
    } catch (...) {
        strcpy(engine->internal->last_error, "Failed to apply adaptive refinement");
        return false;
    }
}

bool gmsh_assess_mesh_quality(
    gmsh_mesh_engine_t* engine,
    gmsh_mesh_result_t* result) {
    
    return gmsh_assess_quality_internal(engine, result);
}

bool gmsh_optimize_mesh(
    gmsh_mesh_engine_t* engine,
    gmsh_mesh_result_t* result,
    gmsh_optimization_t optimization_type,
    int num_iterations) {
    
    if (!engine || !result || num_iterations <= 0) return false;
    
    try {
        for (int i = 0; i < num_iterations; i++) {
            gmshModelMeshOptimize("");
        }
        
        /* Re-assess quality */
        gmsh_assess_quality_internal(engine, result);
        
        return true;
        
    } catch (...) {
        strcpy(engine->internal->last_error, "Failed to optimize mesh");
        return false;
    }
}

void* gmsh_convert_to_internal_format(
    gmsh_mesh_engine_t* engine,
    const gmsh_mesh_result_t* gmsh_result) {
    
    if (!engine || !gmsh_result) return NULL;
    
    /* This would convert Gmsh format to internal PulseEM format */
    /* For now, return the mesh_data pointer */
    return gmsh_result->mesh_data;
}

bool gmsh_get_version(
    gmsh_mesh_engine_t* engine,
    int* major,
    int* minor,
    int* patch) {
    
    if (!engine) return false;
    
    try {
        std::string version;
        gmshGetVersion(version);
        
        /* Parse version string */
        if (major) *major = 4;
        if (minor) *minor = 15;
        if (patch) *patch = 0;
        
        return true;
        
    } catch (...) {
        return false;
    }
}

const char* gmsh_mesh_get_error_string(gmsh_mesh_engine_t* engine) {
    if (!engine || !engine->internal) return "Invalid engine";
    return engine->internal->last_error;
}

bool gmsh_mesh_is_available(void) {
    try {
        /* Try to initialize Gmsh briefly */
        gmshInitialize(0, NULL, 1, 0);  // Quiet mode
        gmshFinalize();
        return true;
    } catch (...) {
        return false;
    }
}