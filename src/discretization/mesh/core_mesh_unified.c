/**
 * @file core_mesh_unified.c
 * @brief Unified mesh_create/mesh_destroy implementation for core_mesh.h
 *
 * Provides the single implementation of mesh_create and mesh_destroy used
 * by MoM solver, PEEC solver, mesh pipeline, and CAD import path.
 *
 * Copyright (c) 2024-2025 PulseEM Technology Group
 * SPDX-License-Identifier: Proprietary
 */

#include "core_mesh.h"
#include <stdlib.h>
#include <string.h>

/*********************************************************************
 * mesh_create - allocate and initialize a mesh_t
 *********************************************************************/
mesh_t* mesh_create(const char* name, mesh_type_t type) {
    mesh_t* mesh = (mesh_t*)calloc(1, sizeof(mesh_t));
    if (!mesh) return NULL;

    if (name) {
        strncpy(mesh->name, name, sizeof(mesh->name) - 1);
        mesh->name[sizeof(mesh->name) - 1] = '\0';
    }
    mesh->type = type;
    mesh->algorithm = MESH_ALGORITHM_UNSTRUCTURED;
    mesh->vertices = NULL;
    mesh->edges = NULL;
    mesh->elements = NULL;
    mesh->num_vertices = 0;
    mesh->num_edges = 0;
    mesh->num_elements = 0;
    mesh->vertex_to_elements = NULL;
    mesh->element_to_elements = NULL;
    mesh->partition_offsets = NULL;
    mesh->partition_elements = NULL;
    mesh->num_partitions = 0;
    mesh->num_mom_elements = 0;
    mesh->num_peec_elements = 0;
    mesh->num_interface_elements = 0;
    return mesh;
}

/*********************************************************************
 * mesh_destroy - free all dynamic data and the mesh
 *********************************************************************/
void mesh_destroy(mesh_t* mesh) {
    if (!mesh) return;

    /* Free per-vertex adjacent_elements */
    if (mesh->vertices) {
        for (int i = 0; i < mesh->num_vertices; i++) {
            if (mesh->vertices[i].adjacent_elements) {
                free(mesh->vertices[i].adjacent_elements);
                mesh->vertices[i].adjacent_elements = NULL;
            }
        }
        free(mesh->vertices);
        mesh->vertices = NULL;
    }

    free(mesh->edges);
    mesh->edges = NULL;

    /* Free per-element vertices and edges */
    if (mesh->elements) {
        for (int i = 0; i < mesh->num_elements; i++) {
            if (mesh->elements[i].vertices) {
                free(mesh->elements[i].vertices);
                mesh->elements[i].vertices = NULL;
            }
            if (mesh->elements[i].edges) {
                free(mesh->elements[i].edges);
                mesh->elements[i].edges = NULL;
            }
        }
        free(mesh->elements);
        mesh->elements = NULL;
    }

    /* Free adjacency arrays (int**) - use counts before clearing */
    {
        int nv = mesh->num_vertices;
        int ne = mesh->num_elements;
        if (mesh->vertex_to_elements) {
            for (int i = 0; i < nv; i++) {
                if (mesh->vertex_to_elements[i]) free(mesh->vertex_to_elements[i]);
            }
            free(mesh->vertex_to_elements);
            mesh->vertex_to_elements = NULL;
        }
        if (mesh->element_to_elements) {
            for (int i = 0; i < ne; i++) {
                if (mesh->element_to_elements[i]) free(mesh->element_to_elements[i]);
            }
            free(mesh->element_to_elements);
            mesh->element_to_elements = NULL;
        }
    }

    free(mesh->partition_offsets);
    mesh->partition_offsets = NULL;
    free(mesh->partition_elements);
    mesh->partition_elements = NULL;

    free(mesh);
}
