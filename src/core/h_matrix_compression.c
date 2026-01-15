#include "h_matrix_compression.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#ifdef _OPENMP
#include <omp.h>
#endif

#define MAX_RANK 100
#define ACA_TOLERANCE 1e-6
#define GMRES_RESTART 30
#define BICGSTAB_MAX_ITER 1000

// Helper function: Euclidean distance
double euclidean_distance(const double *p1, const double *p2) {
    double dx = p1[0] - p2[0];
    double dy = p1[1] - p2[1];
    double dz = p1[2] - p2[2];
    return sqrt(dx*dx + dy*dy + dz*dz);
}

// Helper function: Check admissibility condition
bool is_admissible(const ClusterNode *cluster1, const ClusterNode *cluster2, 
                  int eta) {
    double distance = euclidean_distance(cluster1->centroid, cluster2->centroid);
    double min_diameter = fmin(2.0 * cluster1->radius, 2.0 * cluster2->radius);
    
    return distance >= eta * min_diameter;
}

// Cluster tree construction
ClusterTree* build_cluster_tree(
    const double *coordinates,
    int n_points,
    int max_depth,
    int min_cluster_size) {
    
    ClusterTree *tree = (ClusterTree*)malloc(sizeof(ClusterTree));
    tree->max_depth = max_depth;
    tree->n_clusters = 0;
    
    // Create root cluster
    tree->root = (ClusterNode*)malloc(sizeof(ClusterNode));
    tree->root->indices = (int*)malloc(n_points * sizeof(int));
    tree->root->n_indices = n_points;
    tree->root->centroid = (double*)malloc(3 * sizeof(double));
    tree->root->level = 0;
    tree->root->left = tree->root->right = NULL;
    
    // Initialize indices and calculate centroid
    for (int i = 0; i < n_points; i++) {
        tree->root->indices[i] = i;
        tree->root->centroid[0] += coordinates[3*i];
        tree->root->centroid[1] += coordinates[3*i + 1];
        tree->root->centroid[2] += coordinates[3*i + 2];
    }
    
    tree->root->centroid[0] /= n_points;
    tree->root->centroid[1] /= n_points;
    tree->root->centroid[2] /= n_points;
    
    // Calculate radius
    tree->root->radius = 0.0;
    for (int i = 0; i < n_points; i++) {
        double point[3] = {coordinates[3*i], coordinates[3*i + 1], coordinates[3*i + 2]};
        double dist = euclidean_distance(tree->root->centroid, point);
        tree->root->radius = fmax(tree->root->radius, dist);
    }
    
    tree->n_clusters = 1;
    
    // Recursive bisection
    void bisect_cluster(ClusterNode *node, const double *coords, int max_depth, int min_size) {
        if (node->level >= max_depth || node->n_indices <= min_size) {
            return;
        }
        
        // Find principal axis for splitting
        double covariance[3][3] = {{0}};
        for (int i = 0; i < node->n_indices; i++) {
            int idx = node->indices[i];
            double dx = coords[3*idx] - node->centroid[0];
            double dy = coords[3*idx + 1] - node->centroid[1];
            double dz = coords[3*idx + 2] - node->centroid[2];
            
            covariance[0][0] += dx*dx;
            covariance[0][1] += dx*dy;
            covariance[0][2] += dx*dz;
            covariance[1][1] += dy*dy;
            covariance[1][2] += dy*dz;
            covariance[2][2] += dz*dz;
        }
        
        // Normalize covariance matrix
        for (int i = 0; i < 3; i++) {
            for (int j = i; j < 3; j++) {
                covariance[i][j] /= node->n_indices;
                if (i != j) covariance[j][i] = covariance[i][j];
            }
        }
        
        // Find eigenvector with largest eigenvalue (principal axis)
        // Simplified: use axis with maximum variance
        int split_axis = 0;
        double max_variance = covariance[0][0];
        for (int i = 1; i < 3; i++) {
            if (covariance[i][i] > max_variance) {
                max_variance = covariance[i][i];
                split_axis = i;
            }
        }
        
        // Split along principal axis
        int *left_indices = (int*)malloc(node->n_indices * sizeof(int));
        int *right_indices = (int*)malloc(node->n_indices * sizeof(int));
        int left_count = 0, right_count = 0;
        
        for (int i = 0; i < node->n_indices; i++) {
            int idx = node->indices[i];
            if (coords[3*idx + split_axis] < node->centroid[split_axis]) {
                left_indices[left_count++] = idx;
            } else {
                right_indices[right_count++] = idx;
            }
        }
        
        if (left_count == 0 || right_count == 0) {
            free(left_indices);
            free(right_indices);
            return;
        }
        
        // Create child nodes
        node->left = (ClusterNode*)malloc(sizeof(ClusterNode));
        node->right = (ClusterNode*)malloc(sizeof(ClusterNode));
        
        // Left child
        node->left->indices = left_indices;
        node->left->n_indices = left_count;
        node->left->centroid = (double*)malloc(3 * sizeof(double));
        node->left->level = node->level + 1;
        node->left->left = node->left->right = NULL;
        
        // Right child
        node->right->indices = right_indices;
        node->right->n_indices = right_count;
        node->right->centroid = (double*)malloc(3 * sizeof(double));
        node->right->level = node->level + 1;
        node->right->left = node->right->right = NULL;
        
        // Calculate centroids and radii for children
        for (int i = 0; i < 3; i++) {
            node->left->centroid[i] = 0.0;
            node->right->centroid[i] = 0.0;
        }
        
        for (int i = 0; i < left_count; i++) {
            int idx = left_indices[i];
            for (int j = 0; j < 3; j++) {
                node->left->centroid[j] += coords[3*idx + j];
            }
        }
        
        for (int i = 0; i < right_count; i++) {
            int idx = right_indices[i];
            for (int j = 0; j < 3; j++) {
                node->right->centroid[j] += coords[3*idx + j];
            }
        }
        
        for (int i = 0; i < 3; i++) {
            node->left->centroid[i] /= left_count;
            node->right->centroid[i] /= right_count;
        }
        
        // Calculate radii
        node->left->radius = 0.0;
        node->right->radius = 0.0;
        
        for (int i = 0; i < left_count; i++) {
            double point[3] = {coords[3*left_indices[i]], 
                              coords[3*left_indices[i] + 1], 
                              coords[3*left_indices[i] + 2]};
            double dist = euclidean_distance(node->left->centroid, point);
            node->left->radius = fmax(node->left->radius, dist);
        }
        
        for (int i = 0; i < right_count; i++) {
            double point[3] = {coords[3*right_indices[i]], 
                              coords[3*right_indices[i] + 1], 
                              coords[3*right_indices[i] + 2]};
            double dist = euclidean_distance(node->right->centroid, point);
            node->right->radius = fmax(node->right->radius, dist);
        }
        
        tree->n_clusters += 2;
        
        // Recursive bisection
        bisect_cluster(node->left, coords, max_depth, min_size);
        bisect_cluster(node->right, coords, max_depth, min_size);
    }
    
    bisect_cluster(tree->root, coordinates, max_depth, min_cluster_size);
    
    return tree;
}

// Adaptive Cross Approximation (ACA)
void adaptive_cross_approximation(
    const double complex *matrix,
    int n_rows, int n_cols,
    double tolerance,
    int max_rank,
    double complex **U, double complex **V,
    int *actual_rank) {
    
    *U = (double complex*)malloc(n_rows * max_rank * sizeof(double complex));
    *V = (double complex*)malloc(max_rank * n_cols * sizeof(double complex));
    
    double *row_norms = (double*)calloc(n_rows, sizeof(double));
    double *col_norms = (double*)calloc(n_cols, sizeof(double));
    
    // Initialize norms
    for (int i = 0; i < n_rows; i++) {
        for (int j = 0; j < n_cols; j++) {
            double abs_val = cabs(matrix[i * n_cols + j]);
            row_norms[i] += abs_val * abs_val;
            col_norms[j] += abs_val * abs_val;
        }
    }
    
    for (int i = 0; i < n_rows; i++) {
        row_norms[i] = sqrt(row_norms[i]);
    }
    for (int j = 0; j < n_cols; j++) {
        col_norms[j] = sqrt(col_norms[j]);
    }
    
    int rank = 0;
    double residual_norm = 0.0;
    
    for (rank = 0; rank < max_rank; rank++) {
        // Find pivot row
        int pivot_row = 0;
        double max_row_norm = row_norms[0];
        for (int i = 1; i < n_rows; i++) {
            if (row_norms[i] > max_row_norm) {
                max_row_norm = row_norms[i];
                pivot_row = i;
            }
        }
        
        if (max_row_norm < tolerance) break;
        
        // Extract pivot row
        for (int j = 0; j < n_cols; j++) {
            (*V)[rank * n_cols + j] = matrix[pivot_row * n_cols + j];
            // Subtract contribution from previous ranks
            for (int k = 0; k < rank; k++) {
                (*V)[rank * n_cols + j] -= (*U)[pivot_row * max_rank + k] * (*V)[k * n_cols + j];
            }
        }
        
        // Find pivot column in V
        int pivot_col = 0;
        double max_val = cabs((*V)[rank * n_cols]);
        for (int j = 1; j < n_cols; j++) {
            double abs_val = cabs((*V)[rank * n_cols + j]);
            if (abs_val > max_val) {
                max_val = abs_val;
                pivot_col = j;
            }
        }
        
        double pivot_value = (*V)[rank * n_cols + pivot_col];
        if (cabs(pivot_value) < 1e-12) break;
        
        // Normalize V row
        for (int j = 0; j < n_cols; j++) {
            (*V)[rank * n_cols + j] /= pivot_value;
        }
        
        // Extract U column
        for (int i = 0; i < n_rows; i++) {
            (*U)[i * max_rank + rank] = matrix[i * n_cols + pivot_col];
            // Subtract contribution from previous ranks
            for (int k = 0; k < rank; k++) {
                (*U)[i * max_rank + rank] -= (*U)[i * max_rank + k] * (*V)[k * n_cols + pivot_col];
            }
        }
        
        // Update residual norms
        for (int i = 0; i < n_rows; i++) {
            row_norms[i] = 0.0;
            for (int j = 0; j < n_cols; j++) {
                double complex residual = matrix[i * n_cols + j];
                for (int k = 0; k <= rank; k++) {
                    residual -= (*U)[i * max_rank + k] * (*V)[k * n_cols + j];
                }
                double abs_val = cabs(residual);
                row_norms[i] += abs_val * abs_val;
            }
            row_norms[i] = sqrt(row_norms[i]);
        }
        
        residual_norm = row_norms[pivot_row];
        if (residual_norm < tolerance) break;
    }
    
    *actual_rank = rank;
    
    free(row_norms);
    free(col_norms);
}

// H-matrix construction
HMatrix* create_h_matrix(
    const double complex *dense_matrix,
    int n_rows, int n_cols,
    const HMatrixParams *params) {
    
    HMatrix *h_matrix = (HMatrix*)malloc(sizeof(HMatrix));
    h_matrix->total_rows = n_rows;
    h_matrix->total_cols = n_cols;
    h_matrix->params = *params;
    
    // Build cluster trees for rows and columns
    double *row_coords = (double*)malloc(n_rows * 3 * sizeof(double));
    double *col_coords = (double*)malloc(n_cols * 3 * sizeof(double));
    
    // Generate coordinates (in practice, use actual geometry)
    for (int i = 0; i < n_rows; i++) {
        row_coords[3*i] = i * 0.1; // Simplified coordinates
        row_coords[3*i + 1] = 0.0;
        row_coords[3*i + 2] = 0.0;
    }
    
    for (int i = 0; i < n_cols; i++) {
        col_coords[3*i] = i * 0.1;
        col_coords[3*i + 1] = 0.1;
        col_coords[3*i + 2] = 0.0;
    }
    
    ClusterTree *row_tree = build_cluster_tree(row_coords, n_rows, 6, params->cluster_size);
    ClusterTree *col_tree = build_cluster_tree(col_coords, n_cols, 6, params->cluster_size);
    
    // Allocate blocks (overestimate)
    int max_blocks = n_rows * n_cols / (params->cluster_size * params->cluster_size);
    h_matrix->blocks = (HMatrixBlock*)malloc(max_blocks * sizeof(HMatrixBlock));
    h_matrix->n_blocks = 0;
    
    // Recursive block construction
    void build_h_matrix_blocks(HMatrix *hm, const ClusterNode *row_cluster,
                              const ClusterNode *col_cluster,
                              const double complex *matrix) {
        
        if (is_admissible(row_cluster, col_cluster, hm->params.admissibility)) {
            // Admissible block - use low-rank approximation
            HMatrixBlock *block = &hm->blocks[hm->n_blocks];
            
            block->row_indices = (int*)malloc(row_cluster->n_indices * sizeof(int));
            block->col_indices = (int*)malloc(col_cluster->n_indices * sizeof(int));
            memcpy(block->row_indices, row_cluster->indices, row_cluster->n_indices * sizeof(int));
            memcpy(block->col_indices, col_cluster->indices, col_cluster->n_indices * sizeof(int));
            
            block->n_rows = row_cluster->n_indices;
            block->n_cols = col_cluster->n_indices;
            block->is_low_rank = true;
            
            // Extract submatrix for ACA
            double complex *submatrix = (double complex*)malloc(block->n_rows * block->n_cols * sizeof(double complex));
            for (int i = 0; i < block->n_rows; i++) {
                for (int j = 0; j < block->n_cols; j++) {
                    int global_i = block->row_indices[i];
                    int global_j = block->col_indices[j];
                    submatrix[i * block->n_cols + j] = matrix[global_i * hm->total_cols + global_j];
                }
            }
            
            // Apply ACA
            double complex *U, *V;
            int rank;
            adaptive_cross_approximation(submatrix, block->n_rows, block->n_cols,
                                        hm->params.tolerance, hm->params.max_rank,
                                        &U, &V, &rank);
            
            block->data.low_rank.U = U;
            block->data.low_rank.V = V;
            block->data.low_rank.rank = rank;
            
            free(submatrix);
            hm->n_blocks++;
            
        } else if (row_cluster->left && col_cluster->left) {
            // Non-admissible but subdividable - recurse
            build_h_matrix_blocks(hm, row_cluster->left, col_cluster->left, matrix);
            build_h_matrix_blocks(hm, row_cluster->left, col_cluster->right, matrix);
            build_h_matrix_blocks(hm, row_cluster->right, col_cluster->left, matrix);
            build_h_matrix_blocks(hm, row_cluster->right, col_cluster->right, matrix);
            
        } else {
            // Leaf clusters - use dense block
            HMatrixBlock *block = &hm->blocks[hm->n_blocks];
            
            block->row_indices = (int*)malloc(row_cluster->n_indices * sizeof(int));
            block->col_indices = (int*)malloc(col_cluster->n_indices * sizeof(int));
            memcpy(block->row_indices, row_cluster->indices, row_cluster->n_indices * sizeof(int));
            memcpy(block->col_indices, col_cluster->indices, col_cluster->n_indices * sizeof(int));
            
            block->n_rows = row_cluster->n_indices;
            block->n_cols = col_cluster->n_indices;
            block->is_low_rank = false;
            
            block->data.dense.data = (double complex*)malloc(block->n_rows * block->n_cols * sizeof(double complex));
            for (int i = 0; i < block->n_rows; i++) {
                for (int j = 0; j < block->n_cols; j++) {
                    int global_i = block->row_indices[i];
                    int global_j = block->col_indices[j];
                    block->data.dense.data[i * block->n_cols + j] = matrix[global_i * hm->total_cols + global_j];
                }
            }
            
            hm->n_blocks++;
        }
    }
    
    build_h_matrix_blocks(h_matrix, row_tree->root, col_tree->root, dense_matrix);
    
    free(row_coords);
    free(col_coords);
    free_cluster_tree(row_tree);
    free_cluster_tree(col_tree);
    
    return h_matrix;
}

// H-matrix vector multiplication
void h_matrix_vector_multiply(
    const HMatrix *h_matrix,
    const double complex *input_vector,
    double complex *output_vector) {
    
    // Initialize output vector
    for (int i = 0; i < h_matrix->total_rows; i++) {
        output_vector[i] = 0.0 + 0.0 * I;
    }
    
    // Process each block
    for (int b = 0; b < h_matrix->n_blocks; b++) {
        const HMatrixBlock *block = &h_matrix->blocks[b];
        
        if (block->is_low_rank) {
            // Low-rank multiplication: (U*V^T)*x = U*(V^T*x)
            double complex *temp = (double complex*)malloc(block->data.low_rank.rank * sizeof(double complex));
            
            // V^T * x
            for (int k = 0; k < block->data.low_rank.rank; k++) {
                temp[k] = 0.0 + 0.0 * I;
                for (int j = 0; j < block->n_cols; j++) {
                    int global_j = block->col_indices[j];
                    temp[k] += block->data.low_rank.V[k * block->n_cols + j] * input_vector[global_j];
                }
            }
            
            // U * (V^T * x)
            for (int i = 0; i < block->n_rows; i++) {
                int global_i = block->row_indices[i];
                for (int k = 0; k < block->data.low_rank.rank; k++) {
                    output_vector[global_i] += block->data.low_rank.U[i * block->data.low_rank.rank + k] * temp[k];
                }
            }
            
            free(temp);
            
        } else {
            // Dense multiplication
            for (int i = 0; i < block->n_rows; i++) {
                int global_i = block->row_indices[i];
                for (int j = 0; j < block->n_cols; j++) {
                    int global_j = block->col_indices[j];
                    output_vector[global_i] += block->data.dense.data[i * block->n_cols + j] * input_vector[global_j];
                }
            }
        }
    }
}

// GMRES solver with H-matrix
SolverResult* gmres_solver_hmatrix(
    const HMatrix *h_matrix,
    const double complex *rhs_vector,
    const double complex *initial_guess,
    const IterativeSolverParams *params,
    const HMatrixLU *preconditioner) {
    
    SolverResult *result = (SolverResult*)malloc(sizeof(SolverResult));
    result->solution = (double complex*)malloc(h_matrix->total_rows * sizeof(double complex));
    result->converged = false;
    result->iterations = 0;
    
    // Initialize solution
    if (initial_guess) {
        memcpy(result->solution, initial_guess, h_matrix->total_rows * sizeof(double complex));
    } else {
        for (int i = 0; i < h_matrix->total_rows; i++) {
            result->solution[i] = 0.0 + 0.0 * I;
        }
    }
    
    // GMRES implementation (simplified)
    int n = h_matrix->total_rows;
    int restart = params->restart_parameter;
    
    double complex *r = (double complex*)malloc(n * sizeof(double complex));
    double complex *v = (double complex*)malloc(n * (restart + 1) * sizeof(double complex));
    double complex *h = (double complex*)malloc((restart + 1) * restart * sizeof(double complex));
    double complex *y = (double complex*)malloc(restart * sizeof(double complex));
    double complex *w = (double complex*)malloc(n * sizeof(double complex));
    
    // Initial residual
    h_matrix_vector_multiply(h_matrix, result->solution, w);
    for (int i = 0; i < n; i++) {
        r[i] = rhs_vector[i] - w[i];
    }
    
    double beta = 0.0;
    for (int i = 0; i < n; i++) {
        beta += creal(r[i] * conj(r[i]));
    }
    beta = sqrt(beta);
    result->residual_norm = beta;
    
    if (beta < params->tolerance) {
        result->converged = true;
        free(r); free(v); free(h); free(y); free(w);
        return result;
    }
    
    // Normalize initial vector
    for (int i = 0; i < n; i++) {
        v[i] = r[i] / beta;
    }
    
    // GMRES iterations
    for (int iter = 0; iter < params->max_iterations; iter += restart) {
        int k_max = fmin(restart, params->max_iterations - iter);
        
        // Arnoldi process
        for (int k = 0; k < k_max; k++) {
            h_matrix_vector_multiply(h_matrix, &v[k * n], w);
            
            // Orthogonalization
            for (int j = 0; j <= k; j++) {
                h[j * restart + k] = 0.0 + 0.0 * I;
                for (int i = 0; i < n; i++) {
                    h[j * restart + k] += conj(v[j * n + i]) * w[i];
                }
                for (int i = 0; i < n; i++) {
                    w[i] -= h[j * restart + k] * v[j * n + i];
                }
            }
            
            // Normalize
            double norm_w = 0.0;
            for (int i = 0; i < n; i++) {
                norm_w += creal(w[i] * conj(w[i]));
            }
            norm_w = sqrt(norm_w);
            
            if (k + 1 < restart + 1) {
                h[(k + 1) * restart + k] = norm_w;
                for (int i = 0; i < n; i++) {
                    v[(k + 1) * n + i] = w[i] / norm_w;
                }
            }
        }
        
        // Solve least squares problem (simplified)
        // In practice, use QR decomposition
        double complex beta_vec = beta;
        
        // Update solution (simplified)
        for (int i = 0; i < n; i++) {
            for (int k = 0; k < k_max; k++) {
                result->solution[i] += beta_vec * v[k * n + i];
            }
        }
        
        // Check convergence
        h_matrix_vector_multiply(h_matrix, result->solution, w);
        double new_residual = 0.0;
        for (int i = 0; i < n; i++) {
            double complex res = rhs_vector[i] - w[i];
            new_residual += creal(res * conj(res));
        }
        new_residual = sqrt(new_residual);
        result->residual_norm = new_residual;
        
        if (new_residual < params->tolerance) {
            result->converged = true;
            break;
        }
        
        // Update residual for next cycle
        for (int i = 0; i < n; i++) {
            r[i] = rhs_vector[i] - w[i];
        }
        beta = new_residual;
        
        for (int i = 0; i < n; i++) {
            v[i] = r[i] / beta;
        }
        
        result->iterations = iter + k_max;
    }
    
    free(r); free(v); free(h); free(y); free(w);
    
    return result;
}

// Memory management
void free_cluster_tree(ClusterTree *tree) {
    if (!tree) return;
    
    void free_node(ClusterNode *node) {
        if (!node) return;
        
        free(node->indices);
        free(node->centroid);
        free_node(node->left);
        free_node(node->right);
        free(node);
    }
    
    free_node(tree->root);
    free(tree);
}

void free_h_matrix(HMatrix *h_matrix) {
    if (!h_matrix) return;
    
    for (int i = 0; i < h_matrix->n_blocks; i++) {
        HMatrixBlock *block = &h_matrix->blocks[i];
        
        free(block->row_indices);
        free(block->col_indices);
        
        if (block->is_low_rank) {
            free(block->data.low_rank.U);
            free(block->data.low_rank.V);
        } else {
            free(block->data.dense.data);
        }
    }
    
    free(h_matrix->blocks);
    free(h_matrix);
}

void free_h_matrix_lu(HMatrixLU *lu) {
    if (!lu) return;
    
    free_h_matrix(lu->L);
    free_h_matrix(lu->U);
    free(lu->pivot);
    free(lu);
}

void free_solver_result(SolverResult *result) {
    if (result) {
        free(result->solution);
        free(result);
    }
}