/**
 * @file gmsh_mesh_import.c
 * @brief Import surface triangular mesh from CAD using Gmsh C API (gmshc.h).
 * Produces mesh_t for MoM solver (RWG-ready: vertices, edges, elements).
 */

#define _CRT_SECURE_NO_WARNINGS

#include "core_mesh.h"
#include "../../discretization/geometry/core_geometry.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* Gmsh C API - include path set in project (e.g. $(SolutionDir)libs/gmsh-4.15.0-Windows64-sdk/include) */
#include "gmshc.h"

#define GMSH_ELEMENT_TRIANGLE 2

/* Simple edge key: canonical (min, max) vertex pair for dedup */
typedef struct {
    int vmin;
    int vmax;
} edge_key_t;

static int edge_key_compare(const void* a, const void* b) {
    const edge_key_t* ea = (const edge_key_t*)a;
    const edge_key_t* eb = (const edge_key_t*)b;
    if (ea->vmin != eb->vmin) return (ea->vmin > eb->vmin) ? 1 : -1;
    if (ea->vmax != eb->vmax) return (ea->vmax > eb->vmax) ? 1 : -1;
    return 0;
}

/* Build mesh_t from Gmsh node/element arrays (shared by CAD and .msh import). */
static mesh_t* build_mesh_from_gmsh_arrays(
    const size_t* nodeTags, size_t nodeTags_n,
    const double* coord,
    const size_t* nodeTagsElem, size_t elemTags_n) {
    mesh_t* mesh = NULL;
    int* tag2idx = NULL;
    size_t max_node_tag = 0;
    edge_key_t* edge_keys = NULL;
    int num_edges = 0;
    int edge_cap = (int)elemTags_n * 3 * 2;
    int unique_edges = 0;

    if (!nodeTags || !coord || nodeTags_n == 0 || !nodeTagsElem || elemTags_n == 0) return NULL;

    for (size_t i = 0; i < nodeTags_n; i++) {
        if (nodeTags[i] > max_node_tag) max_node_tag = nodeTags[i];
    }
    if (max_node_tag > (size_t)(1024 * 1024 * 64)) max_node_tag = nodeTags_n;
    tag2idx = (int*)calloc((size_t)(max_node_tag + 1), sizeof(int));
    if (!tag2idx) return NULL;
    for (size_t i = 0; i < nodeTags_n; i++) {
        if (nodeTags[i] <= max_node_tag) tag2idx[nodeTags[i]] = (int)i;
    }

    mesh = mesh_create("gmsh_mesh", MESH_TYPE_TRIANGULAR);
    if (!mesh) { free(tag2idx); return NULL; }
    mesh->num_vertices = (int)nodeTags_n;
    mesh->vertices = (mesh_vertex_t*)calloc((size_t)mesh->num_vertices, sizeof(mesh_vertex_t));
    if (!mesh->vertices) { mesh_destroy(mesh); free(tag2idx); return NULL; }
    for (int i = 0; i < mesh->num_vertices; i++) {
        mesh_vertex_t* v = &mesh->vertices[i];
        v->id = i;
        v->position.x = coord[3 * (size_t)i];
        v->position.y = coord[3 * (size_t)i + 1];
        v->position.z = coord[3 * (size_t)i + 2];
        v->is_boundary = false;
        v->is_interface = false;
        v->adjacent_elements = NULL;
        v->num_adjacent_elements = 0;
    }

    edge_keys = (edge_key_t*)malloc((size_t)edge_cap * sizeof(edge_key_t));
    if (!edge_keys) { mesh_destroy(mesh); free(tag2idx); return NULL; }
    num_edges = 0;
    for (size_t t = 0; t < elemTags_n; t++) {
        size_t n0 = nodeTagsElem[3 * t], n1 = nodeTagsElem[3 * t + 1], n2 = nodeTagsElem[3 * t + 2];
        int i0 = (n0 <= max_node_tag) ? tag2idx[n0] : -1;
        int i1 = (n1 <= max_node_tag) ? tag2idx[n1] : -1;
        int i2 = (n2 <= max_node_tag) ? tag2idx[n2] : -1;
        if (i0 < 0 || i1 < 0 || i2 < 0) continue;
        for (int k = 0; k < 3; k++) {
            int a = (k == 0) ? i0 : (k == 1) ? i1 : i2;
            int b = (k == 0) ? i1 : (k == 1) ? i2 : i0;
            int vmin = (a < b) ? a : b;
            int vmax = (a < b) ? b : a;
            edge_keys[num_edges].vmin = vmin;
            edge_keys[num_edges].vmax = vmax;
            num_edges++;
        }
    }
    qsort(edge_keys, (size_t)num_edges, sizeof(edge_key_t), edge_key_compare);
    unique_edges = 0;
    for (int e = 0; e < num_edges; e++) {
        if (e == 0 || edge_key_compare(&edge_keys[e - 1], &edge_keys[e]) != 0) {
            if (unique_edges != e) edge_keys[unique_edges] = edge_keys[e];
            unique_edges++;
        }
    }

    mesh->num_edges = unique_edges;
    mesh->edges = (mesh_edge_t*)calloc((size_t)mesh->num_edges, sizeof(mesh_edge_t));
    if (!mesh->edges) { free(edge_keys); mesh_destroy(mesh); free(tag2idx); return NULL; }
    for (int e = 0; e < mesh->num_edges; e++) {
        mesh_edge_t* edge = &mesh->edges[e];
        int va = edge_keys[e].vmin;
        int vb = edge_keys[e].vmax;
        edge->id = e;
        edge->vertex1_id = va;
        edge->vertex2_id = vb;
        edge->is_boundary = true;
        edge->is_interface = false;
        geom_point_t* pa = &mesh->vertices[va].position;
        geom_point_t* pb = &mesh->vertices[vb].position;
        edge->midpoint.x = 0.5 * (pa->x + pb->x);
        edge->midpoint.y = 0.5 * (pa->y + pb->y);
        edge->midpoint.z = 0.5 * (pa->z + pb->z);
        double dx = pb->x - pa->x, dy = pb->y - pa->y, dz = pb->z - pa->z;
        edge->length = sqrt(dx * dx + dy * dy + dz * dz);
        if (edge->length > 0.0) {
            edge->tangent.x = dx / edge->length;
            edge->tangent.y = dy / edge->length;
            edge->tangent.z = dz / edge->length;
        }
    }

    mesh->num_elements = (int)elemTags_n;
    mesh->elements = (mesh_element_t*)calloc((size_t)mesh->num_elements, sizeof(mesh_element_t));
    if (!mesh->elements) { free(edge_keys); mesh_destroy(mesh); free(tag2idx); return NULL; }
    for (size_t t = 0; t < elemTags_n; t++) {
        mesh_element_t* elem = &mesh->elements[t];
        size_t n0 = nodeTagsElem[3 * t], n1 = nodeTagsElem[3 * t + 1], n2 = nodeTagsElem[3 * t + 2];
        int i0 = (n0 <= max_node_tag) ? tag2idx[n0] : 0;
        int i1 = (n1 <= max_node_tag) ? tag2idx[n1] : 0;
        int i2 = (n2 <= max_node_tag) ? tag2idx[n2] : 0;
        elem->id = (int)t;
        elem->type = MESH_ELEMENT_TRIANGLE;
        elem->num_vertices = 3;
        elem->num_edges = 3;
        elem->vertices = (int*)malloc(3 * sizeof(int));
        elem->edges = (int*)malloc(3 * sizeof(int));
        if (!elem->vertices || !elem->edges) {
            free(edge_keys);
            mesh_destroy(mesh);
            free(tag2idx);
            return NULL;
        }
        elem->vertices[0] = i0;
        elem->vertices[1] = i1;
        elem->vertices[2] = i2;
        for (int k = 0; k < 3; k++) {
            int a = (k == 0) ? i0 : (k == 1) ? i1 : i2;
            int b = (k == 0) ? i1 : (k == 1) ? i2 : i0;
            int vmin = (a < b) ? a : b;
            int vmax = (a < b) ? b : a;
            int edge_idx = -1;
            for (int e = 0; e < unique_edges; e++) {
                if (edge_keys[e].vmin == vmin && edge_keys[e].vmax == vmax) {
                    edge_idx = e;
                    break;
                }
            }
            elem->edges[k] = edge_idx >= 0 ? edge_idx : 0;
        }
        geom_point_t* p0 = &mesh->vertices[i0].position;
        geom_point_t* p1 = &mesh->vertices[i1].position;
        geom_point_t* p2 = &mesh->vertices[i2].position;
        double ux = p1->x - p0->x, uy = p1->y - p0->y, uz = p1->z - p0->z;
        double vx = p2->x - p0->x, vy = p2->y - p0->y, vz = p2->z - p0->z;
        double nx = uy * vz - uz * vy, ny = uz * vx - ux * vz, nz = ux * vy - uy * vx;
        double nlen = sqrt(nx * nx + ny * ny + nz * nz);
        elem->area = 0.5 * nlen;
        if (nlen > 0.0) {
            elem->normal.x = nx / nlen;
            elem->normal.y = ny / nlen;
            elem->normal.z = nz / nlen;
        } else {
            elem->normal.x = elem->normal.y = elem->normal.z = 0.0;
        }
        elem->centroid.x = (p0->x + p1->x + p2->x) / 3.0;
        elem->centroid.y = (p0->y + p1->y + p2->y) / 3.0;
        elem->centroid.z = (p0->z + p1->z + p2->z) / 3.0;
        elem->material_id = 0;
        elem->region_id = 0;
        elem->domain_id = 0;
        elem->quality_factor = 1.0;
        elem->characteristic_length = sqrt(elem->area);
    }
    mesh->min_bound.x = mesh->min_bound.y = mesh->min_bound.z = 1e30;
    mesh->max_bound.x = mesh->max_bound.y = mesh->max_bound.z = -1e30;
    for (int i = 0; i < mesh->num_vertices; i++) {
        geom_point_t* p = &mesh->vertices[i].position;
        if (p->x < mesh->min_bound.x) mesh->min_bound.x = p->x;
        if (p->y < mesh->min_bound.y) mesh->min_bound.y = p->y;
        if (p->z < mesh->min_bound.z) mesh->min_bound.z = p->z;
        if (p->x > mesh->max_bound.x) mesh->max_bound.x = p->x;
        if (p->y > mesh->max_bound.y) mesh->max_bound.y = p->y;
        if (p->z > mesh->max_bound.z) mesh->max_bound.z = p->z;
    }
    mesh->algorithm = MESH_ALGORITHM_DELAUNAY;
    mesh->quality.min_angle = 30.0;
    mesh->quality.max_angle = 120.0;
    mesh->min_element_size = mesh->max_element_size = mesh->elements[0].characteristic_length;
    for (int t = 1; t < mesh->num_elements; t++) {
        double cl = mesh->elements[t].characteristic_length;
        if (cl < mesh->min_element_size) mesh->min_element_size = cl;
        if (cl > mesh->max_element_size) mesh->max_element_size = cl;
    }
    mesh->average_element_size = 0.5 * (mesh->min_element_size + mesh->max_element_size);
    free(edge_keys);
    free(tag2idx);
    return mesh;
}

/**
 * Import surface mesh from a CAD file (STEP/IGES/STL/etc.) using Gmsh.
 * @param filename Path to CAD file
 * @param characteristic_length Target mesh size (e.g. wavelength / mesh_density)
 * @param out_mesh On success, allocated mesh_t; caller must mesh_destroy()
 * @return 0 on success, -1 on error
 */
int gmsh_import_surface_mesh(const char* filename,
                             double characteristic_length,
                             mesh_t** out_mesh) {
    int ierr = 0;
    size_t* nodeTags = NULL;
    double* coord = NULL;
    size_t nodeTags_n = 0, coord_n = 0;
    size_t* elemTags = NULL;
    size_t* nodeTagsElem = NULL;
    size_t elemTags_n = 0, nodeTagsElem_n = 0;
    mesh_t* mesh = NULL;

    if (!filename || !out_mesh || characteristic_length <= 0.0) {
        return -1;
    }
    *out_mesh = NULL;

    gmshInitialize(0, NULL, 0, 0, &ierr);
    if (ierr != 0) {
        fprintf(stderr, "[Gmsh] Initialize failed\n");
        return -1;
    }

    gmshClear(&ierr);
    gmshMerge(filename, &ierr);
    if (ierr != 0) {
        fprintf(stderr, "[Gmsh] Merge failed for: %s\n", filename);
        gmshFinalize(&ierr);
        return -1;
    }

    /* Options to improve STEP/surface meshing: sew faces and tolerance */
    gmshOptionSetNumber("Geometry.OCCSewFaces", 1.0, &ierr);
    gmshOptionSetNumber("Geometry.Tolerance", 1e-6, &ierr);
    gmshOptionSetNumber("Mesh.CharacteristicLengthMin", characteristic_length, &ierr);
    gmshOptionSetNumber("Mesh.CharacteristicLengthMax", characteristic_length, &ierr);
    gmshOptionSetNumber("General.Verbosity", 1.0, &ierr);

    /* Generate 1D mesh first (edges), then 2D (surfaces); some STEP need this order */
    gmshModelMeshGenerate(1, &ierr);
    if (ierr != 0) {
        ierr = 0;  /* ignore 1D failure, try 2D anyway (e.g. model may have no curves) */
    }
    gmshModelMeshGenerate(2, &ierr);
    if (ierr != 0) {
        int* errDimTags = NULL;
        size_t errDimTags_n = 0;
        gmshModelMeshGetLastEntityError(&errDimTags, &errDimTags_n, &ierr);
        fprintf(stderr, "[Gmsh] MeshGenerate(2) failed (ierr=%d). Tip: STEP may have no 2D surfaces (only wires/points), or try Geometry.OCCSewFaces/Tolerance in Gmsh.\n", ierr);
        if (errDimTags) gmshFree(errDimTags);
        gmshFinalize(&ierr);
        return -1;
    }

    /* Get all nodes and 3-node triangles */
    gmshModelMeshGetNodes(&nodeTags, &nodeTags_n, &coord, &coord_n,
                         NULL, NULL, -1, -1, 0, 0, &ierr);
    if (ierr != 0 || !nodeTags || !coord || nodeTags_n == 0) {
        fprintf(stderr, "[Gmsh] GetNodes failed or empty mesh\n");
        if (nodeTags) gmshFree(nodeTags);
        if (coord) gmshFree(coord);
        gmshFinalize(&ierr);
        return -1;
    }
    gmshModelMeshGetElementsByType(GMSH_ELEMENT_TRIANGLE,
                                  &elemTags, &elemTags_n,
                                  &nodeTagsElem, &nodeTagsElem_n,
                                  -1, 0, 1, &ierr);
    if (ierr != 0 || !nodeTagsElem || elemTags_n == 0) {
        fprintf(stderr, "[Gmsh] No surface triangles (GetElementsByType 2)\n");
        gmshFree(nodeTags);
        gmshFree(coord);
        if (elemTags) gmshFree(elemTags);
        if (nodeTagsElem) gmshFree(nodeTagsElem);
        gmshFinalize(&ierr);
        return -1;
    }

    mesh = build_mesh_from_gmsh_arrays(nodeTags, nodeTags_n, coord, nodeTagsElem, elemTags_n);
    gmshFree(nodeTags);
    gmshFree(coord);
    gmshFree(elemTags);
    gmshFree(nodeTagsElem);
    gmshFinalize(&ierr);
    if (!mesh) return -1;
    *out_mesh = mesh;
    return 0;
}

/**
 * Import surface mesh directly from a Gmsh .msh file (no CAD, no mesh generation).
 * Use this when you have already meshed in Gmsh GUI and saved as .msh.
 * @param filename Path to .msh file (must contain 2D triangle elements)
 * @param out_mesh On success, allocated mesh_t; caller must mesh_destroy()
 * @return 0 on success, -1 on error
 */
int gmsh_import_msh(const char* filename, mesh_t** out_mesh) {
    int ierr = 0;
    size_t* nodeTags = NULL;
    double* coord = NULL;
    size_t nodeTags_n = 0, coord_n = 0;
    size_t* elemTags = NULL;
    size_t* nodeTagsElem = NULL;
    size_t elemTags_n = 0, nodeTagsElem_n = 0;
    mesh_t* mesh = NULL;

    if (!filename || !out_mesh) return -1;
    *out_mesh = NULL;

    gmshInitialize(0, NULL, 0, 0, &ierr);
    if (ierr != 0) {
        fprintf(stderr, "[Gmsh] Initialize failed\n");
        return -1;
    }

    /* Open .msh file (loads mesh directly, no generation) */
    gmshOpen(filename, &ierr);
    if (ierr != 0) {
        fprintf(stderr, "[Gmsh] Open failed for: %s\n", filename);
        gmshFinalize(&ierr);
        return -1;
    }

    gmshModelMeshGetNodes(&nodeTags, &nodeTags_n, &coord, &coord_n,
                         NULL, NULL, -1, -1, 0, 0, &ierr);
    if (ierr != 0 || !nodeTags || !coord || nodeTags_n == 0) {
        fprintf(stderr, "[Gmsh] GetNodes failed or empty mesh in .msh\n");
        if (nodeTags) gmshFree(nodeTags);
        if (coord) gmshFree(coord);
        gmshFinalize(&ierr);
        return -1;
    }
    gmshModelMeshGetElementsByType(GMSH_ELEMENT_TRIANGLE,
                                  &elemTags, &elemTags_n,
                                  &nodeTagsElem, &nodeTagsElem_n,
                                  -1, 0, 1, &ierr);
    if (ierr != 0 || !nodeTagsElem || elemTags_n == 0) {
        fprintf(stderr, "[Gmsh] No 2D triangles in .msh (ensure mesh contains surface triangles)\n");
        gmshFree(nodeTags);
        gmshFree(coord);
        if (elemTags) gmshFree(elemTags);
        if (nodeTagsElem) gmshFree(nodeTagsElem);
        gmshFinalize(&ierr);
        return -1;
    }

    mesh = build_mesh_from_gmsh_arrays(nodeTags, nodeTags_n, coord, nodeTagsElem, elemTags_n);
    gmshFree(nodeTags);
    gmshFree(coord);
    gmshFree(elemTags);
    gmshFree(nodeTagsElem);
    gmshFinalize(&ierr);
    if (!mesh) return -1;
    *out_mesh = mesh;
    fprintf(stderr, "[MoM] Loaded .msh: %d vertices, %d triangles\n", mesh->num_vertices, mesh->num_elements);
    return 0;
}
