/********************************************************************************
 * Higher-Order Basis Functions for HO-MoM
 *
 * Copyright (C) 2025 PulseEM Technologies
 *
 * Implements higher-order basis functions for Method of Moments:
 * - Polynomial basis functions (quadratic, cubic)
 * - Legendre polynomial basis
 * - Lagrange polynomial basis
 * - Hierarchical basis functions
 ********************************************************************************/

#ifndef HIGHER_ORDER_BASIS_H
#define HIGHER_ORDER_BASIS_H

#include "core_common.h"
#include "core_geometry.h"

#ifdef __cplusplus
extern "C" {
#endif

// Maximum polynomial order supported
#define MAX_POLY_ORDER 3

// Basis function types
typedef enum {
    HO_BASIS_POLYNOMIAL,    // Standard polynomial basis
    HO_BASIS_LEGENDRE,      // Legendre polynomial basis
    HO_BASIS_LAGRANGE,      // Lagrange polynomial basis
    HO_BASIS_HIERARCHICAL   // Hierarchical basis
} ho_basis_type_t;

// Structure for higher-order basis function
typedef struct {
    ho_basis_type_t type;      // Basis function type
    int order;                 // Polynomial order (1=linear, 2=quadratic, 3=cubic)
    int num_dofs;              // Number of degrees of freedom
    int* dof_indices;          // DOF indices for this element
    double* coefficients;      // Basis function coefficients
} ho_basis_function_t;

// Structure for higher-order element
typedef struct {
    const geom_triangle_t* triangle;  // Reference triangle
    int order;                        // Polynomial order
    ho_basis_type_t basis_type;       // Basis function type
    int num_basis_functions;          // Number of basis functions
    ho_basis_function_t* basis_functions;  // Array of basis functions
} ho_element_t;

/**
 * @brief Evaluate polynomial shape function on triangle
 * 
 * For order 1 (linear): N_i = L_i (barycentric coordinates)
 * For order 2 (quadratic): N_i = L_i * (2*L_i - 1) for vertices, N_i = 4*L_j*L_k for edges
 * For order 3 (cubic): Higher-order polynomials
 * 
 * @param tri Triangle
 * @param xi, eta, zeta Barycentric coordinates
 * @param order Polynomial order
 * @param shape_idx Shape function index
 * @return Shape function value
 */
double ho_evaluate_polynomial_shape(const geom_triangle_t* tri,
                                    double xi, double eta, double zeta,
                                    int order, int shape_idx);

/**
 * @brief Evaluate Legendre polynomial shape function
 * 
 * Uses Legendre polynomials for better numerical properties
 * 
 * @param tri Triangle
 * @param xi, eta, zeta Barycentric coordinates
 * @param order Polynomial order
 * @param shape_idx Shape function index
 * @return Shape function value
 */
double ho_evaluate_legendre_shape(const geom_triangle_t* tri,
                                  double xi, double eta, double zeta,
                                  int order, int shape_idx);

/**
 * @brief Evaluate Lagrange polynomial shape function
 * 
 * Uses Lagrange interpolation polynomials
 * 
 * @param tri Triangle
 * @param xi, eta, zeta Barycentric coordinates
 * @param order Polynomial order
 * @param shape_idx Shape function index
 * @return Shape function value
 */
double ho_evaluate_lagrange_shape(const geom_triangle_t* tri,
                                 double xi, double eta, double zeta,
                                 int order, int shape_idx);

/**
 * @brief Evaluate hierarchical shape function
 * 
 * Hierarchical basis functions for p-adaptivity
 * 
 * @param tri Triangle
 * @param xi, eta, zeta Barycentric coordinates
 * @param order Polynomial order
 * @param shape_idx Shape function index
 * @return Shape function value
 */
double ho_evaluate_hierarchical_shape(const geom_triangle_t* tri,
                                      double xi, double eta, double zeta,
                                      int order, int shape_idx);

/**
 * @brief Evaluate gradient of shape function
 * 
 * @param tri Triangle
 * @param xi, eta, zeta Barycentric coordinates
 * @param order Polynomial order
 * @param shape_idx Shape function index
 * @param grad_xi, grad_eta, grad_zeta Output: gradient components
 */
void ho_evaluate_shape_gradient(const geom_triangle_t* tri,
                               double xi, double eta, double zeta,
                               int order, int shape_idx,
                               double* grad_xi, double* grad_eta, double* grad_zeta);

/**
 * @brief Create higher-order basis functions for a triangle
 * 
 * @param tri Triangle element
 * @param order Polynomial order (1, 2, or 3)
 * @param basis_type Basis function type
 * @return Pointer to ho_element_t structure (caller must free)
 */
ho_element_t* ho_create_element_basis(const geom_triangle_t* tri,
                                      int order,
                                      ho_basis_type_t basis_type);

/**
 * @brief Evaluate higher-order basis function vector
 * 
 * @param element Higher-order element
 * @param xi, eta, zeta Barycentric coordinates
 * @param basis_idx Basis function index
 * @param result Output: basis function vector (3 components)
 */
void ho_evaluate_basis_vector(const ho_element_t* element,
                              double xi, double eta, double zeta,
                              int basis_idx,
                              geom_point_t* result);

/**
 * @brief Evaluate divergence of higher-order basis function
 * 
 * @param element Higher-order element
 * @param xi, eta, zeta Barycentric coordinates
 * @param basis_idx Basis function index
 * @return Divergence value
 */
double ho_evaluate_basis_divergence(const ho_element_t* element,
                                    double xi, double eta, double zeta,
                                    int basis_idx);

/**
 * @brief Compute number of DOFs for given order
 * 
 * Order 1 (linear): 3 DOFs (vertices)
 * Order 2 (quadratic): 6 DOFs (3 vertices + 3 edges)
 * Order 3 (cubic): 10 DOFs (3 vertices + 6 edges + 1 face)
 * 
 * @param order Polynomial order
 * @return Number of degrees of freedom
 */
int ho_get_num_dofs(int order);

/**
 * @brief Get DOF locations for given order
 * 
 * @param order Polynomial order
 * @param dof_xi, dof_eta, dof_zeta Output arrays for DOF barycentric coordinates
 */
void ho_get_dof_locations(int order,
                          double* dof_xi, double* dof_eta, double* dof_zeta);

/**
 * @brief Free higher-order element structure
 * 
 * @param element Element to free
 */
void ho_free_element(ho_element_t* element);

/**
 * @brief Compute Legendre polynomial P_n(x)
 * 
 * @param n Order
 * @param x Argument
 * @return Legendre polynomial value
 */
double legendre_polynomial(int n, double x);

/**
 * @brief Compute derivative of Legendre polynomial P'_n(x)
 * 
 * @param n Order
 * @param x Argument
 * @return Legendre polynomial derivative value
 */
double legendre_polynomial_derivative(int n, double x);

#ifdef __cplusplus
}
#endif

#endif // HIGHER_ORDER_BASIS_H

