/**
 * @file gmsh_mesh_import.h
 * @brief Import surface triangular mesh from CAD using Gmsh C API.
 */

#ifndef GMSH_MESH_IMPORT_H
#define GMSH_MESH_IMPORT_H

#ifdef __cplusplus
extern "C" {
#endif

struct mesh_t;

/**
 * Import surface mesh from CAD file (STEP/IGES/STL etc.) via Gmsh.
 * @param filename Path to CAD file
 * @param characteristic_length Target mesh size (e.g. wavelength / mesh_density)
 * @param out_mesh On success, allocated mesh_t; caller must mesh_destroy()
 * @return 0 on success, -1 on error
 */
int gmsh_import_surface_mesh(const char* filename,
                             double characteristic_length,
                             struct mesh_t** out_mesh);

/**
 * Import surface mesh directly from a Gmsh .msh file (no CAD step).
 * Use when you have already meshed in Gmsh and saved as .msh.
 * @param filename Path to .msh file (must contain 2D triangle elements)
 * @param out_mesh On success, allocated mesh_t; caller must mesh_destroy()
 * @return 0 on success, -1 on error
 */
int gmsh_import_msh(const char* filename, struct mesh_t** out_mesh);

#ifdef __cplusplus
}
#endif

#endif /* GMSH_MESH_IMPORT_H */
