/******************************************************************************
 * MTL Solver Parameter Matrix Import - Implementation
 ******************************************************************************/

#include "mtl_parameter_import.h"
#include "../../physics/mtl/mtl_physics.h"  // Use standard L1 physics definitions
#include "mtl_solver_module.h"  // For solver-specific types
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

// Static storage for parameter matrices per solver
// Since solver is opaque, we use a simple association mechanism
#define MAX_STORED_SOLVERS 16
static struct {
    mtl_solver_t* solver;
    mtl_parameter_matrices_t matrices;
    bool is_valid;
} parameter_storage[MAX_STORED_SOLVERS];
static int num_stored_solvers = 0;

int mtl_solver_import_parameter_matrices(
    mtl_solver_t* solver,
    const mtl_parameter_matrices_t* matrices
) {
    if (!solver || !matrices) {
        return -1;
    }
    
    if (matrices->num_frequencies < 1 || matrices->num_conductors < 1) {
        return -1;
    }
    
    // Store parameter matrices in static storage
    // Find existing entry or create new one
    int storage_idx = -1;
    for (int i = 0; i < num_stored_solvers; i++) {
        if (parameter_storage[i].solver == solver) {
            storage_idx = i;
            break;
        }
    }
    
    if (storage_idx < 0) {
        // Create new entry
        if (num_stored_solvers >= MAX_STORED_SOLVERS) {
            return -1;  // Storage full
        }
        storage_idx = num_stored_solvers++;
        parameter_storage[storage_idx].solver = solver;
    }
    
    // Free existing matrices if any
    if (parameter_storage[storage_idx].is_valid) {
        mtl_parameter_matrices_free(&parameter_storage[storage_idx].matrices);
    }
    
    // Copy matrices structure
    mtl_parameter_matrices_t* stored = &parameter_storage[storage_idx].matrices;
    stored->num_frequencies = matrices->num_frequencies;
    stored->num_conductors = matrices->num_conductors;
    stored->interpolate = matrices->interpolate;
    
    // Allocate and copy frequencies
    stored->frequencies = (double*)calloc(matrices->num_frequencies, sizeof(double));
    if (!stored->frequencies) {
        return -1;
    }
    memcpy(stored->frequencies, matrices->frequencies, matrices->num_frequencies * sizeof(double));
    
    // Allocate and copy matrices
    int matrix_size = matrices->num_conductors * matrices->num_conductors;
    stored->R_matrix = (double**)calloc(matrices->num_frequencies, sizeof(double*));
    stored->L_matrix = (double**)calloc(matrices->num_frequencies, sizeof(double*));
    stored->C_matrix = (double**)calloc(matrices->num_frequencies, sizeof(double*));
    stored->G_matrix = (double**)calloc(matrices->num_frequencies, sizeof(double*));
    
    if (!stored->R_matrix || !stored->L_matrix || !stored->C_matrix || !stored->G_matrix) {
        mtl_parameter_matrices_free(stored);
        return -1;
    }
    
    for (int f = 0; f < matrices->num_frequencies; f++) {
        stored->R_matrix[f] = (double*)calloc(matrix_size, sizeof(double));
        stored->L_matrix[f] = (double*)calloc(matrix_size, sizeof(double));
        stored->C_matrix[f] = (double*)calloc(matrix_size, sizeof(double));
        stored->G_matrix[f] = (double*)calloc(matrix_size, sizeof(double));
        
        if (!stored->R_matrix[f] || !stored->L_matrix[f] || 
            !stored->C_matrix[f] || !stored->G_matrix[f]) {
            mtl_parameter_matrices_free(stored);
            return -1;
        }
        
        memcpy(stored->R_matrix[f], matrices->R_matrix[f], matrix_size * sizeof(double));
        memcpy(stored->L_matrix[f], matrices->L_matrix[f], matrix_size * sizeof(double));
        memcpy(stored->C_matrix[f], matrices->C_matrix[f], matrix_size * sizeof(double));
        memcpy(stored->G_matrix[f], matrices->G_matrix[f], matrix_size * sizeof(double));
    }
    
    parameter_storage[storage_idx].is_valid = true;
    
    return 0;
}

int mtl_solver_import_parameter_matrices_from_file(
    mtl_solver_t* solver,
    const char* filename,
    const char* format
) {
    if (!solver || !filename || !format) {
        return -1;
    }
    
    mtl_parameter_matrices_t matrices = {0};
    
    // Parse file based on format
    if (strcmp(format, "CSV") == 0) {
        // Parse CSV file
        FILE* fp = fopen(filename, "r");
        if (!fp) {
            return -1;
        }
        
        // Read header to determine format
        char line[4096];
        if (!fgets(line, sizeof(line), fp)) {
            fclose(fp);
            return -1;
        }
        
        // Count number of frequencies (estimate from file size or read all lines)
        int line_count = 1;  // Header already read
        while (fgets(line, sizeof(line), fp)) {
            line_count++;
        }
        rewind(fp);
        fgets(line, sizeof(line), fp);  // Skip header again
        
        // Allocate frequency array
        matrices.num_frequencies = line_count - 1;  // Exclude header
        matrices.frequencies = (double*)calloc(matrices.num_frequencies, sizeof(double));
        if (!matrices.frequencies) {
            fclose(fp);
            return -1;
        }
        
        // Determine number of conductors from header or first data line
        // Format: freq, R11, R12, ..., L11, L12, ..., C11, C12, ..., G11, G12, ...
        // For N conductors, we have N*N elements for each matrix type
        // Total columns = 1 (freq) + 4*N*N (R, L, C, G)
        int num_cols = 0;
        char* token = strtok(line, ",\n");
        while (token) {
            num_cols++;
            token = strtok(NULL, ",\n");
        }
        
        // num_cols = 1 + 4*N*N, so N = sqrt((num_cols-1)/4)
        if (num_cols > 1) {
            matrices.num_conductors = (int)sqrt((num_cols - 1) / 4.0);
        } else {
            matrices.num_conductors = 1;  // Default
        }
        
        if (matrices.num_conductors < 1) {
            free(matrices.frequencies);
            fclose(fp);
            return -1;
        }
        
        int matrix_size = matrices.num_conductors * matrices.num_conductors;
        
        // Allocate matrices
        matrices.R_matrix = (double**)calloc(matrices.num_frequencies, sizeof(double*));
        matrices.L_matrix = (double**)calloc(matrices.num_frequencies, sizeof(double*));
        matrices.C_matrix = (double**)calloc(matrices.num_frequencies, sizeof(double*));
        matrices.G_matrix = (double**)calloc(matrices.num_frequencies, sizeof(double*));
        
        if (!matrices.R_matrix || !matrices.L_matrix || !matrices.C_matrix || !matrices.G_matrix) {
            free(matrices.frequencies);
            fclose(fp);
            return -1;
        }
        
        // Read data lines
        for (int f = 0; f < matrices.num_frequencies; f++) {
            if (!fgets(line, sizeof(line), fp)) {
                break;
            }
            
            // Allocate matrix storage for this frequency
            matrices.R_matrix[f] = (double*)calloc(matrix_size, sizeof(double));
            matrices.L_matrix[f] = (double*)calloc(matrix_size, sizeof(double));
            matrices.C_matrix[f] = (double*)calloc(matrix_size, sizeof(double));
            matrices.G_matrix[f] = (double*)calloc(matrix_size, sizeof(double));
            
            if (!matrices.R_matrix[f] || !matrices.L_matrix[f] || 
                !matrices.C_matrix[f] || !matrices.G_matrix[f]) {
                // Cleanup on error
                for (int i = 0; i < f; i++) {
                    if (matrices.R_matrix[i]) free(matrices.R_matrix[i]);
                    if (matrices.L_matrix[i]) free(matrices.L_matrix[i]);
                    if (matrices.C_matrix[i]) free(matrices.C_matrix[i]);
                    if (matrices.G_matrix[i]) free(matrices.G_matrix[i]);
                }
                free(matrices.frequencies);
                free(matrices.R_matrix);
                free(matrices.L_matrix);
                free(matrices.C_matrix);
                free(matrices.G_matrix);
                fclose(fp);
                return -1;
            }
            
            // Parse CSV line: freq, R11, R12, ..., L11, L12, ..., C11, C12, ..., G11, G12, ...
            token = strtok(line, ",\n");
            if (token) {
                matrices.frequencies[f] = atof(token);
            }
            
            // Read R matrix
            for (int i = 0; i < matrix_size; i++) {
                token = strtok(NULL, ",\n");
                if (token) {
                    matrices.R_matrix[f][i] = atof(token);
                }
            }
            
            // Read L matrix
            for (int i = 0; i < matrix_size; i++) {
                token = strtok(NULL, ",\n");
                if (token) {
                    matrices.L_matrix[f][i] = atof(token);
                }
            }
            
            // Read C matrix
            for (int i = 0; i < matrix_size; i++) {
                token = strtok(NULL, ",\n");
                if (token) {
                    matrices.C_matrix[f][i] = atof(token);
                }
            }
            
            // Read G matrix
            for (int i = 0; i < matrix_size; i++) {
                token = strtok(NULL, ",\n");
                if (token) {
                    matrices.G_matrix[f][i] = atof(token);
                }
            }
        }
        
        fclose(fp);
    } else if (strcmp(format, "HDF5") == 0) {
        // Parse HDF5 file
        // Implementation would use HDF5 library
    } else if (strcmp(format, "MATLAB") == 0) {
        // Parse MATLAB file
        // Implementation would use MATLAB file format library
    } else {
        return -1;  // Unsupported format
    }
    
    // Import matrices
    return mtl_solver_import_parameter_matrices(solver, &matrices);
}

int mtl_solver_get_parameter_matrices_at_frequency(
    mtl_solver_t* solver,
    double frequency,
    double* R,
    double* L,
    double* C,
    double* G
) {
    if (!solver || !R || !L || !C || !G) {
        return -1;
    }
    
    // Get parameter matrices at specific frequency from static storage
    // Find stored matrices for this solver
    mtl_parameter_matrices_t* matrices = NULL;
    for (int i = 0; i < num_stored_solvers; i++) {
        if (parameter_storage[i].solver == solver && parameter_storage[i].is_valid) {
            matrices = &parameter_storage[i].matrices;
            break;
        }
    }
    
    if (!matrices || matrices->num_frequencies == 0) {
        return -1;  // No matrices imported
    }
    
    int num_conductors = matrices->num_conductors;
    int matrix_size = num_conductors * num_conductors;
    
    // Find frequency index or interpolate
    int freq_idx = -1;
    for (int i = 0; i < matrices->num_frequencies; i++) {
        if (fabs(matrices->frequencies[i] - frequency) < 1e-6) {
            freq_idx = i;
            break;
        }
    }
    
    if (freq_idx >= 0) {
        // Exact match - copy matrices
        memcpy(R, matrices->R_matrix[freq_idx], matrix_size * sizeof(double));
        memcpy(L, matrices->L_matrix[freq_idx], matrix_size * sizeof(double));
        memcpy(C, matrices->C_matrix[freq_idx], matrix_size * sizeof(double));
        memcpy(G, matrices->G_matrix[freq_idx], matrix_size * sizeof(double));
        return 0;
    }
    
    // Interpolate between frequencies
    if (matrices->num_frequencies > 1 && matrices->interpolate) {
        // Find bounding frequencies
        int lower_idx = 0;
        int upper_idx = matrices->num_frequencies - 1;
        
        // Check if frequency is outside range
        if (frequency < matrices->frequencies[0]) {
            // Extrapolate below - use first two points
            lower_idx = 0;
            upper_idx = (matrices->num_frequencies > 1) ? 1 : 0;
        } else if (frequency > matrices->frequencies[matrices->num_frequencies - 1]) {
            // Extrapolate above - use last two points
            lower_idx = matrices->num_frequencies - 2;
            upper_idx = matrices->num_frequencies - 1;
        } else {
            // Interpolate - find bounding frequencies
            for (int i = 0; i < matrices->num_frequencies - 1; i++) {
                if (frequency >= matrices->frequencies[i] && 
                    frequency <= matrices->frequencies[i + 1]) {
                    lower_idx = i;
                    upper_idx = i + 1;
                    break;
                }
            }
        }
        
        // Linear interpolation
        double f_lower = matrices->frequencies[lower_idx];
        double f_upper = matrices->frequencies[upper_idx];
        double alpha = 0.0;
        
        if (fabs(f_upper - f_lower) > 1e-12) {
            alpha = (frequency - f_lower) / (f_upper - f_lower);
        }
        
        // Clamp alpha for extrapolation
        if (alpha < 0.0) alpha = 0.0;
        if (alpha > 1.0) alpha = 1.0;
        
        for (int i = 0; i < matrix_size; i++) {
            R[i] = (1.0 - alpha) * matrices->R_matrix[lower_idx][i] + 
                   alpha * matrices->R_matrix[upper_idx][i];
            L[i] = (1.0 - alpha) * matrices->L_matrix[lower_idx][i] + 
                   alpha * matrices->L_matrix[upper_idx][i];
            C[i] = (1.0 - alpha) * matrices->C_matrix[lower_idx][i] + 
                   alpha * matrices->C_matrix[upper_idx][i];
            G[i] = (1.0 - alpha) * matrices->G_matrix[lower_idx][i] + 
                   alpha * matrices->G_matrix[upper_idx][i];
        }
        return 0;
    }
    
    // No interpolation and no exact match
    return -1;
}

void mtl_parameter_matrices_free(mtl_parameter_matrices_t* matrices) {
    if (!matrices) {
        return;
    }
    
    if (matrices->frequencies) {
        free(matrices->frequencies);
        matrices->frequencies = NULL;
    }
    
    if (matrices->R_matrix) {
        for (int i = 0; i < matrices->num_frequencies; i++) {
            if (matrices->R_matrix[i]) {
                free(matrices->R_matrix[i]);
            }
        }
        free(matrices->R_matrix);
        matrices->R_matrix = NULL;
    }
    
    // Similar for L, C, G matrices
    if (matrices->L_matrix) {
        for (int i = 0; i < matrices->num_frequencies; i++) {
            if (matrices->L_matrix[i]) {
                free(matrices->L_matrix[i]);
            }
        }
        free(matrices->L_matrix);
        matrices->L_matrix = NULL;
    }
    
    if (matrices->C_matrix) {
        for (int i = 0; i < matrices->num_frequencies; i++) {
            if (matrices->C_matrix[i]) {
                free(matrices->C_matrix[i]);
            }
        }
        free(matrices->C_matrix);
        matrices->C_matrix = NULL;
    }
    
    if (matrices->G_matrix) {
        for (int i = 0; i < matrices->num_frequencies; i++) {
            if (matrices->G_matrix[i]) {
                free(matrices->G_matrix[i]);
            }
        }
        free(matrices->G_matrix);
        matrices->G_matrix = NULL;
    }
    
    matrices->num_frequencies = 0;
    matrices->num_conductors = 0;
}
