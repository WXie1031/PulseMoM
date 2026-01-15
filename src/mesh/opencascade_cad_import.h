#ifndef OPENCASCADE_CAD_IMPORT_H
#define OPENCASCADE_CAD_IMPORT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

typedef struct {
    double healing_precision;
    double max_tolerance;
    int heal_geometry;
} opencascade_import_params_t;

typedef struct {
    int num_faces;
    int num_edges;
    int num_vertices;
    int num_solids;
    int num_shells;
    int num_wires;
    int num_surfaces;
    int is_closed;
    double bounding_box[6];
    double surface_area;
    double volume;
    double unit_scale;
    void* surfaces;
    void* occt_shape;
} opencascade_geometry_t;

bool opencascade_import_cad(const char* filename,
                            opencascade_import_params_t* params,
                            opencascade_geometry_t* geometry);

#ifdef __cplusplus
}
#endif

#endif /* OPENCASCADE_CAD_IMPORT_H */
