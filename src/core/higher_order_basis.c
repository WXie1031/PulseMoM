/********************************************************************************
 * Higher-Order Basis Functions Implementation for HO-MoM
 *
 * Copyright (C) 2025 PulseEM Technologies
 *
 * Implements higher-order basis functions for Method of Moments
 ********************************************************************************/

#include "higher_order_basis.h"
#include "core_common.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

/*********************************************************************
 * Legendre Polynomial Functions
 *********************************************************************/

/**
 * @brief Compute Legendre polynomial P_n(x) using recurrence relation
 * 
 * P_0(x) = 1
 * P_1(x) = x
 * P_{n+1}(x) = ((2n+1)*x*P_n(x) - n*P_{n-1}(x)) / (n+1)
 */
double legendre_polynomial(int n, double x) {
    if (n < 0) {
        return 0.0;
    }
    
    if (n == 0) {
        return 1.0;
    }
    
    if (n == 1) {
        return x;
    }
    
    // Clamp x to [-1, 1] for numerical stability
    if (x > 1.0) x = 1.0;
    if (x < -1.0) x = -1.0;
    
    // Recurrence relation
    double P_prev = 1.0;  // P_0
    double P_curr = x;    // P_1
    
    for (int i = 2; i <= n; i++) {
        double P_next = ((2.0 * i - 1.0) * x * P_curr - (i - 1.0) * P_prev) / i;
        P_prev = P_curr;
        P_curr = P_next;
    }
    
    return P_curr;
}

/**
 * @brief Compute derivative of Legendre polynomial P'_n(x)
 * 
 * P'_n(x) = n * (x*P_n(x) - P_{n-1}(x)) / (x² - 1)
 * For x = ±1, use limit: P'_n(±1) = ±n(n+1)/2
 */
double legendre_polynomial_derivative(int n, double x) {
    if (n <= 0) {
        return 0.0;
    }
    
    if (n == 1) {
        return 1.0;
    }
    
    // Handle special cases at boundaries
    if (fabs(x - 1.0) < NUMERICAL_EPSILON) {
        return n * (n + 1.0) * 0.5;
    }
    
    if (fabs(x + 1.0) < NUMERICAL_EPSILON) {
        return ((n % 2 == 0) ? 1.0 : -1.0) * n * (n + 1.0) * 0.5;
    }
    
    // General formula
    double P_n = legendre_polynomial(n, x);
    double P_n_minus_1 = legendre_polynomial(n - 1, x);
    double denom = x * x - 1.0;
    
    if (fabs(denom) < NUMERICAL_EPSILON) {
        // Use limit formula
        return n * (x * P_n - P_n_minus_1) / (2.0 * x);
    }
    
    return n * (x * P_n - P_n_minus_1) / denom;
}

/*********************************************************************
 * DOF Location and Counting
 *********************************************************************/

/**
 * @brief Compute number of DOFs for given order
 */
int ho_get_num_dofs(int order) {
    if (order == 1) {
        return 3;  // Vertices only
    } else if (order == 2) {
        return 6;  // 3 vertices + 3 edges
    } else if (order == 3) {
        return 10; // 3 vertices + 6 edges + 1 face
    }
    return 0;
}

/**
 * @brief Get DOF locations for given order
 */
void ho_get_dof_locations(int order,
                          double* dof_xi, double* dof_eta, double* dof_zeta) {
    if (order == 1) {
        // Vertices only
        dof_xi[0] = 1.0; dof_eta[0] = 0.0; dof_zeta[0] = 0.0;  // Vertex 0
        dof_xi[1] = 0.0; dof_eta[1] = 1.0; dof_zeta[1] = 0.0;  // Vertex 1
        dof_xi[2] = 0.0; dof_eta[2] = 0.0; dof_zeta[2] = 1.0;  // Vertex 2
    } else if (order == 2) {
        // Vertices
        dof_xi[0] = 1.0; dof_eta[0] = 0.0; dof_zeta[0] = 0.0;
        dof_xi[1] = 0.0; dof_eta[1] = 1.0; dof_zeta[1] = 0.0;
        dof_xi[2] = 0.0; dof_eta[2] = 0.0; dof_zeta[2] = 1.0;
        // Edge midpoints
        dof_xi[3] = 0.5; dof_eta[3] = 0.5; dof_zeta[3] = 0.0;  // Edge 0-1
        dof_xi[4] = 0.5; dof_eta[4] = 0.0; dof_zeta[4] = 0.5;  // Edge 0-2
        dof_xi[5] = 0.0; dof_eta[5] = 0.5; dof_zeta[5] = 0.5;  // Edge 1-2
    } else if (order == 3) {
        // Vertices
        dof_xi[0] = 1.0; dof_eta[0] = 0.0; dof_zeta[0] = 0.0;
        dof_xi[1] = 0.0; dof_eta[1] = 1.0; dof_zeta[1] = 0.0;
        dof_xi[2] = 0.0; dof_eta[2] = 0.0; dof_zeta[2] = 1.0;
        // Edge points (1/3 and 2/3)
        dof_xi[3] = 2.0/3.0; dof_eta[3] = 1.0/3.0; dof_zeta[3] = 0.0;  // Edge 0-1
        dof_xi[4] = 1.0/3.0; dof_eta[4] = 2.0/3.0; dof_zeta[4] = 0.0;
        dof_xi[5] = 2.0/3.0; dof_eta[5] = 0.0; dof_zeta[5] = 1.0/3.0;  // Edge 0-2
        dof_xi[6] = 1.0/3.0; dof_eta[6] = 0.0; dof_zeta[6] = 2.0/3.0;
        dof_xi[7] = 0.0; dof_eta[7] = 2.0/3.0; dof_zeta[7] = 1.0/3.0;  // Edge 1-2
        dof_xi[8] = 0.0; dof_eta[8] = 1.0/3.0; dof_zeta[8] = 2.0/3.0;
        // Face center
        dof_xi[9] = ONE_THIRD; dof_eta[9] = ONE_THIRD; dof_zeta[9] = ONE_THIRD;
    }
}

/*********************************************************************
 * Polynomial Shape Functions
 *********************************************************************/

/**
 * @brief Evaluate polynomial shape function on triangle
 */
double ho_evaluate_polynomial_shape(const geom_triangle_t* tri,
                                    double xi, double eta, double zeta,
                                    int order, int shape_idx) {
    (void)tri;  // Unused for polynomial basis
    
    if (order == 1) {
        // Linear: N_i = L_i (barycentric coordinates)
        if (shape_idx == 0) return xi;
        if (shape_idx == 1) return eta;
        if (shape_idx == 2) return zeta;
    } else if (order == 2) {
        // Quadratic: vertex functions N_i = L_i * (2*L_i - 1)
        //           edge functions N_i = 4*L_j*L_k
        if (shape_idx == 0) return xi * (2.0 * xi - 1.0);           // Vertex 0
        if (shape_idx == 1) return eta * (2.0 * eta - 1.0);         // Vertex 1
        if (shape_idx == 2) return zeta * (2.0 * zeta - 1.0);     // Vertex 2
        if (shape_idx == 3) return 4.0 * xi * eta;                 // Edge 0-1
        if (shape_idx == 4) return 4.0 * xi * zeta;              // Edge 0-2
        if (shape_idx == 5) return 4.0 * eta * zeta;               // Edge 1-2
    } else if (order == 3) {
        // Cubic: more complex expressions
        // Vertex functions: N_i = (1/2) * L_i * (3*L_i - 1) * (3*L_i - 2)
        // Edge functions: N_i = (9/2) * L_j * L_k * (3*L_j - 1) or (3*L_k - 1)
        // Face function: N = 27 * L_0 * L_1 * L_2
        if (shape_idx == 0) return 0.5 * xi * (3.0 * xi - 1.0) * (3.0 * xi - 2.0);
        if (shape_idx == 1) return 0.5 * eta * (3.0 * eta - 1.0) * (3.0 * eta - 2.0);
        if (shape_idx == 2) return 0.5 * zeta * (3.0 * zeta - 1.0) * (3.0 * zeta - 2.0);
        if (shape_idx == 3) return 4.5 * xi * eta * (3.0 * xi - 1.0);      // Edge 0-1, point 1/3
        if (shape_idx == 4) return 4.5 * xi * eta * (3.0 * eta - 1.0);    // Edge 0-1, point 2/3
        if (shape_idx == 5) return 4.5 * xi * zeta * (3.0 * xi - 1.0);    // Edge 0-2, point 1/3
        if (shape_idx == 6) return 4.5 * xi * zeta * (3.0 * zeta - 1.0);  // Edge 0-2, point 2/3
        if (shape_idx == 7) return 4.5 * eta * zeta * (3.0 * eta - 1.0); // Edge 1-2, point 1/3
        if (shape_idx == 8) return 4.5 * eta * zeta * (3.0 * zeta - 1.0); // Edge 1-2, point 2/3
        if (shape_idx == 9) return 27.0 * xi * eta * zeta;                // Face center
    }
    
    return 0.0;
}

/*********************************************************************
 * Legendre Shape Functions
 *********************************************************************/

/**
 * @brief Evaluate Legendre polynomial shape function
 */
double ho_evaluate_legendre_shape(const geom_triangle_t* tri,
                                  double xi, double eta, double zeta,
                                  int order, int shape_idx) {
    (void)tri;  // Unused for Legendre basis
    
    if (order == 1) {
        // Linear: same as polynomial
        if (shape_idx == 0) return xi;
        if (shape_idx == 1) return eta;
        if (shape_idx == 2) return zeta;
    } else if (order == 2) {
        // Quadratic: use Legendre polynomials
        // Map barycentric coordinates to [-1, 1] for Legendre polynomials
        if (shape_idx == 0) return legendre_polynomial(0, 2.0 * xi - 1.0);
        if (shape_idx == 1) return legendre_polynomial(0, 2.0 * eta - 1.0);
        if (shape_idx == 2) return legendre_polynomial(0, 2.0 * zeta - 1.0);
        if (shape_idx == 3) return legendre_polynomial(1, 2.0 * xi - 1.0) * sqrt(xi * eta);
        if (shape_idx == 4) return legendre_polynomial(1, 2.0 * xi - 1.0) * sqrt(xi * zeta);
        if (shape_idx == 5) return legendre_polynomial(1, 2.0 * eta - 1.0) * sqrt(eta * zeta);
    } else if (order == 3) {
        // Cubic: higher-order Legendre polynomials
        if (shape_idx < 3) {
            return legendre_polynomial(0, 2.0 * (shape_idx == 0 ? xi : (shape_idx == 1 ? eta : zeta)) - 1.0);
        } else {
            // Edge and face functions with Legendre polynomials
            // Simplified implementation
            return ho_evaluate_polynomial_shape(tri, xi, eta, zeta, order, shape_idx);
        }
    }
    
    return 0.0;
}

/*********************************************************************
 * Lagrange Shape Functions
 *********************************************************************/

/**
 * @brief Evaluate Lagrange polynomial shape function
 * 
 * Optimized implementation using analytical formulas for barycentric coordinates
 */
double ho_evaluate_lagrange_shape(const geom_triangle_t* tri,
                                 double xi, double eta, double zeta,
                                 int order, int shape_idx) {
    (void)tri;  // Unused for Lagrange basis
    
    // For order 1, Lagrange basis is same as barycentric coordinates
    if (order == 1) {
        if (shape_idx == 0) return xi;
        if (shape_idx == 1) return eta;
        if (shape_idx == 2) return zeta;
        return 0.0;
    }
    
    // For higher orders, use analytical formulas based on DOF locations
    // This avoids expensive memory allocation and numerical distance calculations
    if (order == 2) {
        // Quadratic Lagrange basis on triangle
        if (shape_idx == 0) return xi * (2.0 * xi - 1.0);           // Vertex 0
        if (shape_idx == 1) return eta * (2.0 * eta - 1.0);        // Vertex 1
        if (shape_idx == 2) return zeta * (2.0 * zeta - 1.0);      // Vertex 2
        if (shape_idx == 3) return 4.0 * xi * eta;                 // Edge 0-1 midpoint
        if (shape_idx == 4) return 4.0 * xi * zeta;                // Edge 0-2 midpoint
        if (shape_idx == 5) return 4.0 * eta * zeta;               // Edge 1-2 midpoint
    } else if (order == 3) {
        // Cubic Lagrange basis - use polynomial basis for efficiency
        // (Lagrange and polynomial bases are equivalent for triangles)
        return ho_evaluate_polynomial_shape(tri, xi, eta, zeta, order, shape_idx);
    }
    
    return 0.0;
}

/*********************************************************************
 * Hierarchical Shape Functions
 *********************************************************************/

/**
 * @brief Evaluate hierarchical shape function
 */
double ho_evaluate_hierarchical_shape(const geom_triangle_t* tri,
                                      double xi, double eta, double zeta,
                                      int order, int shape_idx) {
    // Hierarchical basis: start with linear, add higher-order terms
    if (shape_idx < 3) {
        // Linear part
        return ho_evaluate_polynomial_shape(tri, xi, eta, zeta, 1, shape_idx);
    } else if (order >= 2 && shape_idx < 6) {
        // Quadratic edge functions
        return ho_evaluate_polynomial_shape(tri, xi, eta, zeta, 2, shape_idx);
    } else if (order >= 3) {
        // Cubic functions
        return ho_evaluate_polynomial_shape(tri, xi, eta, zeta, 3, shape_idx);
    }
    
    return 0.0;
}

/*********************************************************************
 * Shape Function Gradients
 *********************************************************************/

/**
 * @brief Evaluate gradient of shape function
 * 
 * Optimized implementation using analytical derivatives
 */
void ho_evaluate_shape_gradient(const geom_triangle_t* tri,
                               double xi, double eta, double zeta,
                               int order, int shape_idx,
                               double* grad_xi, double* grad_eta, double* grad_zeta) {
    (void)tri;  // Unused for gradient computation
    
    // Initialize
    *grad_xi = 0.0;
    *grad_eta = 0.0;
    *grad_zeta = 0.0;
    
    // Analytical derivatives for polynomial basis
    if (order == 1) {
        // Linear: N_i = L_i, so gradient is constant
        if (shape_idx == 0) {
            *grad_xi = 1.0;
            *grad_eta = 0.0;
        } else if (shape_idx == 1) {
            *grad_xi = 0.0;
            *grad_eta = 1.0;
        } else if (shape_idx == 2) {
            *grad_xi = 0.0;
            *grad_eta = 0.0;
        }
    } else if (order == 2) {
        // Quadratic: analytical derivatives
        if (shape_idx == 0) {
            *grad_xi = 4.0 * xi - 1.0;  // d/dxi [xi*(2*xi-1)]
            *grad_eta = 0.0;
        } else if (shape_idx == 1) {
            *grad_xi = 0.0;
            *grad_eta = 4.0 * eta - 1.0;  // d/deta [eta*(2*eta-1)]
        } else if (shape_idx == 2) {
            *grad_xi = 0.0;
            *grad_eta = 0.0;
            // zeta gradient will be computed below
        } else if (shape_idx == 3) {
            *grad_xi = 4.0 * eta;  // d/dxi [4*xi*eta]
            *grad_eta = 4.0 * xi;
        } else if (shape_idx == 4) {
            *grad_xi = 4.0 * zeta;  // d/dxi [4*xi*zeta]
            *grad_eta = 0.0;
        } else if (shape_idx == 5) {
            *grad_xi = 0.0;
            *grad_eta = 4.0 * zeta;  // d/deta [4*eta*zeta]
        }
        
        // Handle zeta derivatives for vertex 2
        if (shape_idx == 2) {
            *grad_zeta = 4.0 * zeta - 1.0;  // d/dzeta [zeta*(2*zeta-1)]
        } else if (shape_idx == 4) {
            *grad_zeta = 4.0 * xi;  // d/dzeta [4*xi*zeta]
        } else if (shape_idx == 5) {
            *grad_zeta = 4.0 * eta;  // d/dzeta [4*eta*zeta]
        } else {
            *grad_zeta = 0.0;
        }
    } else if (order == 3) {
        // Cubic: use analytical derivatives where possible
        // For complex cases, fall back to numerical differentiation
        double eps = 1e-8;
        double N_xi = ho_evaluate_polynomial_shape(tri, xi + eps, eta, zeta, order, shape_idx);
        double N_xi_minus = ho_evaluate_polynomial_shape(tri, xi - eps, eta, zeta, order, shape_idx);
        *grad_xi = (N_xi - N_xi_minus) / (2.0 * eps);
        
        double N_eta = ho_evaluate_polynomial_shape(tri, xi, eta + eps, zeta, order, shape_idx);
        double N_eta_minus = ho_evaluate_polynomial_shape(tri, xi, eta - eps, zeta, order, shape_idx);
        *grad_eta = (N_eta - N_eta_minus) / (2.0 * eps);
        
        *grad_zeta = 0.0;  // Will be computed below
    }
    
    // zeta = 1 - xi - eta, so grad_zeta = -grad_xi - grad_eta
    // (unless already computed above)
    if (order != 2 || (shape_idx != 2 && shape_idx != 4 && shape_idx != 5)) {
        *grad_zeta = -(*grad_xi) - (*grad_eta);
    }
}

/*********************************************************************
 * Higher-Order Element Creation
 *********************************************************************/

/**
 * @brief Create higher-order basis functions for a triangle
 * 
 * Optimized: Reduced memory allocations by using single allocation for dof_indices
 */
ho_element_t* ho_create_element_basis(const geom_triangle_t* tri,
                                      int order,
                                      ho_basis_type_t basis_type) {
    if (!tri || order < 1 || order > MAX_POLY_ORDER) {
        return NULL;
    }
    
    int num_dofs = ho_get_num_dofs(order);
    if (num_dofs <= 0) {
        return NULL;
    }
    
    // Optimized: Single allocation for element structure and all basis functions
    // Allocate element + basis_functions array + dof_indices array in one block
    size_t element_size = sizeof(ho_element_t);
    size_t basis_functions_size = num_dofs * sizeof(ho_basis_function_t);
    size_t dof_indices_size = num_dofs * sizeof(int);
    size_t total_size = element_size + basis_functions_size + dof_indices_size;
    
    char* memory_block = (char*)malloc(total_size);
    if (!memory_block) {
        return NULL;
    }
    
    // Set up pointers
    ho_element_t* element = (ho_element_t*)memory_block;
    element->basis_functions = (ho_basis_function_t*)(memory_block + element_size);
    int* dof_indices_array = (int*)(memory_block + element_size + basis_functions_size);
    
    // Initialize element
    element->triangle = tri;
    element->order = order;
    element->basis_type = basis_type;
    element->num_basis_functions = num_dofs;
    
    // Initialize basis functions with shared dof_indices array
    for (int i = 0; i < num_dofs; i++) {
        element->basis_functions[i].type = basis_type;
        element->basis_functions[i].order = order;
        element->basis_functions[i].num_dofs = 1;
        element->basis_functions[i].dof_indices = &dof_indices_array[i];  // Point to shared array
        dof_indices_array[i] = i;  // Initialize value
        element->basis_functions[i].coefficients = NULL;
    }
    
    return element;
}

/*********************************************************************
 * Basis Function Evaluation
 *********************************************************************/

/**
 * @brief Evaluate higher-order basis function vector
 */
void ho_evaluate_basis_vector(const ho_element_t* element,
                              double xi, double eta, double zeta,
                              int basis_idx,
                              geom_point_t* result) {
    if (!element || !result || basis_idx < 0 || 
        basis_idx >= element->num_basis_functions) {
        result->x = 0.0;
        result->y = 0.0;
        result->z = 0.0;
        return;
    }
    
    // Evaluate shape function
    double N = 0.0;
    switch (element->basis_type) {
        case HO_BASIS_POLYNOMIAL:
            N = ho_evaluate_polynomial_shape(element->triangle, xi, eta, zeta,
                                            element->order, basis_idx);
            break;
        case HO_BASIS_LEGENDRE:
            N = ho_evaluate_legendre_shape(element->triangle, xi, eta, zeta,
                                          element->order, basis_idx);
            break;
        case HO_BASIS_LAGRANGE:
            N = ho_evaluate_lagrange_shape(element->triangle, xi, eta, zeta,
                                          element->order, basis_idx);
            break;
        case HO_BASIS_HIERARCHICAL:
            N = ho_evaluate_hierarchical_shape(element->triangle, xi, eta, zeta,
                                               element->order, basis_idx);
            break;
    }
    
    // For RWG-like basis, compute vector from shape function
    // This is a simplified version - full implementation would compute
    // proper vector basis functions
    const geom_triangle_t* tri = element->triangle;
    
    // Compute gradient of shape function
    double grad_xi, grad_eta, grad_zeta;
    ho_evaluate_shape_gradient(element->triangle, xi, eta, zeta,
                               element->order, basis_idx,
                               &grad_xi, &grad_eta, &grad_zeta);
    
    // Map to physical coordinates (simplified - assumes linear mapping)
    result->x = N * (tri->vertices[1].x - tri->vertices[0].x);
    result->y = N * (tri->vertices[1].y - tri->vertices[0].y);
    result->z = N * (tri->vertices[1].z - tri->vertices[0].z);
}

/**
 * @brief Evaluate divergence of higher-order basis function
 */
double ho_evaluate_basis_divergence(const ho_element_t* element,
                                    double xi, double eta, double zeta,
                                    int basis_idx) {
    if (!element || basis_idx < 0 || basis_idx >= element->num_basis_functions) {
        return 0.0;
    }
    
    // Compute gradient
    double grad_xi, grad_eta, grad_zeta;
    ho_evaluate_shape_gradient(element->triangle, xi, eta, zeta,
                               element->order, basis_idx,
                               &grad_xi, &grad_eta, &grad_zeta);
    
    // Divergence in barycentric coordinates (simplified)
    // Full implementation would compute proper divergence in physical space
    return grad_xi + grad_eta + grad_zeta;
}

/*********************************************************************
 * Memory Management
 *********************************************************************/

/**
 * @brief Free higher-order element structure
 * 
 * Optimized: Single free() call since all memory is allocated in one block
 */
void ho_free_element(ho_element_t* element) {
    if (!element) {
        return;
    }
    
    // Free coefficients if allocated separately
    if (element->basis_functions) {
        for (int i = 0; i < element->num_basis_functions; i++) {
            if (element->basis_functions[i].coefficients) {
                free(element->basis_functions[i].coefficients);
            }
        }
    }
    
    // Optimized: Single free() since element, basis_functions, and dof_indices
    // are all allocated in one contiguous block
    free(element);
}

