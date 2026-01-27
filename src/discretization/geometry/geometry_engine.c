/********************************************************************************
 * Geometry Engine Implementation (L2 Discretization Layer)
 *
 * Copyright (C) 2025 PulseEM Technologies
 *
 * This file implements geometry processing for discretization.
 * L2 layer: How to convert continuous physical space into degrees of freedom.
 ********************************************************************************/

#include "geometry_engine.h"
#include "../../common/types.h"
#include "../../common/errors.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <stdint.h>

// Internal geometry engine structure
struct geometry_engine {
    geom_entity_t* entities;
    int num_entities;
    int max_entities;
    
    real_t bbox_min[3];
    real_t bbox_max[3];
    bool bbox_computed;
};

// Forward declarations for helper functions
static int parse_stl_ascii(struct geometry_engine* engine, FILE* file);
static int parse_stl_binary(struct geometry_engine* engine, FILE* file);
static int parse_obj_file(struct geometry_engine* engine, FILE* file);
static int parse_dxf_file(struct geometry_engine* engine, FILE* file);

void* geometry_engine_create(void) {
    struct geometry_engine* engine = (struct geometry_engine*)calloc(1, sizeof(struct geometry_engine));
    if (!engine) return NULL;
    
    engine->max_entities = 1024;
    engine->entities = (geom_entity_t*)calloc(engine->max_entities, sizeof(geom_entity_t));
    if (!engine->entities) {
        free(engine);
        return NULL;
    }
    
    engine->bbox_min[0] = engine->bbox_min[1] = engine->bbox_min[2] = 1e10;
    engine->bbox_max[0] = engine->bbox_max[1] = engine->bbox_max[2] = -1e10;
    engine->bbox_computed = false;
    
    return engine;
}

void geometry_engine_destroy(void* engine_ptr) {
    if (!engine_ptr) return;
    
    struct geometry_engine* engine = (struct geometry_engine*)engine_ptr;
    
    // Free entities
    if (engine->entities) {
        for (int i = 0; i < engine->num_entities; i++) {
            if (engine->entities[i].vertices) {
                free(engine->entities[i].vertices);
            }
        }
        free(engine->entities);
    }
    
    free(engine);
}

int geometry_engine_import_file(
    void* engine_ptr,
    const char* filename,
    geom_format_t format) {
    
    if (!engine_ptr || !filename) return STATUS_ERROR_INVALID_INPUT;
    
    struct geometry_engine* engine = (struct geometry_engine*)engine_ptr;
    
    // L2 layer handles file parsing, not physics interpretation
    FILE* file = fopen(filename, "rb");
    if (!file) {
        return STATUS_ERROR_FILE_NOT_FOUND;
    }
    
    int status = STATUS_SUCCESS;
    
    switch (format) {
        case GEOM_FORMAT_STL: {
            // Parse STL file (ASCII or binary)
            // Check if ASCII or binary
            char header[80];
            if (fread(header, 1, 80, file) != 80) {
                fclose(file);
                return STATUS_ERROR_INVALID_FORMAT;
            }
            
            // Check if ASCII (starts with "solid")
            fseek(file, 0, SEEK_SET);
            char first_line[256];
            if (fgets(first_line, sizeof(first_line), file)) {
                if (strncmp(first_line, "solid", 5) == 0) {
                    // ASCII STL
                    status = parse_stl_ascii(engine, file);
                } else {
                    // Binary STL
                    status = parse_stl_binary(engine, file);
                }
            } else {
                status = STATUS_ERROR_INVALID_FORMAT;
            }
            break;
        }
        
        case GEOM_FORMAT_OBJ: {
            // Parse OBJ file
            status = parse_obj_file(engine, file);
            break;
        }
        
        case GEOM_FORMAT_DXF: {
            // Parse DXF file (basic implementation)
            status = parse_dxf_file(engine, file);
            break;
        }
        
        case GEOM_FORMAT_STEP:
        case GEOM_FORMAT_IGES:
        case GEOM_FORMAT_GDSII:
        case GEOM_FORMAT_OASIS:
        case GEOM_FORMAT_GERBER:
            // These formats require specialized parsers
            // For now, return not implemented
            status = STATUS_ERROR_NOT_IMPLEMENTED;
            break;
            
        default:
            status = STATUS_ERROR_INVALID_FORMAT;
            break;
    }
    
    fclose(file);
    return status;
}

// Helper: Parse ASCII STL file
static int parse_stl_ascii(
    struct geometry_engine* engine,
    FILE* file) {
    
    if (!engine || !file) return STATUS_ERROR_INVALID_INPUT;
    
    char line[1024];
    geom_entity_t entity;
    memset(&entity, 0, sizeof(geom_entity_t));
    
    int triangle_count = 0;
    real_t current_normal[3] = {0.0, 0.0, 0.0};
    real_t triangle_vertices[3][3];
    int vertex_count = 0;
    
    while (fgets(line, sizeof(line), file)) {
        // Remove whitespace
        char* trimmed = line;
        while (*trimmed == ' ' || *trimmed == '\t') trimmed++;
        
        if (strncmp(trimmed, "facet normal", 12) == 0) {
            // Parse normal vector
            sscanf(trimmed, "facet normal %lf %lf %lf",
                   &current_normal[0], &current_normal[1], &current_normal[2]);
            vertex_count = 0;
        } else if (strncmp(trimmed, "vertex", 6) == 0) {
            // Parse vertex
            if (vertex_count < 3) {
                sscanf(trimmed, "vertex %lf %lf %lf",
                       &triangle_vertices[vertex_count][0],
                       &triangle_vertices[vertex_count][1],
                       &triangle_vertices[vertex_count][2]);
                vertex_count++;
            }
        } else if (strncmp(trimmed, "endfacet", 8) == 0) {
            // Complete triangle
            if (vertex_count == 3) {
                // Create triangle entity
                entity.id = engine->num_entities;
                entity.type = GEOM_TYPE_TRIANGLE;
                entity.num_vertices = 3;
                entity.vertices = (geom_vertex_t*)malloc(3 * sizeof(geom_vertex_t));
                
                if (!entity.vertices) {
                    return STATUS_ERROR_MEMORY_ALLOCATION;
                }
                
                // Copy vertices
                for (int i = 0; i < 3; i++) {
                    entity.vertices[i].id = i;
                    entity.vertices[i].coordinates[0] = triangle_vertices[i][0];
                    entity.vertices[i].coordinates[1] = triangle_vertices[i][1];
                    entity.vertices[i].coordinates[2] = triangle_vertices[i][2];
                }
                
                // Compute triangle center
                entity.center[0] = (triangle_vertices[0][0] + triangle_vertices[1][0] + triangle_vertices[2][0]) / 3.0;
                entity.center[1] = (triangle_vertices[0][1] + triangle_vertices[1][1] + triangle_vertices[2][1]) / 3.0;
                entity.center[2] = (triangle_vertices[0][2] + triangle_vertices[1][2] + triangle_vertices[2][2]) / 3.0;
                
                // Add entity
                int add_status = geometry_engine_add_entity(engine, &entity);
                if (add_status != STATUS_SUCCESS) {
                    free(entity.vertices);
                    return add_status;
                }
                
                triangle_count++;
            }
            vertex_count = 0;
        } else if (strncmp(trimmed, "endsolid", 8) == 0) {
            // End of STL file
            break;
        }
    }
    
    return STATUS_SUCCESS;
}

// Helper: Parse binary STL file
static int parse_stl_binary(
    struct geometry_engine* engine,
    FILE* file) {
    
    if (!engine || !file) return STATUS_ERROR_INVALID_INPUT;
    
    // Skip header (80 bytes)
    fseek(file, 80, SEEK_SET);
    
    // Read number of triangles
    uint32_t num_triangles;
    if (fread(&num_triangles, sizeof(uint32_t), 1, file) != 1) {
        return STATUS_ERROR_INVALID_FORMAT;
    }
    
    // Read triangles
    for (uint32_t i = 0; i < num_triangles; i++) {
        // Read normal (3 floats)
        float normal[3];
        if (fread(normal, sizeof(float), 3, file) != 3) {
            return STATUS_ERROR_INVALID_FORMAT;
        }
        
        // Read vertices (3 vertices * 3 floats)
        float vertices[3][3];
        for (int j = 0; j < 3; j++) {
            if (fread(vertices[j], sizeof(float), 3, file) != 3) {
                return STATUS_ERROR_INVALID_FORMAT;
            }
        }
        
        // Skip attribute byte count (2 bytes)
        uint16_t attr_count;
        if (fread(&attr_count, sizeof(uint16_t), 1, file) != 1) {
            return STATUS_ERROR_INVALID_FORMAT;
        }
        
        // Create triangle entity
        geom_entity_t entity;
        memset(&entity, 0, sizeof(geom_entity_t));
        entity.id = engine->num_entities;
        entity.type = GEOM_TYPE_TRIANGLE;
        entity.num_vertices = 3;
        entity.vertices = (geom_vertex_t*)malloc(3 * sizeof(geom_vertex_t));
        
        if (!entity.vertices) {
            return STATUS_ERROR_MEMORY_ALLOCATION;
        }
        
        // Copy vertices
        for (int j = 0; j < 3; j++) {
            entity.vertices[j].id = j;
            entity.vertices[j].coordinates[0] = (real_t)vertices[j][0];
            entity.vertices[j].coordinates[1] = (real_t)vertices[j][1];
            entity.vertices[j].coordinates[2] = (real_t)vertices[j][2];
        }
        
        // Compute triangle center
        entity.center[0] = ((real_t)vertices[0][0] + (real_t)vertices[1][0] + (real_t)vertices[2][0]) / 3.0;
        entity.center[1] = ((real_t)vertices[0][1] + (real_t)vertices[1][1] + (real_t)vertices[2][1]) / 3.0;
        entity.center[2] = ((real_t)vertices[0][2] + (real_t)vertices[1][2] + (real_t)vertices[2][2]) / 3.0;
        
        // Add entity
        int status = geometry_engine_add_entity(engine, &entity);
        if (status != STATUS_SUCCESS) {
            free(entity.vertices);
            return status;
        }
    }
    
    return STATUS_SUCCESS;
}

// Helper: Parse OBJ file
static int parse_obj_file(
    struct geometry_engine* engine,
    FILE* file) {
    
    if (!engine || !file) return STATUS_ERROR_INVALID_INPUT;
    
    char line[1024];
    geom_vertex_t* vertices = NULL;
    int num_vertices = 0;
    int max_vertices = 1024;
    vertices = (geom_vertex_t*)malloc(max_vertices * sizeof(geom_vertex_t));
    if (!vertices) return STATUS_ERROR_MEMORY_ALLOCATION;
    
    // Parse OBJ file line by line
    while (fgets(line, sizeof(line), file)) {
        // Remove whitespace
        char* trimmed = line;
        while (*trimmed == ' ' || *trimmed == '\t') trimmed++;
        
        if (trimmed[0] == 'v' && trimmed[1] == ' ') {
            // Vertex: v x y z
            if (num_vertices >= max_vertices) {
                max_vertices *= 2;
                geom_vertex_t* new_vertices = (geom_vertex_t*)realloc(vertices, max_vertices * sizeof(geom_vertex_t));
                if (!new_vertices) {
                    free(vertices);
                    return STATUS_ERROR_MEMORY_ALLOCATION;
                }
                vertices = new_vertices;
            }
            
            sscanf(trimmed, "v %lf %lf %lf",
                   &vertices[num_vertices].coordinates[0],
                   &vertices[num_vertices].coordinates[1],
                   &vertices[num_vertices].coordinates[2]);
            vertices[num_vertices].id = num_vertices;
            num_vertices++;
        } else if (trimmed[0] == 'f' && trimmed[1] == ' ') {
            // Face: f v1 v2 v3 ... (can be triangle or polygon)
            // Parse face indices
            int face_indices[16];  // Support up to 16 vertices per face
            int num_face_vertices = 0;
            char* token = strtok(trimmed + 2, " \t\n");
            
            while (token && num_face_vertices < 16) {
                // OBJ format: f v1/vt1/vn1 v2/vt2/vn2 ...
                // Extract vertex index (before first '/')
                int vertex_idx = atoi(token) - 1;  // OBJ uses 1-based indexing
                if (vertex_idx >= 0 && vertex_idx < num_vertices) {
                    face_indices[num_face_vertices++] = vertex_idx;
                }
                token = strtok(NULL, " \t\n");
            }
            
            // Create triangle entities from face
            // Triangulate polygon if necessary
            for (int i = 1; i < num_face_vertices - 1; i++) {
                geom_entity_t entity;
                memset(&entity, 0, sizeof(geom_entity_t));
                entity.id = engine->num_entities;
                entity.type = GEOM_TYPE_TRIANGLE;
                entity.num_vertices = 3;
                entity.vertices = (geom_vertex_t*)malloc(3 * sizeof(geom_vertex_t));
                
                if (!entity.vertices) {
                    free(vertices);
                    return STATUS_ERROR_MEMORY_ALLOCATION;
                }
                
                // Copy triangle vertices
                entity.vertices[0] = vertices[face_indices[0]];
                entity.vertices[1] = vertices[face_indices[i]];
                entity.vertices[2] = vertices[face_indices[i + 1]];
                
                // Compute triangle center
                entity.center[0] = (entity.vertices[0].coordinates[0] + entity.vertices[1].coordinates[0] + entity.vertices[2].coordinates[0]) / 3.0;
                entity.center[1] = (entity.vertices[0].coordinates[1] + entity.vertices[1].coordinates[1] + entity.vertices[2].coordinates[1]) / 3.0;
                entity.center[2] = (entity.vertices[0].coordinates[2] + entity.vertices[1].coordinates[2] + entity.vertices[2].coordinates[2]) / 3.0;
                
                // Add entity
                int status = geometry_engine_add_entity(engine, &entity);
                if (status != STATUS_SUCCESS) {
                    free(entity.vertices);
                    free(vertices);
                    return status;
                }
            }
        }
    }
    
    free(vertices);
    return STATUS_SUCCESS;
}

// Helper: Parse DXF file (basic implementation)
// DXF format: group code (integer) followed by value (string/number)
// Supports 3DFACE (triangles) and basic POLYLINE entities
static int parse_dxf_file(
    struct geometry_engine* engine,
    FILE* file) {
    
    if (!engine || !file) return STATUS_ERROR_INVALID_INPUT;
    
    char line[1024];
    int group_code = 0;
    char group_value[256];
    int in_entities_section = 0;
    int in_3dface = 0;
    int in_polyline = 0;
    int vertex_count = 0;
    real_t current_vertices[4][3];  // Support up to 4 vertices for 3DFACE
    real_t polyline_vertices[1024][3];
    int polyline_vertex_count = 0;
    
    // Parse DXF file line by line
    while (fgets(line, sizeof(line), file)) {
        // Remove trailing whitespace
        int len = (int)strlen(line);
        while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r')) {
            line[len-1] = '\0';
            len--;
        }
        
        // DXF format: alternating lines of group code and value
        if (group_code == 0) {
            // This line is a group code
            group_code = atoi(line);
        } else {
            // This line is a group value
            strncpy(group_value, line, sizeof(group_value) - 1);
            group_value[sizeof(group_value) - 1] = '\0';
            
            // Process based on group code
            if (group_code == 2) {
                // Section/Entity name
                if (strcmp(group_value, "ENTITIES") == 0) {
                    in_entities_section = 1;
                }
            } else if (group_code == 0 && in_entities_section) {
                // Entity type
                if (strcmp(group_value, "3DFACE") == 0) {
                    in_3dface = 1;
                    vertex_count = 0;
                    memset(current_vertices, 0, sizeof(current_vertices));
                } else if (strcmp(group_value, "POLYLINE") == 0) {
                    in_polyline = 1;
                    polyline_vertex_count = 0;
                } else if (strcmp(group_value, "SEQEND") == 0) {
                    // End of polyline
                    if (in_polyline && polyline_vertex_count >= 3) {
                        // Triangulate polyline
                        for (int i = 1; i < polyline_vertex_count - 1; i++) {
                            geom_entity_t entity;
                            memset(&entity, 0, sizeof(geom_entity_t));
                            entity.id = engine->num_entities;
                            entity.type = GEOM_TYPE_TRIANGLE;
                            entity.num_vertices = 3;
                            entity.vertices = (geom_vertex_t*)malloc(3 * sizeof(geom_vertex_t));
                            
                            if (!entity.vertices) {
                                return STATUS_ERROR_MEMORY_ALLOCATION;
                            }
                            
                            // Create triangle from polyline vertices
                            entity.vertices[0].id = 0;
                            entity.vertices[0].coordinates[0] = polyline_vertices[0][0];
                            entity.vertices[0].coordinates[1] = polyline_vertices[0][1];
                            entity.vertices[0].coordinates[2] = polyline_vertices[0][2];
                            
                            entity.vertices[1].id = 1;
                            entity.vertices[1].coordinates[0] = polyline_vertices[i][0];
                            entity.vertices[1].coordinates[1] = polyline_vertices[i][1];
                            entity.vertices[1].coordinates[2] = polyline_vertices[i][2];
                            
                            entity.vertices[2].id = 2;
                            entity.vertices[2].coordinates[0] = polyline_vertices[i + 1][0];
                            entity.vertices[2].coordinates[1] = polyline_vertices[i + 1][1];
                            entity.vertices[2].coordinates[2] = polyline_vertices[i + 1][2];
                            
                            // Compute triangle center
                            entity.center[0] = (entity.vertices[0].coordinates[0] + entity.vertices[1].coordinates[0] + entity.vertices[2].coordinates[0]) / 3.0;
                            entity.center[1] = (entity.vertices[0].coordinates[1] + entity.vertices[1].coordinates[1] + entity.vertices[2].coordinates[1]) / 3.0;
                            entity.center[2] = (entity.vertices[0].coordinates[2] + entity.vertices[1].coordinates[2] + entity.vertices[2].coordinates[2]) / 3.0;
                            
                            int status = geometry_engine_add_entity(engine, &entity);
                            if (status != STATUS_SUCCESS) {
                                free(entity.vertices);
                                return status;
                            }
                        }
                    }
                    in_polyline = 0;
                    polyline_vertex_count = 0;
                } else if (in_3dface && strcmp(group_value, "3DFACE") != 0 && strcmp(group_value, "SEQEND") != 0) {
                    // End of 3DFACE entity
                    if (vertex_count >= 3) {
                        // Create triangle entity from 3DFACE
                        geom_entity_t entity;
                        memset(&entity, 0, sizeof(geom_entity_t));
                        entity.id = engine->num_entities;
                        entity.type = GEOM_TYPE_TRIANGLE;
                        entity.num_vertices = 3;
                        entity.vertices = (geom_vertex_t*)malloc(3 * sizeof(geom_vertex_t));
                        
                        if (!entity.vertices) {
                            return STATUS_ERROR_MEMORY_ALLOCATION;
                        }
                        
                        // Copy vertices
                        for (int i = 0; i < 3; i++) {
                            entity.vertices[i].id = i;
                            entity.vertices[i].coordinates[0] = current_vertices[i][0];
                            entity.vertices[i].coordinates[1] = current_vertices[i][1];
                            entity.vertices[i].coordinates[2] = current_vertices[i][2];
                        }
                        
                        // Compute triangle center
                        entity.center[0] = (current_vertices[0][0] + current_vertices[1][0] + current_vertices[2][0]) / 3.0;
                        entity.center[1] = (current_vertices[0][1] + current_vertices[1][1] + current_vertices[2][1]) / 3.0;
                        entity.center[2] = (current_vertices[0][2] + current_vertices[1][2] + current_vertices[2][2]) / 3.0;
                        
                        int status = geometry_engine_add_entity(engine, &entity);
                        if (status != STATUS_SUCCESS) {
                            free(entity.vertices);
                            return status;
                        }
                    }
                    in_3dface = 0;
                    vertex_count = 0;
                }
            } else if (in_3dface) {
                // Parse 3DFACE vertex coordinates
                // Group codes: 10,20,30 = first vertex (x,y,z)
                //              11,21,31 = second vertex
                //              12,22,32 = third vertex
                //              13,23,33 = fourth vertex (optional)
                if (group_code == 10 || group_code == 11 || group_code == 12 || group_code == 13) {
                    int vtx_idx = (group_code == 10) ? 0 : (group_code == 11) ? 1 : (group_code == 12) ? 2 : 3;
                    if (vtx_idx < 4) {
                        current_vertices[vtx_idx][0] = (real_t)atof(group_value);
                    }
                } else if (group_code == 20 || group_code == 21 || group_code == 22 || group_code == 23) {
                    int vtx_idx = (group_code == 20) ? 0 : (group_code == 21) ? 1 : (group_code == 22) ? 2 : 3;
                    if (vtx_idx < 4) {
                        current_vertices[vtx_idx][1] = (real_t)atof(group_value);
                    }
                } else if (group_code == 30 || group_code == 31 || group_code == 32 || group_code == 33) {
                    int vtx_idx = (group_code == 30) ? 0 : (group_code == 31) ? 1 : (group_code == 32) ? 2 : 3;
                    if (vtx_idx < 4) {
                        current_vertices[vtx_idx][2] = (real_t)atof(group_value);
                        if (vtx_idx < 3) {
                            vertex_count = (vtx_idx + 1 > vertex_count) ? vtx_idx + 1 : vertex_count;
                        }
                    }
                }
            } else if (in_polyline) {
                // Parse POLYLINE vertex (VERTEX entity)
                // Group codes: 10,20,30 = vertex coordinates
                if (group_code == 10) {
                    if (polyline_vertex_count < 1024) {
                        polyline_vertices[polyline_vertex_count][0] = (real_t)atof(group_value);
                    }
                } else if (group_code == 20) {
                    if (polyline_vertex_count < 1024) {
                        polyline_vertices[polyline_vertex_count][1] = (real_t)atof(group_value);
                    }
                } else if (group_code == 30) {
                    if (polyline_vertex_count < 1024) {
                        polyline_vertices[polyline_vertex_count][2] = (real_t)atof(group_value);
                        polyline_vertex_count++;
                    }
                }
            }
            
            // Reset group code for next iteration
            group_code = 0;
        }
    }
    
    // Handle any remaining 3DFACE
    if (in_3dface && vertex_count >= 3) {
        geom_entity_t entity;
        memset(&entity, 0, sizeof(geom_entity_t));
        entity.id = engine->num_entities;
        entity.type = GEOM_TYPE_TRIANGLE;
        entity.num_vertices = 3;
        entity.vertices = (geom_vertex_t*)malloc(3 * sizeof(geom_vertex_t));
        
        if (!entity.vertices) {
            return STATUS_ERROR_MEMORY_ALLOCATION;
        }
        
        for (int i = 0; i < 3; i++) {
            entity.vertices[i].id = i;
            entity.vertices[i].coordinates[0] = current_vertices[i][0];
            entity.vertices[i].coordinates[1] = current_vertices[i][1];
            entity.vertices[i].coordinates[2] = current_vertices[i][2];
        }
        
        entity.center[0] = (current_vertices[0][0] + current_vertices[1][0] + current_vertices[2][0]) / 3.0;
        entity.center[1] = (current_vertices[0][1] + current_vertices[1][1] + current_vertices[2][1]) / 3.0;
        entity.center[2] = (current_vertices[0][2] + current_vertices[1][2] + current_vertices[2][2]) / 3.0;
        
        int status = geometry_engine_add_entity(engine, &entity);
        if (status != STATUS_SUCCESS) {
            free(entity.vertices);
            return status;
        }
    }
    
    return STATUS_SUCCESS;
}

int geometry_engine_add_entity(
    void* engine_ptr,
    const geom_entity_t* entity) {
    
    if (!engine_ptr || !entity) return STATUS_ERROR_INVALID_INPUT;
    
    struct geometry_engine* engine = (struct geometry_engine*)engine_ptr;
    
    // Check if we need to expand
    if (engine->num_entities >= engine->max_entities) {
        int new_max = engine->max_entities * 2;
        geom_entity_t* new_entities = (geom_entity_t*)realloc(
            engine->entities, new_max * sizeof(geom_entity_t));
        if (!new_entities) return STATUS_ERROR_MEMORY_ALLOCATION;
        engine->entities = new_entities;
        engine->max_entities = new_max;
    }
    
    // Copy entity
    engine->entities[engine->num_entities] = *entity;
    
    // Copy vertices if present
    if (entity->vertices && entity->num_vertices > 0) {
        engine->entities[engine->num_entities].vertices = 
            (geom_vertex_t*)malloc(entity->num_vertices * sizeof(geom_vertex_t));
        if (!engine->entities[engine->num_entities].vertices) {
            return STATUS_ERROR_MEMORY_ALLOCATION;
        }
        memcpy(engine->entities[engine->num_entities].vertices,
               entity->vertices,
               entity->num_vertices * sizeof(geom_vertex_t));
    }
    
    engine->num_entities++;
    engine->bbox_computed = false;
    
    return STATUS_SUCCESS;
}

int geometry_engine_get_bbox(
    void* engine_ptr,
    real_t* bbox_min,
    real_t* bbox_max) {
    
    if (!engine_ptr || !bbox_min || !bbox_max) return STATUS_ERROR_INVALID_INPUT;
    
    struct geometry_engine* engine = (struct geometry_engine*)engine_ptr;
    
    // Compute bbox if not already computed
    if (!engine->bbox_computed) {
        engine->bbox_min[0] = engine->bbox_min[1] = engine->bbox_min[2] = 1e10;
        engine->bbox_max[0] = engine->bbox_max[1] = engine->bbox_max[2] = -1e10;
        
        for (int i = 0; i < engine->num_entities; i++) {
            if (engine->entities[i].bbox_min[0] < engine->bbox_min[0])
                engine->bbox_min[0] = engine->entities[i].bbox_min[0];
            if (engine->entities[i].bbox_max[0] > engine->bbox_max[0])
                engine->bbox_max[0] = engine->entities[i].bbox_max[0];
            
            if (engine->entities[i].bbox_min[1] < engine->bbox_min[1])
                engine->bbox_min[1] = engine->entities[i].bbox_min[1];
            if (engine->entities[i].bbox_max[1] > engine->bbox_max[1])
                engine->bbox_max[1] = engine->entities[i].bbox_max[1];
            
            if (engine->entities[i].bbox_min[2] < engine->bbox_min[2])
                engine->bbox_min[2] = engine->entities[i].bbox_min[2];
            if (engine->entities[i].bbox_max[2] > engine->bbox_max[2])
                engine->bbox_max[2] = engine->entities[i].bbox_max[2];
        }
        
        engine->bbox_computed = true;
    }
    
    bbox_min[0] = engine->bbox_min[0];
    bbox_min[1] = engine->bbox_min[1];
    bbox_min[2] = engine->bbox_min[2];
    bbox_max[0] = engine->bbox_max[0];
    bbox_max[1] = engine->bbox_max[1];
    bbox_max[2] = engine->bbox_max[2];
    
    return STATUS_SUCCESS;
}

bool geometry_engine_validate(void* engine_ptr) {
    if (!engine_ptr) return false;
    
    struct geometry_engine* engine = (struct geometry_engine*)engine_ptr;
    
    // L2 layer checks geometric validity, not physics
    for (int i = 0; i < engine->num_entities; i++) {
        // Check entity validity
        if (engine->entities[i].id < 0) return false;
        if (engine->entities[i].type < GEOM_TYPE_POINT || 
            engine->entities[i].type > GEOM_TYPE_CIRCLE) return false;
    }
    
    return true;
}
