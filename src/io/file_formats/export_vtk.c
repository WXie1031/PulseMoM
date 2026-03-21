/******************************************************************************
 * VTK Export - Implementation
 ******************************************************************************/

#include "export_vtk.h"
#include "../io/postprocessing/field_postprocessing.h"
#include "../../discretization/mesh/core_mesh.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// VTK element type mapping
#define VTK_VERTEX 1
#define VTK_LINE 3
#define VTK_TRIANGLE 5
#define VTK_QUAD 9
#define VTK_TETRA 10
#define VTK_HEXAHEDRON 12

export_vtk_options_t export_vtk_get_default_options(void) {
    export_vtk_options_t opts = {0};
    opts.format = EXPORT_VTK_FORMAT_ASCII;
    opts.include_vectors = true;
    opts.include_scalars = true;
    opts.include_tensors = false;
    opts.use_compression = false;
    opts.precision = 6;
    return opts;
}

int export_vtk_field(
    const postprocessing_field_data_t* field_data,
    const mesh_t* mesh,
    const char* filename,
    const export_vtk_options_t* options
) {
    if (!field_data || !mesh || !filename) {
        return -1;
    }
    
    export_vtk_options_t opts = options ? *options : export_vtk_get_default_options();
    
    FILE* fp = fopen(filename, "w");
    if (!fp) {
        return -1;
    }
    
    // Write VTK header
    if (opts.format == EXPORT_VTK_FORMAT_XML) {
        fprintf(fp, "<?xml version=\"1.0\"?>\n");
        fprintf(fp, "<VTKFile type=\"UnstructuredGrid\" version=\"0.1\">\n");
        fprintf(fp, "  <UnstructuredGrid>\n");
    } else {
        fprintf(fp, "# vtk DataFile Version 3.0\n");
        fprintf(fp, "PulseMoM Field Data\n");
        fprintf(fp, "%s\n", opts.format == EXPORT_VTK_FORMAT_ASCII ? "ASCII" : "BINARY");
        fprintf(fp, "DATASET UNSTRUCTURED_GRID\n");
    }
    
    // Write points
    fprintf(fp, "POINTS %d float\n", field_data->num_points);
    for (int i = 0; i < field_data->num_points; i++) {
        fprintf(fp, "%.6e %.6e %.6e\n",
                field_data->positions[i * 3],
                field_data->positions[i * 3 + 1],
                field_data->positions[i * 3 + 2]);
    }
    
    // Write field data
    if (opts.include_vectors) {
        fprintf(fp, "POINT_DATA %d\n", field_data->num_points);
        fprintf(fp, "VECTORS E_field float\n");
        for (int i = 0; i < field_data->num_points; i++) {
            fprintf(fp, "%.6e %.6e %.6e\n",
                    field_data->e_field_real[i * 3],
                    field_data->e_field_real[i * 3 + 1],
                    field_data->e_field_real[i * 3 + 2]);
        }
        
        fprintf(fp, "VECTORS H_field float\n");
        for (int i = 0; i < field_data->num_points; i++) {
            fprintf(fp, "%.6e %.6e %.6e\n",
                    field_data->h_field_real[i * 3],
                    field_data->h_field_real[i * 3 + 1],
                    field_data->h_field_real[i * 3 + 2]);
        }
    }
    
    if (opts.include_scalars) {
        fprintf(fp, "SCALARS E_magnitude float 1\n");
        fprintf(fp, "LOOKUP_TABLE default\n");
        for (int i = 0; i < field_data->num_points; i++) {
            double Ex = field_data->e_field_real[i * 3];
            double Ey = field_data->e_field_real[i * 3 + 1];
            double Ez = field_data->e_field_real[i * 3 + 2];
            double mag = sqrt(Ex*Ex + Ey*Ey + Ez*Ez);
            fprintf(fp, "%.6e\n", mag);
        }
    }
    
    if (opts.format == EXPORT_VTK_FORMAT_XML) {
        fprintf(fp, "  </UnstructuredGrid>\n");
        fprintf(fp, "</VTKFile>\n");
    }
    
    fclose(fp);
    return 0;
}

int export_vtk_current(
    const postprocessing_current_distribution_t* current_dist,
    const mesh_t* mesh,
    const char* filename,
    const export_vtk_options_t* options
) {
    if (!current_dist || !mesh || !filename) {
        return -1;
    }
    
    export_vtk_options_t opts = options ? *options : export_vtk_get_default_options();
    
    FILE* fp = fopen(filename, "w");
    if (!fp) {
        return -1;
    }
    
    // Write VTK header
    fprintf(fp, "# vtk DataFile Version 3.0\n");
    fprintf(fp, "PulseMoM Current Distribution\n");
    fprintf(fp, "%s\n", opts.format == EXPORT_VTK_FORMAT_ASCII ? "ASCII" : "BINARY");
    fprintf(fp, "DATASET UNSTRUCTURED_GRID\n");
    
    // Write mesh points
    if (!mesh || mesh->num_vertices <= 0) {
        fclose(fp);
        return -1;
    }
    
    fprintf(fp, "POINTS %d float\n", mesh->num_vertices);
    for (int i = 0; i < mesh->num_vertices; i++) {
        fprintf(fp, "%.6e %.6e %.6e\n",
                mesh->vertices[i].position.x,
                mesh->vertices[i].position.y,
                mesh->vertices[i].position.z);
    }
    
    /* CELLS and CELL_TYPES so ParaView displays a continuous surface (triangles)
     * with per-cell scalar; interpolation is done by VTK for smooth shading. */
    if (mesh->num_elements <= 0 || !mesh->elements) {
        fclose(fp);
        return -1;
    }
    int total_size = 0;
    for (int i = 0; i < mesh->num_elements; i++) {
        if (mesh->elements[i].vertices && mesh->elements[i].num_vertices >= 3)
            total_size += mesh->elements[i].num_vertices + 1;
    }
    fprintf(fp, "CELLS %d %d\n", mesh->num_elements, total_size);
    for (int i = 0; i < mesh->num_elements; i++) {
        const mesh_element_t* e = &mesh->elements[i];
        if (!e->vertices || e->num_vertices < 3) {
            fprintf(fp, "1 0\n");  /* fallback: point at vertex 0 */
            continue;
        }
        fprintf(fp, "%d", e->num_vertices);
        for (int j = 0; j < e->num_vertices; j++)
            fprintf(fp, " %d", e->vertices[j]);
        fprintf(fp, "\n");
    }
    fprintf(fp, "CELL_TYPES %d\n", mesh->num_elements);
    for (int i = 0; i < mesh->num_elements; i++) {
        int vtk_type = VTK_TRIANGLE;
        if (mesh->elements[i].type == MESH_ELEMENT_QUADRILATERAL && mesh->elements[i].num_vertices >= 4)
            vtk_type = VTK_QUAD;
        else if (mesh->elements[i].num_vertices >= 3)
            vtk_type = VTK_TRIANGLE;
        else
            vtk_type = VTK_VERTEX;
        fprintf(fp, "%d\n", vtk_type);
    }
    
    /* Prefer POINT_DATA when available: vertex-wise magnitude → ParaView interpolates within cells → 单元间连续平滑 */
    if (opts.include_scalars && current_dist->current_magnitude_vertices != NULL &&
        current_dist->num_vertices == mesh->num_vertices) {
        fprintf(fp, "POINT_DATA %d\n", mesh->num_vertices);
        fprintf(fp, "SCALARS current_magnitude float 1\n");
        fprintf(fp, "LOOKUP_TABLE default\n");
        for (int i = 0; i < mesh->num_vertices; i++) {
            fprintf(fp, "%.6e\n", current_dist->current_magnitude_vertices[i]);
        }
    }
    /* CELL_DATA: per-element magnitude (optional; written when no vertex data or for compatibility) */
    fprintf(fp, "CELL_DATA %d\n", current_dist->num_elements);
    if (opts.include_scalars && current_dist->current_magnitude != NULL) {
        fprintf(fp, "SCALARS %s float 1\n",
                current_dist->current_magnitude_vertices != NULL ? "current_magnitude_cell" : "current_magnitude");
        fprintf(fp, "LOOKUP_TABLE default\n");
        for (int i = 0; i < current_dist->num_elements; i++) {
            fprintf(fp, "%.6e\n", current_dist->current_magnitude[i]);
        }
    }
    
    fclose(fp);
    return 0;
}

int export_vtk_mesh(
    const mesh_t* mesh,
    const double** attributes,
    int num_attributes,
    const char** attribute_names,
    const char* filename,
    const export_vtk_options_t* options
) {
    if (!mesh || !filename) {
        return -1;
    }
    
    export_vtk_options_t opts = options ? *options : export_vtk_get_default_options();
    
    FILE* fp = fopen(filename, "w");
    if (!fp) {
        return -1;
    }
    
    // Write VTK header
    fprintf(fp, "# vtk DataFile Version 3.0\n");
    fprintf(fp, "PulseMoM Mesh Data\n");
    fprintf(fp, "%s\n", opts.format == EXPORT_VTK_FORMAT_ASCII ? "ASCII" : "BINARY");
    fprintf(fp, "DATASET UNSTRUCTURED_GRID\n");
    
    // Write points
    fprintf(fp, "POINTS %d float\n", mesh->num_vertices);
    for (int i = 0; i < mesh->num_vertices; i++) {
        fprintf(fp, "%.6e %.6e %.6e\n",
                mesh->vertices[i].position.x,
                mesh->vertices[i].position.y,
                mesh->vertices[i].position.z);
    }
    
    // Write cells
    if (!mesh || mesh->num_elements <= 0) {
        fclose(fp);
        return -1;
    }
    
    int total_cells = mesh->num_elements;
    int total_size = 0;
    for (int i = 0; i < mesh->num_elements; i++) {
        if (mesh->elements[i].vertices) {
            total_size += mesh->elements[i].num_vertices + 1;
        }
    }
    
    fprintf(fp, "CELLS %d %d\n", total_cells, total_size);
    for (int i = 0; i < mesh->num_elements; i++) {
        if (mesh->elements[i].vertices) {
            fprintf(fp, "%d", mesh->elements[i].num_vertices);
            for (int j = 0; j < mesh->elements[i].num_vertices; j++) {
                fprintf(fp, " %d", mesh->elements[i].vertices[j]);
            }
            fprintf(fp, "\n");
        }
    }
    
    // Write cell types
    fprintf(fp, "CELL_TYPES %d\n", total_cells);
    for (int i = 0; i < mesh->num_elements; i++) {
        int vtk_type = 1;  // Default: vertex
        switch (mesh->elements[i].type) {
            case MESH_ELEMENT_TRIANGLE: vtk_type = 5; break;
            case MESH_ELEMENT_QUADRILATERAL: vtk_type = 9; break;
            case MESH_ELEMENT_TETRAHEDRON: vtk_type = 10; break;
            case MESH_ELEMENT_HEXAHEDRON: vtk_type = 12; break;
            default: vtk_type = 1; break;
        }
        fprintf(fp, "%d\n", vtk_type);
    }
    
    // Write attributes if provided
    if (attributes && num_attributes > 0 && attribute_names) {
        fprintf(fp, "CELL_DATA %d\n", total_cells);
        for (int a = 0; a < num_attributes; a++) {
            if (attribute_names[a] && attributes[a]) {
                fprintf(fp, "SCALARS %s float 1\n", attribute_names[a]);
                fprintf(fp, "LOOKUP_TABLE default\n");
                for (int i = 0; i < total_cells; i++) {
                    fprintf(fp, "%.6e\n", attributes[a][i]);
                }
            }
        }
    }
    
    fclose(fp);
    return 0;
}

int export_vtk_timeseries(
    const void* time_data,
    const mesh_t* mesh,
    const char* filename_base,
    const export_vtk_options_t* options
) {
    if (!time_data || !mesh || !filename_base) {
        return -1;
    }
    
    // Export time series as multiple VTK files
    // Each time step is exported as a separate VTK file with appended time step number
    
    // Determine number of time steps from time_data structure
    // Note: time_data structure format depends on the solver type
    // For now, assume it contains num_time_steps and time_points array
    
    typedef struct {
        int num_time_steps;
        double* time_points;
        double** field_data;  // [num_time_steps][num_points * 3] for vector fields
        int num_points;
    } time_series_data_t;
    
    time_series_data_t* ts_data = (time_series_data_t*)time_data;
    
    if (!ts_data || ts_data->num_time_steps <= 0) {
        return -1;
    }
    
    // Create base filename with extension
    char base_filename[512];
    strncpy(base_filename, filename_base, sizeof(base_filename) - 1);
    base_filename[sizeof(base_filename) - 1] = '\0';
    
    // Remove extension if present
    char* ext = strrchr(base_filename, '.');
    if (ext) {
        *ext = '\0';
    }
    
    // Export each time step
    for (int t = 0; t < ts_data->num_time_steps; t++) {
        char step_filename[512];
        snprintf(step_filename, sizeof(step_filename), "%s_t%05d.vtk", base_filename, t);
        
        // Create field data structure for this time step
        postprocessing_field_data_t step_field = {0};
        step_field.num_points = ts_data->num_points;
        step_field.frequency = 0.0;  // Time domain, no frequency
        
        // Allocate field arrays
        step_field.positions = (double*)calloc(ts_data->num_points * 3, sizeof(double));
        step_field.e_field_real = (double*)calloc(ts_data->num_points * 3, sizeof(double));
        step_field.e_field_imag = NULL;  // Time domain is real
        step_field.h_field_real = (double*)calloc(ts_data->num_points * 3, sizeof(double));
        step_field.h_field_imag = NULL;
        
        if (!step_field.positions || !step_field.e_field_real || !step_field.h_field_real) {
            if (step_field.positions) free(step_field.positions);
            if (step_field.e_field_real) free(step_field.e_field_real);
            if (step_field.h_field_real) free(step_field.h_field_real);
            return -1;
        }
        
        // Copy positions from mesh (if available)
        if (mesh && mesh->num_vertices > 0) {
            for (int i = 0; i < ts_data->num_points && i < mesh->num_vertices; i++) {
                step_field.positions[i * 3] = mesh->vertices[i].position.x;
                step_field.positions[i * 3 + 1] = mesh->vertices[i].position.y;
                step_field.positions[i * 3 + 2] = mesh->vertices[i].position.z;
            }
        }
        
        // Copy field data for this time step
        if (ts_data->field_data && ts_data->field_data[t]) {
            for (int i = 0; i < ts_data->num_points * 3; i++) {
                step_field.e_field_real[i] = ts_data->field_data[t][i];
                // For time domain, H field might be in separate array or computed
                // For now, set to zero
                step_field.h_field_real[i] = 0.0;
            }
        }
        
        // Export this time step
        int result = export_vtk_field(&step_field, mesh, step_filename, options);
        
        // Cleanup
        free(step_field.positions);
        free(step_field.e_field_real);
        free(step_field.h_field_real);
        
        if (result != 0) {
            return result;
        }
    }
    
    return 0;
}
