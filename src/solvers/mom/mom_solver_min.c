#include "../../discretization/geometry/core_geometry.h"
#include "mom_solver.h"
#include "../../discretization/mesh/core_mesh.h"
#include "../../operators/assembler/core_assembler.h"
#include "../../discretization/geometry/port_support_extended.h"
#include "../../discretization/geometry/opencascade_cad_import.h"
#include "../../discretization/mesh/cad_mesh_generation.h"
#include "../../discretization/mesh/gmsh_mesh_import.h"
#include "../../io/advanced_file_formats.h"
#include "../../io/file_formats/export_vtk.h"
#include "../../io/postprocessing/field_postprocessing.h"
#include "../../operators/kernels/electromagnetic_kernels.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Unified MoM solver entry points
int mom_solve_unified(mesh_t *mesh, double frequency, complex_t *excitation,
                     complex_t *current_solution, void *config /* mom_unified_config_t* */);
void mom_compute_radiation_pattern_unified(mesh_t *mesh, complex_t *currents, double frequency,
                                        double theta_min, double theta_max, double phi_min, double phi_max,
                                        int n_theta, int n_phi, complex_t *far_field);

typedef struct mom_solver {
    mom_config_t config;
    geom_geometry_t* geometry;
    mesh_t* mesh;
    mom_excitation_t excitation;

    // Unified numerical buffers
    complex_t* excitation_vector;       // RHS for unified solver
    mom_result_t result;
    int num_unknowns;
    
    // Port support
    extended_port_t** ports;            // Array of port pointers
    int num_ports;                     // Number of ports
    int ports_capacity;                // Capacity of ports array
} mom_solver_t;

static void mom_solver_free_buffers(mom_solver_t* solver) {
    if (!solver) return;
    free(solver->excitation_vector);
    free(solver->result.current_coefficients);
    free(solver->result.current_magnitude);
    free(solver->result.current_phase);
    free(solver->result.near_field.e_field);
    free(solver->result.near_field.h_field);
    free(solver->result.far_field.theta_angles);
    free(solver->result.far_field.phi_angles);
    free(solver->result.far_field.e_theta);
    free(solver->result.far_field.e_phi);
    free(solver->result.rcs_values);
    memset(&solver->result, 0, sizeof(solver->result));
    solver->excitation_vector = NULL;
    
    // Free ports
    if (solver->ports) {
        for (int i = 0; i < solver->num_ports; i++) {
            if (solver->ports[i]) {
                port_destroy(solver->ports[i]);
            }
        }
        free(solver->ports);
        solver->ports = NULL;
    }
    solver->num_ports = 0;
    solver->ports_capacity = 0;
}

mom_solver_t* mom_solver_create(const mom_config_t* config) {
    mom_solver_t* s = (mom_solver_t*)calloc(1, sizeof(mom_solver_t));
    if (!s) return NULL;
    if (config) s->config = *config;
    if (s->config.formulation == (mom_formulation_t)0) {
        s->config.formulation = MOM_FORMULATION_EFIE;
    }
    s->config.max_iterations = s->config.max_iterations > 0 ? s->config.max_iterations : 200;
    s->config.tolerance = (s->config.tolerance > 0.0) ? s->config.tolerance : 1e-3;
    // Sensible defaults for excitation
    s->excitation.type = MOM_EXCITATION_PLANE_WAVE;
    s->excitation.frequency = (config && config->frequency > 0.0) ? config->frequency : 1e9;
    s->excitation.amplitude = 1.0;
    s->ports = NULL;
    s->num_ports = 0;
    s->ports_capacity = 0;
    
    // EFIE near/singular handling defaults (safe for small meshes)
    if (s->config.near_threshold <= 0.0) {
        s->config.near_threshold = 0.1;  // 0.1 wavelengths default (conservative)
    }
    // Validate and clamp threshold to reasonable range [0.01, 1.0] wavelengths
    if (s->config.near_threshold < 0.01) {
        s->config.near_threshold = 0.01;  // Minimum: 0.01 wavelengths
    }
    if (s->config.near_threshold > 1.0) {
        s->config.near_threshold = 1.0;  // Maximum: 1.0 wavelengths
    }
    
    // Validate self-term regularization if provided
    if (s->config.enable_self_term_analytic) {
        if (s->config.near_threshold <= 0.0) {  // Note: using near_threshold as placeholder
            // If there's a separate regularization field, validate it here
        }
    }
    
    // Note: enable_duffy and enable_self_term_analytic are bool, so they default to false
    // If user wants them enabled, they should set them explicitly in config
    // For backward compatibility, we don't force them on by default
    // However, when enabled, they provide improved accuracy for near-field interactions
    
    return s;
}

void mom_solver_destroy(mom_solver_t* solver) {
    if (!solver) return;
    mom_solver_free_buffers(solver);
    free(solver);
}

int mom_solver_configure(mom_solver_t* solver, const mom_config_t* config) {
    if (!solver || !config) return -1;
    solver->config = *config;
    return 0;
}

int mom_solver_set_geometry(mom_solver_t* solver, void* geometry) {
    if (!solver || !geometry) return -1;
    solver->geometry = (geom_geometry_t*)geometry;
    return 0;
}

int mom_solver_set_mesh(mom_solver_t* solver, void* mesh) {
    if (!solver || !mesh) return -1;
    solver->mesh = (mesh_t*)mesh;
    /* 初值；实际维数在 mom_solver_assemble_matrix 中按 kernel_formulation 同步（EFIE→边，MFIE/CFIE→三角）。 */
    if (solver->mesh->num_elements > 0) {
        solver->num_unknowns = solver->mesh->num_elements;
    } else if (solver->mesh->num_edges > 0) {
        solver->num_unknowns = solver->mesh->num_edges;
    } else {
        solver->num_unknowns = 0;
    }
    if (solver->num_unknowns <= 0) {
        solver->num_unknowns = 0;
        return -1;
    }
    return 0;
}

int mom_solver_add_excitation(mom_solver_t* solver, const mom_excitation_t* excitation) {
    if (!solver || !excitation) return -1;
    solver->excitation = *excitation;
    return 0;
}

int mom_solver_add_lumped_excitation(mom_solver_t* solver, const point3d_t* position,
                                     const point3d_t* polarization,
                                     double amplitude, double width, int layer_index) {
    if (!solver) return -1;
    (void)width; (void)layer_index;
    solver->excitation.type = MOM_EXCITATION_VOLTAGE_SOURCE;
    solver->excitation.amplitude = amplitude;
    solver->excitation.k_vector = *position;
    solver->excitation.polarization = *polarization;
    return 0;
}

// Add port template to solver
int mom_solver_add_port(mom_solver_t* solver, struct extended_port_t* port) {
    if (!solver || !port) return -1;
    
    // Grow ports array if needed
    if (solver->num_ports >= solver->ports_capacity) {
        int new_capacity = (solver->ports_capacity == 0) ? 4 : solver->ports_capacity * 2;
        extended_port_t** new_ports = (extended_port_t**)realloc(solver->ports, new_capacity * sizeof(extended_port_t*));
        if (!new_ports) return -1;
        solver->ports = new_ports;
        solver->ports_capacity = new_capacity;
    }
    
    // Cast to match the typedef type used in the struct
    solver->ports[solver->num_ports++] = (extended_port_t*)(void*)port;
    return 0;
}

/* Mesh element → triangle_element_t (same layout as mom_solver_unified) */
static void mom_mesh_elem_to_triangle_element_rhs(const mesh_element_t* elem, const mesh_vertex_t* verts, triangle_element_t* out) {
    if (!elem || !verts || !out) return;
    for (int k = 0; k < 3; k++) {
        int vi = elem->vertices[k];
        out->vertices[k][0] = verts[vi].position.x;
        out->vertices[k][1] = verts[vi].position.y;
        out->vertices[k][2] = verts[vi].position.z;
    }
    out->normal[0] = elem->normal.x;
    out->normal[1] = elem->normal.y;
    out->normal[2] = elem->normal.z;
    out->area = elem->area > 0.0 ? elem->area : 0.0;
}

static int mom_rwg_opposite_vertex_xyz_rhs(const mesh_t* mesh, int tri_idx, int edge_idx, double o[3]) {
    if (!mesh || tri_idx < 0 || tri_idx >= mesh->num_elements || edge_idx < 0 || edge_idx >= mesh->num_edges || !o) return -1;
    const mesh_element_t* el = &mesh->elements[tri_idx];
    const mesh_edge_t* ed = &mesh->edges[edge_idx];
    int a = ed->vertex1_id;
    int b = ed->vertex2_id;
    for (int k = 0; k < 3 && k < el->num_vertices; k++) {
        int v = el->vertices[k];
        if (v != a && v != b) {
            o[0] = mesh->vertices[v].position.x;
            o[1] = mesh->vertices[v].position.y;
            o[2] = mesh->vertices[v].position.z;
            return 0;
        }
    }
    return -1;
}

typedef struct {
    triangle_element_t tri_p, tri_m;
    int has_p, has_m;
    double opp_p[3];
    double opp_m[3];
} MomRwgEdgeGeomRhs;

static void mom_rwg_edge_geom_fill_rhs(const mesh_t* mesh, const mesh_element_t* elements,
    const mesh_vertex_t* vertices, const int* edge_plus, const int* edge_minus, int ei,
    MomRwgEdgeGeomRhs* g) {
    if (!g) return;
    memset(g, 0, sizeof(*g));
    int tp = edge_plus[ei];
    int tm = edge_minus[ei];
    g->has_p = (tp >= 0 && tp < mesh->num_elements);
    g->has_m = (tm >= 0 && tm < mesh->num_elements);
    if (g->has_p) {
        mom_mesh_elem_to_triangle_element_rhs(&elements[tp], vertices, &g->tri_p);
        mom_rwg_opposite_vertex_xyz_rhs(mesh, tp, ei, g->opp_p);
    }
    if (g->has_m) {
        mom_mesh_elem_to_triangle_element_rhs(&elements[tm], vertices, &g->tri_m);
        mom_rwg_opposite_vertex_xyz_rhs(mesh, tm, ei, g->opp_m);
    }
}

// Build RWG mapping: edge -> adjacent triangles
static int build_rwg_mapping_local(const mesh_t* mesh, int* edge_plus, int* edge_minus, double* edge_length) {
    if (!mesh || mesh->num_edges <= 0 || !mesh->edges) return -1;
    for (int e = 0; e < mesh->num_edges; e++) {
        const mesh_edge_t* edge = &mesh->edges[e];
        edge_length[e] = edge->length > 0.0 ? edge->length : 0.0;
        edge_plus[e] = -1;
        edge_minus[e] = -1;
    }
    // Traverse triangles to fill plus/minus incidence
    for (int t = 0; t < mesh->num_elements; t++) {
        const mesh_element_t* elem = &mesh->elements[t];
        if (elem->type != MESH_ELEMENT_TRIANGLE || elem->num_edges <= 0) continue;
        for (int k = 0; k < elem->num_edges && k < 3; k++) {
            int eidx = elem->edges[k];
            if (eidx < 0 || eidx >= mesh->num_edges) continue;
            // Decide orientation by comparing vertex ordering
            const mesh_edge_t* e = &mesh->edges[eidx];
            int v0 = elem->vertices[k];
            int v1 = elem->vertices[(k + 1) % elem->num_vertices];
            bool same_dir = (e->vertex1_id == v0 && e->vertex2_id == v1);
            if (same_dir) {
                edge_plus[eidx] = t;
            } else {
                edge_minus[eidx] = t;
            }
        }
    }
    return 0;
}

/**
 * RHS for RWG EFIE: V_e = ∫ f_e · E_inc dS using same geometry as Z assembly (explicit opposite verts).
 */
static void mom_solver_fill_plane_wave_rwg_rhs(mom_solver_t* solver) {
    if (!solver || !solver->mesh || !solver->excitation_vector) return;
    mesh_t* mesh = solver->mesh;
    const size_t n = (size_t)solver->num_unknowns;
    if (mesh->num_edges <= 0 || (int)n != mesh->num_edges) return;

    double freq = (solver->config.frequency > 0.0) ? solver->config.frequency :
                  ((solver->excitation.frequency > 0.0) ? solver->excitation.frequency : 1e9);

    double amp = (solver->excitation.amplitude != 0.0) ? solver->excitation.amplitude : 1.0;
    double px = solver->excitation.polarization.x;
    double py = solver->excitation.polarization.y;
    double pz = solver->excitation.polarization.z;
    double pnorm = sqrt(px * px + py * py + pz * pz);
    if (pnorm < 1e-15) {
        px = 1.0;
        py = 0.0;
        pz = 0.0;
    } else {
        px /= pnorm;
        py /= pnorm;
        pz /= pnorm;
    }
    double E0[3] = { amp * px, amp * py, amp * pz };

    double kx = solver->excitation.k_vector.x;
    double ky = solver->excitation.k_vector.y;
    double kz = solver->excitation.k_vector.z;
    double kn = sqrt(kx * kx + ky * ky + kz * kz);
    if (kn < 1e-15) {
        kx = 0.0;
        ky = 0.0;
        kz = 1.0;
    } else {
        kx /= kn;
        ky /= kn;
        kz /= kn;
    }
    double k_hat[3] = { kx, ky, kz };

    const double ph = solver->excitation.phase;
    const complex_t rot = { cos(ph), sin(ph) };

    int* edge_plus = (int*)calloc((size_t)mesh->num_edges, sizeof(int));
    int* edge_minus = (int*)calloc((size_t)mesh->num_edges, sizeof(int));
    double* edge_len = (double*)calloc((size_t)mesh->num_edges, sizeof(double));
    if (!edge_plus || !edge_minus || !edge_len) {
        free(edge_plus);
        free(edge_minus);
        free(edge_len);
        return;
    }
    if (build_rwg_mapping_local(mesh, edge_plus, edge_minus, edge_len) != 0) {
        free(edge_plus);
        free(edge_minus);
        free(edge_len);
        return;
    }

    const mesh_element_t* elements = mesh->elements;
    const mesh_vertex_t* vertices = mesh->vertices;
    const mesh_edge_t* medges = mesh->edges;

    for (int e = 0; e < mesh->num_edges && e < (int)n; e++) {
        MomRwgEdgeGeomRhs G;
        mom_rwg_edge_geom_fill_rhs(mesh, elements, vertices, edge_plus, edge_minus, e, &G);
        double len_e = edge_len[e];
        if (len_e <= 0.0 && e < mesh->num_edges) {
            len_e = medges[e].length;
        }
        if (len_e <= 0.0) {
            len_e = 1e-6;
        }

        complex_t Vi = integrate_rwg_plane_wave_edge_geom(
            G.has_p ? &G.tri_p : NULL,
            G.has_p ? G.opp_p : NULL,
            G.has_p,
            G.has_m ? &G.tri_m : NULL,
            G.has_m ? G.opp_m : NULL,
            G.has_m,
            len_e,
            E0,
            k_hat,
            freq);

        /* Global phase e^{j φ0} on top of exp(-j k·r) inside the integral reference */
        complex_t out;
        out.re = Vi.re * rot.re - Vi.im * rot.im;
        out.im = Vi.re * rot.im + Vi.im * rot.re;
        solver->excitation_vector[e] = out;
    }

    free(edge_plus);
    free(edge_minus);
    free(edge_len);
}

// Compute RWG basis function vector at a point on a triangle
static void compute_rwg_basis_at_point(
    const mesh_t* mesh,
    int edge_idx,
    int tri_idx,
    int is_plus,
    const geom_point_t* r_point,
    double* basis_vec) {
    
    if (!mesh || !r_point || !basis_vec) {
        if (basis_vec) { basis_vec[0] = basis_vec[1] = basis_vec[2] = 0.0; }
        return;
    }
    
    const mesh_element_t* tri = &mesh->elements[tri_idx];
    if (tri->type != MESH_ELEMENT_TRIANGLE || tri->num_vertices < 3) {
        basis_vec[0] = basis_vec[1] = basis_vec[2] = 0.0;
        return;
    }
    
    const mesh_edge_t* edge = &mesh->edges[edge_idx];
    double edge_len = edge->length > 0.0 ? edge->length : 0.0;
    double tri_area = tri->area > 0.0 ? tri->area : 1e-15;
    
    if (tri_area < 1e-15 || edge_len < 1e-15) {
        basis_vec[0] = basis_vec[1] = basis_vec[2] = 0.0;
        return;
    }
    
    // Find opposite vertex (vertex not on the edge)
    int opp_vertex_idx = -1;
    for (int v = 0; v < tri->num_vertices; v++) {
        int vidx = tri->vertices[v];
        if (vidx != edge->vertex1_id && vidx != edge->vertex2_id) {
            opp_vertex_idx = vidx;
            break;
        }
    }
    
    if (opp_vertex_idx < 0 || opp_vertex_idx >= mesh->num_vertices) {
        basis_vec[0] = basis_vec[1] = basis_vec[2] = 0.0;
        return;
    }
    
    const geom_point_t* r_opp = &mesh->vertices[opp_vertex_idx].position;
    double coeff = edge_len / (2.0 * tri_area);
    
    if (is_plus) {
        // f_n(r) = (l_n / (2*A_n+)) * (r - r_n+)
        basis_vec[0] = coeff * (r_point->x - r_opp->x);
        basis_vec[1] = coeff * (r_point->y - r_opp->y);
        basis_vec[2] = coeff * (r_point->z - r_opp->z);
    } else {
        // f_n(r) = (l_n / (2*A_n-)) * (r_n- - r)
        basis_vec[0] = coeff * (r_opp->x - r_point->x);
        basis_vec[1] = coeff * (r_opp->y - r_point->y);
        basis_vec[2] = coeff * (r_opp->z - r_point->z);
    }
}

// Map port templates to RHS excitation vector using strict RWG basis integration
static int mom_solver_map_ports_to_rhs(mom_solver_t* solver) {
    if (!solver || !solver->mesh || !solver->excitation_vector) return -1;
    if (solver->num_ports == 0) return 0; // No ports, use default excitation
    
    const size_t n = (size_t)solver->num_unknowns;
    double freq = (solver->config.frequency > 0.0) ? solver->config.frequency :
                  ((solver->excitation.frequency > 0.0) ? solver->excitation.frequency : 1e9);
    
    // Clear RHS first
    for (size_t i = 0; i < n; i++) {
        solver->excitation_vector[i] = complex_zero();
    }
    
    // Build RWG mapping: edge -> adjacent triangles
    int* edge_plus = (int*)calloc(solver->mesh->num_edges, sizeof(int));
    int* edge_minus = (int*)calloc(solver->mesh->num_edges, sizeof(int));
    double* edge_length = (double*)calloc(solver->mesh->num_edges, sizeof(double));
    if (!edge_plus || !edge_minus || !edge_length) {
        free(edge_plus); free(edge_minus); free(edge_length);
        return -1;
    }
    if (build_rwg_mapping_local(solver->mesh, edge_plus, edge_minus, edge_length) != 0) {
        free(edge_plus); free(edge_minus); free(edge_length);
        return -1;
    }
    
    // Map each port to RWG edges
    for (int p = 0; p < solver->num_ports; p++) {
        extended_port_t* port = solver->ports[p];
        if (!port || !port->is_active) continue;
        
        double amp = port->excitation_magnitude;
        double phase_deg = port->excitation_phase;
        double phase_rad = phase_deg * M_PI / 180.0;
        complex_t port_exc = {amp * cos(phase_rad), amp * sin(phase_rad)};
        
        // Get port impedance at frequency
        double Z_real, Z_imag;
        if (port_get_impedance_at_frequency(port, freq, &Z_real, &Z_imag) != 0) {
            Z_real = port->characteristic_impedance;
            Z_imag = 0.0;
        }
        
        // Port geometry
        geom_point_t port_pos = port->position;
        geom_point_t port_dir = port->direction;
        double port_width = port->width > 0.0 ? port->width : 1e-3;
        
        // Normalize direction
        double dir_norm = sqrt(port_dir.x*port_dir.x + port_dir.y*port_dir.y + port_dir.z*port_dir.z);
        if (dir_norm > 1e-12) {
            port_dir.x /= dir_norm;
            port_dir.y /= dir_norm;
            port_dir.z /= dir_norm;
        }
        
        // For transmission line ports, compute port field direction
        double E_field_dir[3] = {port_dir.x, port_dir.y, port_dir.z};
        if (port->port_type == PORT_TYPE_MICROSTRIP || 
            port->port_type == PORT_TYPE_STRIPLINE ||
            port->port_type == PORT_TYPE_COAXIAL) {
            double Z_mag = sqrt(Z_real*Z_real + Z_imag*Z_imag);
            if (Z_mag > 1e-12) {
                // Scale by impedance for voltage source
                port_exc.re /= Z_mag;
                port_exc.im /= Z_mag;
            }
        }
        
            // Find edges within port region and compute RWG basis projection using Gaussian quadrature
            int edges_found = 0;
            const int rhs_is_element = (solver->num_unknowns == solver->mesh->num_elements);
            for (int e = 0; e < solver->mesh->num_edges; e++) {
                const mesh_edge_t* edge = &solver->mesh->edges[e];
                if (edge->length < 1e-15) continue;
                
                // Get adjacent triangles
                int tri_plus = edge_plus[e];
                int tri_minus = edge_minus[e];
                
                if (tri_plus < 0 && tri_minus < 0) continue; // Edge not on surface
                
                // Check if edge is within port region (using edge midpoint for quick check)
                geom_point_t edge_mid = edge->midpoint;
                double dx = edge_mid.x - port_pos.x;
                double dy = edge_mid.y - port_pos.y;
                double dz = edge_mid.z - port_pos.z;
                double dist = sqrt(dx*dx + dy*dy + dz*dz);
                
                if (dist > port_width * 1.5) continue; // Skip edges far from port
                
                // Integrate over triangles using 7-point Gaussian quadrature
                complex_t integral_plus = complex_zero();
                complex_t integral_minus = complex_zero();
                
                // 7-point Gaussian quadrature points and weights (barycentric coordinates)
                double gauss_xi[7] = {1.0/3.0, 0.797426985353087, 0.101286507323456, 0.101286507323456,
                                      0.059715871789770, 0.470142064105115, 0.470142064105115};
                double gauss_eta[7] = {1.0/3.0, 0.101286507323456, 0.797426985353087, 0.101286507323456,
                                       0.470142064105115, 0.059715871789770, 0.470142064105115};
                double gauss_weights[7] = {0.225, 0.125939180544827, 0.125939180544827, 0.125939180544827,
                                          0.132394152788506, 0.132394152788506, 0.132394152788506};
                
                // Integrate over plus triangle
                if (tri_plus >= 0) {
                    const mesh_element_t* tri = &solver->mesh->elements[tri_plus];
                    if (tri->type == MESH_ELEMENT_TRIANGLE && tri->num_vertices >= 3) {
                        const geom_point_t* v0 = &solver->mesh->vertices[tri->vertices[0]].position;
                        const geom_point_t* v1 = &solver->mesh->vertices[tri->vertices[1]].position;
                        const geom_point_t* v2 = &solver->mesh->vertices[tri->vertices[2]].position;
                        
                        for (int gp = 0; gp < 7; gp++) {
                            double xi = gauss_xi[gp];
                            double eta = gauss_eta[gp];
                            double zeta = 1.0 - xi - eta;
                            double weight = gauss_weights[gp];
                            
                            // Interpolate position: r = xi*v0 + eta*v1 + zeta*v2
                            geom_point_t r_point;
                            r_point.x = xi * v0->x + eta * v1->x + zeta * v2->x;
                            r_point.y = xi * v0->y + eta * v1->y + zeta * v2->y;
                            r_point.z = xi * v0->z + eta * v1->z + zeta * v2->z;
                            
                            // Check if integration point is within port region
                            double dx_gp = r_point.x - port_pos.x;
                            double dy_gp = r_point.y - port_pos.y;
                            double dz_gp = r_point.z - port_pos.z;
                            double dist_gp = sqrt(dx_gp*dx_gp + dy_gp*dy_gp + dz_gp*dz_gp);
                            
                            if (dist_gp < port_width * 1.5) {
                                // Compute RWG basis function at integration point
                                double basis_vec[3];
                                compute_rwg_basis_at_point(solver->mesh, e, tri_plus, 1, &r_point, basis_vec);
                                
                                // Dot product: E_field_dir · f_i(r)
                                double dot_prod = E_field_dir[0]*basis_vec[0] + E_field_dir[1]*basis_vec[1] + E_field_dir[2]*basis_vec[2];
                                
                                // Weight by distance (Gaussian falloff)
                                double dist_weight = exp(-dist_gp * dist_gp / (port_width * port_width * 0.5));
                                
                                // Accumulate: weight * area * dot_prod * dist_weight
                                double contrib = weight * tri->area * dot_prod * dist_weight;
                                integral_plus.re += contrib;
                            }
                        }
                    }
                }
                
                // Integrate over minus triangle
                if (tri_minus >= 0) {
                    const mesh_element_t* tri = &solver->mesh->elements[tri_minus];
                    if (tri->type == MESH_ELEMENT_TRIANGLE && tri->num_vertices >= 3) {
                        const geom_point_t* v0 = &solver->mesh->vertices[tri->vertices[0]].position;
                        const geom_point_t* v1 = &solver->mesh->vertices[tri->vertices[1]].position;
                        const geom_point_t* v2 = &solver->mesh->vertices[tri->vertices[2]].position;
                        
                        for (int gp = 0; gp < 7; gp++) {
                            double xi = gauss_xi[gp];
                            double eta = gauss_eta[gp];
                            double zeta = 1.0 - xi - eta;
                            double weight = gauss_weights[gp];
                            
                            // Interpolate position
                            geom_point_t r_point;
                            r_point.x = xi * v0->x + eta * v1->x + zeta * v2->x;
                            r_point.y = xi * v0->y + eta * v1->y + zeta * v2->y;
                            r_point.z = xi * v0->z + eta * v1->z + zeta * v2->z;
                            
                            // Check if integration point is within port region
                            double dx_gp = r_point.x - port_pos.x;
                            double dy_gp = r_point.y - port_pos.y;
                            double dz_gp = r_point.z - port_pos.z;
                            double dist_gp = sqrt(dx_gp*dx_gp + dy_gp*dy_gp + dz_gp*dz_gp);
                            
                            if (dist_gp < port_width * 1.5) {
                                // Compute RWG basis function at integration point
                                double basis_vec[3];
                                compute_rwg_basis_at_point(solver->mesh, e, tri_minus, 0, &r_point, basis_vec);
                                
                                // Dot product: E_field_dir · f_i(r)
                                double dot_prod = E_field_dir[0]*basis_vec[0] + E_field_dir[1]*basis_vec[1] + E_field_dir[2]*basis_vec[2];
                                
                                // Weight by distance
                                double dist_weight = exp(-dist_gp * dist_gp / (port_width * port_width * 0.5));
                                
                                // Accumulate
                                double contrib = weight * tri->area * dot_prod * dist_weight;
                                integral_minus.re += contrib;
                            }
                        }
                    }
                }
                
                // Total integral: I_n = I_plus + I_minus
                complex_t I_n;
                I_n.re = integral_plus.re + integral_minus.re;
                I_n.im = integral_plus.im + integral_minus.im;
                
                // Contribution: port_exc * I_n
                if (fabs(I_n.re) > 1e-15 || fabs(I_n.im) > 1e-15) {
                    complex_t contrib;
                    contrib.re = port_exc.re * I_n.re - port_exc.im * I_n.im;
                    contrib.im = port_exc.re * I_n.im + port_exc.im * I_n.re;
                    
                    if (rhs_is_element) {
                        /* 三角形未知：将边端口激励均分到相邻两三角形 RHS（启发式） */
                        int hit = 0;
                        if (tri_plus >= 0 && tri_plus < solver->mesh->num_elements) {
                            solver->excitation_vector[tri_plus].re += 0.5 * contrib.re;
                            solver->excitation_vector[tri_plus].im += 0.5 * contrib.im;
                            hit = 1;
                        }
                        if (tri_minus >= 0 && tri_minus < solver->mesh->num_elements) {
                            solver->excitation_vector[tri_minus].re += 0.5 * contrib.re;
                            solver->excitation_vector[tri_minus].im += 0.5 * contrib.im;
                            hit = 1;
                        }
                        if (hit) edges_found++;
                    } else if (e < (int)n) {
                        solver->excitation_vector[e].re += contrib.re;
                        solver->excitation_vector[e].im += contrib.im;
                        edges_found++;
                    }
                }
            }
        
        // Fallback: if no edges found, use simplified element-based mapping
        if (edges_found == 0) {
            for (int i = 0; i < solver->mesh->num_elements && i < (int)n; i++) {
                const mesh_element_t* elem = &solver->mesh->elements[i];
                if (elem->type != MESH_ELEMENT_TRIANGLE) continue;
                
                geom_point_t elem_center = elem->centroid;
                double dx = elem_center.x - port_pos.x;
                double dy = elem_center.y - port_pos.y;
                double dz = elem_center.z - port_pos.z;
                double dist = sqrt(dx*dx + dy*dy + dz*dz);
                
                if (dist < port_width) {
                    double weight = exp(-dist * dist / (port_width * port_width * 0.5));
                    complex_t contrib = {port_exc.re * weight, port_exc.im * weight};
                    solver->excitation_vector[i].re += contrib.re;
                    solver->excitation_vector[i].im += contrib.im;
                }
            }
        }
    }
    
    free(edge_plus);
    free(edge_minus);
    free(edge_length);
    
    return 0;
}

static int mom_solver_allocate_linear_system(mom_solver_t* solver) {
    if (!solver || solver->num_unknowns <= 0) return -1;
    size_t n = (size_t)solver->num_unknowns;
    mom_solver_free_buffers(solver);
    solver->excitation_vector = (complex_t*)calloc(n, sizeof(complex_t));
    solver->result.current_coefficients = (mom_scalar_complex_t*)calloc(n, sizeof(mom_scalar_complex_t));
    solver->result.current_magnitude = (double*)calloc(n, sizeof(double));
    solver->result.current_phase = (double*)calloc(n, sizeof(double));
    if (!solver->excitation_vector || !solver->result.current_coefficients ||
        !solver->result.current_magnitude || !solver->result.current_phase) {
        mom_solver_free_buffers(solver);
        return -1;
    }
    solver->result.num_basis_functions = solver->num_unknowns;
    return 0;
}

int mom_solver_assemble_matrix(mom_solver_t* solver) {
    if (!solver || !solver->mesh) return -1;
    if (solver->mesh->num_elements <= 0) return -1;
    double freq = (solver->config.frequency > 0.0) ? solver->config.frequency :
                  ((solver->excitation.frequency > 0.0) ? solver->excitation.frequency : 1e9);

    /* 与 mom_solve_unified 一致：EFIE → RWG 边；MFIE/CFIE → 脉冲三角（kernel_formulation 在 unified 内为 0/1/2） */
    if (solver->mesh->num_edges > 0) {
        if (solver->config.formulation == MOM_FORMULATION_EFIE || (int)solver->config.formulation == 0) {
            solver->num_unknowns = solver->mesh->num_edges;
        } else {
            solver->num_unknowns = solver->mesh->num_elements;
        }
    } else {
        solver->num_unknowns = solver->mesh->num_elements;
    }

    if (mom_solver_allocate_linear_system(solver) != 0) return -1;
    const size_t n = (size_t)solver->num_unknowns;
    
    /* RWG + EFIE + plane wave: V_i = ∫ f_i·E_inc dS (matches Z explicit Galerkin). Else: uniform scalar (pulse/MFIE fallback). */
    const int rwg_plane_wave_rhs =
        (solver->mesh->num_edges > 0 && (int)n == solver->mesh->num_edges &&
         (solver->config.formulation == MOM_FORMULATION_EFIE || (int)solver->config.formulation == 0) &&
         solver->excitation.type == MOM_EXCITATION_PLANE_WAVE);

    // Map ports to RHS if ports are defined, otherwise use default excitation
    if (solver->num_ports > 0) {
        if (mom_solver_map_ports_to_rhs(solver) != 0) {
            if (rwg_plane_wave_rhs) {
                mom_solver_fill_plane_wave_rwg_rhs(solver);
            } else {
                const double amp = (solver->excitation.amplitude != 0.0) ? solver->excitation.amplitude : 1.0;
                const double phase = solver->excitation.phase;
                complex_t exc = {amp * cos(phase), amp * sin(phase)};
                for (size_t i = 0; i < n; i++) {
                    solver->excitation_vector[i] = exc;
                }
            }
        }
    } else if (rwg_plane_wave_rhs) {
        mom_solver_fill_plane_wave_rwg_rhs(solver);
    } else {
        const double amp = (solver->excitation.amplitude != 0.0) ? solver->excitation.amplitude : 1.0;
        const double phase = solver->excitation.phase;
        complex_t exc = {amp * cos(phase), amp * sin(phase)};
        for (size_t i = 0; i < n; i++) {
            solver->excitation_vector[i] = exc;
        }
    }

    solver->result.matrix_fill_time = 0.0; // unified path handles assembly internally
    return 0;
}

int mom_solver_solve(mom_solver_t* solver) {
    if (!solver || !solver->excitation_vector || !solver->mesh) return -1;
    if (solver->num_unknowns <= 0) return -1;
    double freq = (solver->config.frequency > 0.0) ? solver->config.frequency :
                  ((solver->excitation.frequency > 0.0) ? solver->excitation.frequency : 1e9);

    int status = mom_solve_unified(solver->mesh, freq, solver->excitation_vector,
                                   (complex_t*)solver->result.current_coefficients, NULL);
    if (status != 0) {
        return status;
    }

    size_t n = (size_t)solver->num_unknowns;
    for (size_t i = 0; i < n; i++) {
        double re = solver->result.current_coefficients[i].re;
        double im = solver->result.current_coefficients[i].im;
        solver->result.current_magnitude[i] = hypot(re, im);
        solver->result.current_phase[i] = atan2(im, re);
    }

    solver->result.iterations = 1;
    solver->result.converged = true;
    solver->result.solve_time = 0.0;
    return 0;
}

int mom_solver_enable_preconditioner(mom_solver_t* solver, bool enable) {
    (void)solver; (void)enable;
    return 0;
}

int mom_solver_compute_port_current(mom_solver_t* solver, double px, double py, double width, int layer_index, double* Iport) {
    if (!solver || !Iport) return -1;
    if (!solver->mesh || !solver->result.current_coefficients) return -1;
    
    // Find port by position if not provided by ID
    extended_port_t* port = NULL;
    if (solver->num_ports > 0) {
        for (int p = 0; p < solver->num_ports; p++) {
            if (solver->ports[p]) {
                double dx = solver->ports[p]->position.x - px;
                double dy = solver->ports[p]->position.y - py;
                double dist = sqrt(dx*dx + dy*dy);
                if (dist < width * 2.0) { // Within port width
                    port = solver->ports[p];
                    break;
                }
            }
        }
    }
    
    // Compute current by integrating over port region
    double current_sum = 0.0;
    double weight_sum = 0.0;
    const size_t n = (size_t)solver->num_unknowns;
    
    for (int i = 0; i < solver->mesh->num_elements && i < (int)n; i++) {
        const mesh_element_t* elem = &solver->mesh->elements[i];
        if (elem->type != MESH_ELEMENT_TRIANGLE) continue;
        
        // Compute element center
        geom_point_t elem_center;
        elem_center.x = 0.0; elem_center.y = 0.0; elem_center.z = 0.0;
        if (elem->num_vertices > 0 && solver->mesh->vertices) {
            for (int v = 0; v < elem->num_vertices; v++) {
                int vidx = elem->vertices[v];
                if (vidx >= 0 && vidx < solver->mesh->num_vertices) {
                    elem_center.x += solver->mesh->vertices[vidx].position.x;
                    elem_center.y += solver->mesh->vertices[vidx].position.y;
                    elem_center.z += solver->mesh->vertices[vidx].position.z;
                }
            }
            double inv_nv = 1.0 / elem->num_vertices;
            elem_center.x *= inv_nv;
            elem_center.y *= inv_nv;
            elem_center.z *= inv_nv;
        }
        
        // Distance from port to element
        double dx = elem_center.x - px;
        double dy = elem_center.y - py;
        double dist = sqrt(dx*dx + dy*dy);
        
        if (dist < width * 1.5) { // Port region
            double weight = exp(-dist * dist / (width * width * 0.5));
            double current_mag = solver->result.current_magnitude[i];
            current_sum += current_mag * weight * elem->area;
            weight_sum += weight * elem->area;
        }
    }
    
    if (weight_sum > 1e-12) {
        *Iport = current_sum / weight_sum;
    } else {
        // Fallback: average magnitude
        double avg = 0.0;
        if (solver->result.current_magnitude && solver->num_unknowns > 0) {
            for (int i = 0; i < solver->num_unknowns; i++) avg += solver->result.current_magnitude[i];
            avg /= (double)solver->num_unknowns;
        }
        *Iport = avg;
    }
    
    return 0;
}

int mom_solver_compute_near_field(mom_solver_t* solver, const point3d_t* points, int num_points) {
    if (!solver || !points || num_points <= 0) return -1;
    if (!solver->result.current_coefficients || solver->num_unknowns <= 0) return -1;
    free(solver->result.near_field.e_field);
    free(solver->result.near_field.h_field);
    solver->result.near_field.num_points = num_points;
    solver->result.near_field.e_field = (mom_scalar_complex_t*)calloc((size_t)num_points * 3, sizeof(mom_scalar_complex_t));
    solver->result.near_field.h_field = (mom_scalar_complex_t*)calloc((size_t)num_points * 3, sizeof(mom_scalar_complex_t));
    if (!solver->result.near_field.e_field || !solver->result.near_field.h_field) return -1;

    double freq = (solver->config.frequency > 0.0) ? solver->config.frequency :
                  ((solver->excitation.frequency > 0.0) ? solver->excitation.frequency : 1e9);
    double k = 2.0 * M_PI * freq / C0;
    point3d_t kvec = solver->excitation.k_vector;
    double amp = (solver->excitation.amplitude != 0.0) ? solver->excitation.amplitude : 1.0;
    const double inv_4pi = 1.0 / (4.0 * M_PI);
    const double min_R = 1e-10;

    /* Incident plane wave */
    for (int i = 0; i < num_points; i++) {
        double phase = kvec.x * points[i].x + kvec.y * points[i].y + kvec.z * points[i].z;
        double c = cos(phase), s = sin(phase);
        solver->result.near_field.e_field[i*3 + 0].re = amp * solver->excitation.polarization.x * c;
        solver->result.near_field.e_field[i*3 + 0].im = amp * solver->excitation.polarization.x * s;
        solver->result.near_field.e_field[i*3 + 1].re = amp * solver->excitation.polarization.y * c;
        solver->result.near_field.e_field[i*3 + 1].im = amp * solver->excitation.polarization.y * s;
        solver->result.near_field.e_field[i*3 + 2].re = amp * solver->excitation.polarization.z * c;
        solver->result.near_field.e_field[i*3 + 2].im = amp * solver->excitation.polarization.z * s;
        double nx = kvec.x, ny = kvec.y, nz = kvec.z;
        double exr = solver->result.near_field.e_field[i*3 + 0].re;
        double eyr = solver->result.near_field.e_field[i*3 + 1].re;
        double ezr = solver->result.near_field.e_field[i*3 + 2].re;
        solver->result.near_field.h_field[i*3 + 0].re = (ny * ezr - nz * eyr) / ETA0;
        solver->result.near_field.h_field[i*3 + 1].re = (nz * exr - nx * ezr) / ETA0;
        solver->result.near_field.h_field[i*3 + 2].re = (nx * eyr - ny * exr) / ETA0;
    }

    /* Scattered E from surface currents (point-source approximation per triangle) so |E| varies in space */
    if (solver->mesh && solver->mesh->vertices && solver->mesh->elements) {
        const mesh_t* m = solver->mesh;
        int n_elems = (m->num_elements < solver->num_unknowns) ? m->num_elements : solver->num_unknowns;
        double coeff = k * ETA0 * inv_4pi;
        for (int i = 0; i < num_points; i++) {
            double px = points[i].x, py = points[i].y, pz = points[i].z;
            double scat_re = 0.0, scat_im = 0.0;
            for (int j = 0; j < n_elems; j++) {
                const mesh_element_t* e = &m->elements[j];
                if (e->type != MESH_ELEMENT_TRIANGLE || e->num_vertices < 3) continue;
                double cx = e->centroid.x, cy = e->centroid.y, cz = e->centroid.z;
                double dx = px - cx, dy = py - cy, dz = pz - cz;
                double R = sqrt(dx*dx + dy*dy + dz*dz);
                if (R < min_R) continue;
                double J_re = solver->result.current_coefficients[j].re;
                double J_im = solver->result.current_coefficients[j].im;
                double area = (e->area > 0.0) ? e->area : 0.0;
                if (area <= 0.0) continue;
                double kr = k * R;
                double exp_re = cos(kr), exp_im = -sin(kr);
                double fac = coeff * area / R;
                /* -j * (fac * exp(-j*k*R) * J) along polarization: scat = -j * fac * (exp_re + j*exp_im)*(J_re + j*J_im) */
                double re_part = fac * (exp_re * J_re - exp_im * J_im);
                double im_part = fac * (exp_im * J_re + exp_re * J_im);
                scat_re += im_part;   /* (-j * contrib).re = contrib_im */
                scat_im -= re_part;   /* (-j * contrib).im = -contrib_re */
            }
            double pol_x = solver->excitation.polarization.x;
            double pol_y = solver->excitation.polarization.y;
            double pol_z = solver->excitation.polarization.z;
            solver->result.near_field.e_field[i*3 + 0].re += scat_re * pol_x;
            solver->result.near_field.e_field[i*3 + 0].im += scat_im * pol_x;
            solver->result.near_field.e_field[i*3 + 1].re += scat_re * pol_y;
            solver->result.near_field.e_field[i*3 + 1].im += scat_im * pol_y;
            solver->result.near_field.e_field[i*3 + 2].re += scat_re * pol_z;
            solver->result.near_field.e_field[i*3 + 2].im += scat_im * pol_z;
        }
        /* H = (1/ETA0)*k̂×E from total E (incident + scattered) */
        for (int i = 0; i < num_points; i++) {
            double exr = solver->result.near_field.e_field[i*3 + 0].re;
            double eyr = solver->result.near_field.e_field[i*3 + 1].re;
            double ezr = solver->result.near_field.e_field[i*3 + 2].re;
            double nx = kvec.x, ny = kvec.y, nz = kvec.z;
            solver->result.near_field.h_field[i*3 + 0].re = (ny * ezr - nz * eyr) / ETA0;
            solver->result.near_field.h_field[i*3 + 1].re = (nz * exr - nx * ezr) / ETA0;
            solver->result.near_field.h_field[i*3 + 2].re = (nx * eyr - ny * exr) / ETA0;
        }
    }
    return 0;
}

int mom_solver_compute_far_field(mom_solver_t* solver, double theta_min, double theta_max, int n_theta,
                                 double phi_min, double phi_max, int n_phi) {
    if (!solver || n_theta <= 0 || n_phi <= 0) return -1;
    if (!solver->result.current_coefficients || solver->num_unknowns <= 0) return -1;
    free(solver->result.far_field.theta_angles);
    free(solver->result.far_field.phi_angles);
    free(solver->result.far_field.e_theta);
    free(solver->result.far_field.e_phi);
    int total = n_theta * n_phi;
    solver->result.far_field.num_theta = n_theta;
    solver->result.far_field.num_phi = n_phi;
    solver->result.far_field.theta_angles = (double*)calloc(total, sizeof(double));
    solver->result.far_field.phi_angles = (double*)calloc(total, sizeof(double));
    solver->result.far_field.e_theta = (mom_scalar_complex_t*)calloc(total, sizeof(mom_scalar_complex_t));
    solver->result.far_field.e_phi = (mom_scalar_complex_t*)calloc(total, sizeof(mom_scalar_complex_t));
    if (!solver->result.far_field.theta_angles || !solver->result.far_field.phi_angles ||
        !solver->result.far_field.e_theta || !solver->result.far_field.e_phi) {
        return -1;
    }
    // Fill sampling angles and compute far field via unified kernel
    complex_t* ff = (complex_t*)calloc((size_t)total, sizeof(complex_t));
    if (!ff) return -1;
    mom_compute_radiation_pattern_unified(solver->mesh, (complex_t*)solver->result.current_coefficients,
                                          (solver->config.frequency > 0.0) ? solver->config.frequency : solver->excitation.frequency,
                                          theta_min, theta_max, phi_min, phi_max, n_theta, n_phi, ff);
    int idx = 0;
    for (int i = 0; i < n_theta; i++) {
        double t = theta_min + (theta_max - theta_min) * ((double)i / (double)(n_theta - 1));
        for (int j = 0; j < n_phi; j++) {
            double p = phi_min + (phi_max - phi_min) * ((double)j / (double)(n_phi - 1));
            solver->result.far_field.theta_angles[idx] = t;
            solver->result.far_field.phi_angles[idx] = p;
            solver->result.far_field.e_theta[idx].re = ff[idx].re;
            solver->result.far_field.e_theta[idx].im = ff[idx].im;
            solver->result.far_field.e_phi[idx].re = ff[idx].re;
            solver->result.far_field.e_phi[idx].im = ff[idx].im;
            idx++;
        }
    }
    free(ff);
    return 0;
}

const mom_result_t* mom_solver_get_results(const mom_solver_t* solver) { if (!solver) return NULL; return &solver->result; }
int mom_solver_get_num_unknowns(const mom_solver_t* solver) { return solver ? solver->num_unknowns : 0; }
double mom_solver_get_memory_usage(const mom_solver_t* solver) {
    if (!solver) return 0.0;
    size_t bytes = 0;
    size_t n = (size_t)solver->num_unknowns;
    bytes += n * sizeof(complex_t); // excitation_vector
    bytes += n * (sizeof(mom_scalar_complex_t) + sizeof(double) * 2);
    return (double)bytes;
}

int mom_solver_export_surface_current(const mom_solver_t* solver, const char* csv_path) {
    if (!solver || !csv_path) return -1;
    if (!solver->mesh || !solver->result.current_coefficients) return -1;
    FILE* f = fopen(csv_path, "w");
    if (!f) return -1;

    fprintf(f, "element_index,cx,cy,cz,Re_J,Im_J,magnitude_J\n");
    const int n = solver->num_unknowns;
    const int num_elem = solver->mesh->num_elements;
    const int count = (n < num_elem) ? n : num_elem;
    for (int i = 0; i < count; ++i) {
        const mesh_element_t* elem = &solver->mesh->elements[i];
        if (!elem) continue;
        geom_point_t c = elem->centroid;
        complex_t J = solver->result.current_coefficients[i];
        double re = J.re;
        double im = J.im;
        double mag = sqrt(re * re + im * im);
        fprintf(f, "%d,%.12e,%.12e,%.12e,%.12e,%.12e,%.12e\n",
                i, c.x, c.y, c.z, re, im, mag);
    }

    fclose(f);
    return 0;
}

int mom_solver_export_surface_mesh_for_plot(const mom_solver_t* solver, const char* txt_path) {
    if (!solver || !txt_path || !solver->mesh || !solver->result.current_coefficients) return -1;
    const mesh_t* m = solver->mesh;
    if (!m->vertices || !m->elements) return -1;

    int num_tri = 0;
    for (int i = 0; i < m->num_elements; i++) {
        const mesh_element_t* e = &m->elements[i];
        if (e->type == MESH_ELEMENT_TRIANGLE && e->num_vertices >= 3 && e->vertices) num_tri++;
    }
    if (num_tri == 0) return -1;

    FILE* f = fopen(txt_path, "w");
    if (!f) return -1;

    fprintf(f, "%d %d\n", m->num_vertices, num_tri);
    for (int i = 0; i < m->num_vertices; i++) {
        const geom_point_t* p = &m->vertices[i].position;
        fprintf(f, "%.12e %.12e %.12e\n", p->x, p->y, p->z);
    }

    const int n = solver->num_unknowns;
    for (int i = 0; i < m->num_elements && i < n; i++) {
        const mesh_element_t* e = &m->elements[i];
        if (e->type != MESH_ELEMENT_TRIANGLE || e->num_vertices < 3 || !e->vertices) continue;
        double re = solver->result.current_coefficients[i].re;
        double im = solver->result.current_coefficients[i].im;
        double mag = sqrt(re * re + im * im);
        int a = e->vertices[0], b = e->vertices[1], c = e->vertices[2];
        if (a < 0) a = 0; if (a >= m->num_vertices) a = m->num_vertices - 1;
        if (b < 0) b = 0; if (b >= m->num_vertices) b = m->num_vertices - 1;
        if (c < 0) c = 0; if (c >= m->num_vertices) c = m->num_vertices - 1;
        fprintf(f, "%d %d %d %.12e\n", a, b, c, mag);
    }

    fclose(f);
    return 0;
}

int mom_solver_export_surface_current_vtk(const mom_solver_t* solver, const char* vtk_path, int conductor_material_id) {
    if (!solver || !vtk_path || !solver->mesh || !solver->result.current_coefficients) return -1;
    const mesh_t* m = solver->mesh;
    if (m->num_elements <= 0 || !m->elements || m->num_vertices <= 0) return -1;

    /* 单元未知（与 mom_solve_unified 三角形装配一致）：系数按三角形给 |J| 可视化 */
    if (solver->num_unknowns == m->num_elements) {
        postprocessing_current_distribution_t cur_dist = {0};
        cur_dist.num_elements = m->num_elements;
        cur_dist.frequency = (solver->config.frequency > 0.0) ? solver->config.frequency : solver->excitation.frequency;
        cur_dist.current_magnitude = (double*)malloc((size_t)m->num_elements * sizeof(double));
        if (!cur_dist.current_magnitude) return -1;
        for (int i = 0; i < m->num_elements; i++) {
            mom_scalar_complex_t Ie = solver->result.current_coefficients[i];
            double mag = hypot(Ie.re, Ie.im);
            if (conductor_material_id >= 0 && m->elements[i].material_id != conductor_material_id) {
                mag = 0.0;
            }
            cur_dist.current_magnitude[i] = mag;
        }
        {
            double* v_sum = (double*)calloc((size_t)m->num_vertices, sizeof(double));
            int* v_count = (int*)calloc((size_t)m->num_vertices, sizeof(int));
            if (v_sum && v_count) {
                for (int i = 0; i < m->num_elements; i++) {
                    const mesh_element_t* e = &m->elements[i];
                    if (!e->vertices || e->num_vertices < 3) continue;
                    double mag = cur_dist.current_magnitude[i];
                    for (int k = 0; k < e->num_vertices && k < 3; k++) {
                        int v = e->vertices[k];
                        if (v >= 0 && v < m->num_vertices) {
                            v_sum[v] += mag;
                            v_count[v]++;
                        }
                    }
                }
                cur_dist.current_magnitude_vertices = (double*)malloc((size_t)m->num_vertices * sizeof(double));
                if (cur_dist.current_magnitude_vertices) {
                    cur_dist.num_vertices = m->num_vertices;
                    for (int v = 0; v < m->num_vertices; v++) {
                        cur_dist.current_magnitude_vertices[v] =
                            (v_count[v] > 0) ? (v_sum[v] / (double)v_count[v]) : 0.0;
                    }
                    const int smooth_iters = 2;
                    double* v_new = (double*)malloc((size_t)m->num_vertices * sizeof(double));
                    if (v_new) {
                        for (int it = 0; it < smooth_iters; it++) {
                            memset(v_sum, 0, (size_t)m->num_vertices * sizeof(double));
                            memset(v_count, 0, (size_t)m->num_vertices * sizeof(int));
                            for (int i = 0; i < m->num_elements; i++) {
                                const mesh_element_t* e = &m->elements[i];
                                if (!e->vertices || e->num_vertices < 3) continue;
                                int nv = e->num_vertices;
                                if (nv > 3) nv = 3;
                                for (int a = 0; a < nv; a++) {
                                    int va = e->vertices[a];
                                    if (va < 0 || va >= m->num_vertices) continue;
                                    for (int b = 0; b < nv; b++) {
                                        int vb = e->vertices[b];
                                        if (vb < 0 || vb >= m->num_vertices) continue;
                                        v_sum[va] += cur_dist.current_magnitude_vertices[vb];
                                        v_count[va]++;
                                    }
                                }
                            }
                            for (int v = 0; v < m->num_vertices; v++) {
                                if (v_count[v] > 0) {
                                    v_new[v] = v_sum[v] / (double)v_count[v];
                                } else {
                                    v_new[v] = cur_dist.current_magnitude_vertices[v];
                                }
                            }
                            memcpy(cur_dist.current_magnitude_vertices, v_new,
                                   (size_t)m->num_vertices * sizeof(double));
                        }
                        free(v_new);
                    }
                }
            }
            free(v_sum);
            free(v_count);
        }
        export_vtk_options_t vtk_opts = export_vtk_get_default_options();
        int ret = export_vtk_current(&cur_dist, m, vtk_path, &vtk_opts);
        free(cur_dist.current_magnitude);
        free(cur_dist.current_magnitude_vertices);
        return ret;
    }

    if (m->num_edges <= 0 || !m->edges) return -1;

    /* 使用 RWG 边基的电流系数，在三角形质心和顶点上显式评估 J(r)。 */

    /* 1) 构建 RWG 映射：每条边的 plus/minus 三角形，以及边长 */
    int* edge_plus = (int*)calloc(m->num_edges, sizeof(int));
    int* edge_minus = (int*)calloc(m->num_edges, sizeof(int));
    double* edge_length = (double*)calloc(m->num_edges, sizeof(double));
    if (!edge_plus || !edge_minus || !edge_length) {
        free(edge_plus); free(edge_minus); free(edge_length);
        return -1;
    }
    if (build_rwg_mapping_local(m, edge_plus, edge_minus, edge_length) != 0) {
        free(edge_plus); free(edge_minus); free(edge_length);
        return -1;
    }

    postprocessing_current_distribution_t cur_dist = {0};
    cur_dist.num_elements = m->num_elements;
    cur_dist.frequency = (solver->config.frequency > 0.0) ? solver->config.frequency : solver->excitation.frequency;
    cur_dist.current_magnitude = (double*)malloc((size_t)m->num_elements * sizeof(double));
    cur_dist.current_real = (double*)calloc((size_t)m->num_elements * 3u, sizeof(double));
    cur_dist.current_imag = (double*)calloc((size_t)m->num_elements * 3u, sizeof(double));
    cur_dist.current_phase = NULL;
    cur_dist.current_magnitude_vertices = NULL;
    cur_dist.num_vertices = 0;
    if (!cur_dist.current_magnitude || !cur_dist.current_real || !cur_dist.current_imag) {
        free(cur_dist.current_magnitude);
        free(cur_dist.current_real);
        free(cur_dist.current_imag);
        free(edge_plus); free(edge_minus); free(edge_length);
        return -1;
    }

    /* 2) 在每个三角形质心上用 RWG 展开评估 J(r)=Σ_e I_e f_e(r)。
     * 对复系数 I_e：J 的笛卡尔分量亦为复数；分别存 Re/Im，再取 |J|=sqrt(Σ_k |J_k|^2)。 */

    /* 方便访问的指针 */
    const mesh_element_t* elements = m->elements;
    const mesh_vertex_t* vertices = m->vertices;

    /* 系数个数 = num_unknowns = 边基数量（已在 set_mesh/unified 中切换） */
    const int n_edges = solver->num_unknowns;

    for (int e = 0; e < n_edges && e < m->num_edges; e++) {
        /* 边电流系数 I_e（避免与 <complex.h> 中的 I 宏冲突，这里不用变量名 I） */
        mom_scalar_complex_t Ie = solver->result.current_coefficients[e];
#if defined(_MSC_VER)
        double I_re = Ie.re;
        double I_im = Ie.im;
#else
        double I_re = creal(Ie);
        double I_im = cimag(Ie);
#endif

        /* 该边在 plus、minus 两个三角形上的贡献 */
        int t_plus = edge_plus[e];
        int t_minus = edge_minus[e];
        double len = edge_length[e] > 0.0 ? edge_length[e] : 0.0;

        /* plus 三角形贡献 */
        if (t_plus >= 0 && t_plus < m->num_elements && len > 0.0) {
            const mesh_element_t* tri = &elements[t_plus];
            if (tri->type == MESH_ELEMENT_TRIANGLE && tri->num_vertices >= 3 && tri->vertices) {
                /* 三角顶点坐标 */
                int v0 = tri->vertices[0];
                int v1 = tri->vertices[1];
                int v2 = tri->vertices[2];
                const geom_point_t* p0 = &vertices[v0].position;
                const geom_point_t* p1 = &vertices[v1].position;
                const geom_point_t* p2 = &vertices[v2].position;

                /* 三角面积（若 mesh 已给出 area，直接用；否则用叉积估算） */
                double A = tri->area;
                if (A <= 0.0) {
                    double ax = p1->x - p0->x;
                    double ay = p1->y - p0->y;
                    double az = p1->z - p0->z;
                    double bx = p2->x - p0->x;
                    double by = p2->y - p0->y;
                    double bz = p2->z - p0->z;
                    double cx = ay * bz - az * by;
                    double cy = az * bx - ax * bz;
                    double cz = ax * by - ay * bx;
                    A = 0.5 * sqrt(cx*cx + cy*cy + cz*cz);
                }
                if (A > 0.0) {
                    /* 边端点 */
                    const mesh_edge_t* edge = &m->edges[e];
                    int ev0 = edge->vertex1_id;
                    int ev1 = edge->vertex2_id;

                    /* plus 三角中与该边相对的自由顶点 free_v */
                    int free_v = -1;
                    if (v0 != ev0 && v0 != ev1) free_v = v0;
                    else if (v1 != ev0 && v1 != ev1) free_v = v1;
                    else free_v = v2;

                    const geom_point_t* pf = &vertices[free_v].position;

                    /* 质心 r_c */
                    geom_point_t c = tri->centroid;
                    if (c.x == 0.0 && c.y == 0.0 && c.z == 0.0) {
                        c.x = (p0->x + p1->x + p2->x) / 3.0;
                        c.y = (p0->y + p1->y + p2->y) / 3.0;
                        c.z = (p0->z + p1->z + p2->z) / 3.0;
                    }

                    /* RWG 定义：f_e(r) = (l_e / (2A+)) * (r - r_free) on plus 三角 */
                    double rx = c.x - pf->x;
                    double ry = c.y - pf->y;
                    double rz = c.z - pf->z;
                    double scale = len / (2.0 * A);

                    double Jx = scale * rx;
                    double Jy = scale * ry;
                    double Jz = scale * rz;

                    /* (I_re+j I_im) * (Jx,Jy,Jz)，f_e 在质心取值为实矢量 */
                    const int b = t_plus * 3;
                    cur_dist.current_real[b + 0] += I_re * Jx;
                    cur_dist.current_imag[b + 0] += I_im * Jx;
                    cur_dist.current_real[b + 1] += I_re * Jy;
                    cur_dist.current_imag[b + 1] += I_im * Jy;
                    cur_dist.current_real[b + 2] += I_re * Jz;
                    cur_dist.current_imag[b + 2] += I_im * Jz;
                }
            }
        }

        /* minus 三角形贡献（符号相反） */
        if (t_minus >= 0 && t_minus < m->num_elements && len > 0.0) {
            const mesh_element_t* tri = &elements[t_minus];
            if (tri->type == MESH_ELEMENT_TRIANGLE && tri->num_vertices >= 3 && tri->vertices) {
                int v0 = tri->vertices[0];
                int v1 = tri->vertices[1];
                int v2 = tri->vertices[2];
                const geom_point_t* p0 = &vertices[v0].position;
                const geom_point_t* p1 = &vertices[v1].position;
                const geom_point_t* p2 = &vertices[v2].position;

                double A = tri->area;
                if (A <= 0.0) {
                    double ax = p1->x - p0->x;
                    double ay = p1->y - p0->y;
                    double az = p1->z - p0->z;
                    double bx = p2->x - p0->x;
                    double by = p2->y - p0->y;
                    double bz = p2->z - p0->z;
                    double cx = ay * bz - az * by;
                    double cy = az * bx - ax * bz;
                    double cz = ax * by - ay * bx;
                    A = 0.5 * sqrt(cx*cx + cy*cy + cz*cz);
                }
                if (A > 0.0) {
                    const mesh_edge_t* edge = &m->edges[e];
                    int ev0 = edge->vertex1_id;
                    int ev1 = edge->vertex2_id;

                    int free_v = -1;
                    if (v0 != ev0 && v0 != ev1) free_v = v0;
                    else if (v1 != ev0 && v1 != ev1) free_v = v1;
                    else free_v = v2;

                    const geom_point_t* pf = &vertices[free_v].position;

                    geom_point_t c = tri->centroid;
                    if (c.x == 0.0 && c.y == 0.0 && c.z == 0.0) {
                        c.x = (p0->x + p1->x + p2->x) / 3.0;
                        c.y = (p0->y + p1->y + p2->y) / 3.0;
                        c.z = (p0->z + p1->z + p2->z) / 3.0;
                    }

                    /* RWG 定义：f_e(r) = -(l_e / (2A-)) * (r - r_free) on minus 三角 */
                    double rx = c.x - pf->x;
                    double ry = c.y - pf->y;
                    double rz = c.z - pf->z;
                    double scale = -len / (2.0 * A);

                    double Jx = scale * rx;
                    double Jy = scale * ry;
                    double Jz = scale * rz;

                    const int b = t_minus * 3;
                    cur_dist.current_real[b + 0] += I_re * Jx;
                    cur_dist.current_imag[b + 0] += I_im * Jx;
                    cur_dist.current_real[b + 1] += I_re * Jy;
                    cur_dist.current_imag[b + 1] += I_im * Jy;
                    cur_dist.current_real[b + 2] += I_re * Jz;
                    cur_dist.current_imag[b + 2] += I_im * Jz;
                }
            }
        }
    }

    /* 3) |J| = sqrt( Σ_k (Re(J_k)^2 + Im(J_k)^2) )；按需要屏蔽非导体区域 */
    for (int i = 0; i < m->num_elements; i++) {
        const int b = i * 3;
        double s2 = 0.0;
        for (int k = 0; k < 3; k++) {
            double jr = cur_dist.current_real[b + k];
            double ji = cur_dist.current_imag[b + k];
            s2 += jr * jr + ji * ji;
        }
        double mag = sqrt(s2);
        if (conductor_material_id >= 0 && m->elements[i].material_id != conductor_material_id) {
            mag = 0.0;
            for (int k = 0; k < 3; k++) {
                cur_dist.current_real[b + k] = 0.0;
                cur_dist.current_imag[b + k] = 0.0;
            }
        }
        cur_dist.current_magnitude[i] = mag;
    }

    /* 4) 从单元 |J| 构造顶点场并做轻微平滑，作为 POINT_DATA 用于平滑显示 */
    {
        double* v_sum = (double*)calloc((size_t)m->num_vertices, sizeof(double));
        int* v_count = (int*)calloc((size_t)m->num_vertices, sizeof(int));
        if (v_sum && v_count) {
            /* 4.1 从单元常数场构造顶点场（顶点值 = 共享该顶点单元上 |J| 的平均） */
            for (int i = 0; i < m->num_elements; i++) {
                const mesh_element_t* e = &m->elements[i];
                if (!e->vertices || e->num_vertices < 3) continue;
                double mag = cur_dist.current_magnitude[i];
                for (int k = 0; k < e->num_vertices && k < 3; k++) {
                    int v = e->vertices[k];
                    if (v >= 0 && v < m->num_vertices) {
                        v_sum[v] += mag;
                        v_count[v]++;
                    }
                }
            }
            cur_dist.current_magnitude_vertices = (double*)malloc((size_t)m->num_vertices * sizeof(double));
            if (cur_dist.current_magnitude_vertices) {
                cur_dist.num_vertices = m->num_vertices;
                for (int v = 0; v < m->num_vertices; v++) {
                    cur_dist.current_magnitude_vertices[v] =
                        (v_count[v] > 0) ? (v_sum[v] / (double)v_count[v]) : 0.0;
                }

                /* 4.2 在顶点场上做少量拉普拉斯平滑（仅用于可视化，不改变求解结果） */
                const int smooth_iters = 2;
                double* v_new = (double*)malloc((size_t)m->num_vertices * sizeof(double));
                if (v_new) {
                    for (int it = 0; it < smooth_iters; it++) {
                        memset(v_sum, 0, (size_t)m->num_vertices * sizeof(double));
                        memset(v_count, 0, (size_t)m->num_vertices * sizeof(int));

                        for (int i = 0; i < m->num_elements; i++) {
                            const mesh_element_t* e = &m->elements[i];
                            if (!e->vertices || e->num_vertices < 3) continue;
                            int nv = e->num_vertices;
                            if (nv > 3) nv = 3;
                            for (int a = 0; a < nv; a++) {
                                int va = e->vertices[a];
                                if (va < 0 || va >= m->num_vertices) continue;
                                for (int b = 0; b < nv; b++) {
                                    int vb = e->vertices[b];
                                    if (vb < 0 || vb >= m->num_vertices) continue;
                                    v_sum[va] += cur_dist.current_magnitude_vertices[vb];
                                    v_count[va]++;
                                }
                            }
                        }

                        for (int v = 0; v < m->num_vertices; v++) {
                            if (v_count[v] > 0) {
                                v_new[v] = v_sum[v] / (double)v_count[v];
                            } else {
                                v_new[v] = cur_dist.current_magnitude_vertices[v];
                            }
                        }
                        memcpy(cur_dist.current_magnitude_vertices, v_new,
                               (size_t)m->num_vertices * sizeof(double));
                    }
                    free(v_new);
                }
            }
        }
        free(v_sum);
        free(v_count);
    }

    export_vtk_options_t vtk_opts = export_vtk_get_default_options();
    int ret = export_vtk_current(&cur_dist, m, vtk_path, &vtk_opts);

    free(edge_plus); free(edge_minus); free(edge_length);
    free(cur_dist.current_magnitude);
    free(cur_dist.current_real);
    free(cur_dist.current_imag);
    free(cur_dist.current_magnitude_vertices);
    return ret;
}

int mom_solver_set_layered_medium(mom_solver_t* solver,
                                  const LayeredMedium* medium,
                                  const FrequencyDomain* freq,
                                  const GreensFunctionParams* params) {
    (void)solver; (void)medium; (void)freq; (void)params;
    return 0;
}

int mom_solver_import_msh(mom_solver_t* solver, const char* filename) {
    mesh_t* mesh = NULL;
    if (!solver || !filename) return -1;
    if (gmsh_import_msh(filename, &mesh) != 0 || !mesh) return -1;
    if (mom_solver_set_mesh(solver, mesh) != 0) {
        mesh_destroy(mesh);
        return -1;
    }
    return 0;
}

int mom_solver_import_cad(mom_solver_t* solver, const char* filename, const char* format) {
    if (!solver || !filename || !format) {
        return -1;
    }

    /* ------------------------------------------------------------------
     * 1) Use OpenCascade to validate CAD file and get basic geometry info
     * ------------------------------------------------------------------ */
    opencascade_import_params_t occt_params;
    memset(&occt_params, 0, sizeof(occt_params));
    occt_params.heal_geometry = 1;
    occt_params.healing_precision = 1e-6;
    occt_params.max_tolerance = 1e-4;

    opencascade_geometry_t occt_geom;
    memset(&occt_geom, 0, sizeof(occt_geom));

    if (!opencascade_import_cad(filename, &occt_params, &occt_geom)) {
        fprintf(stderr, "[MoM] OpenCascade CAD import failed for file: %s\n", filename);
        return -1;
    }

    /* ------------------------------------------------------------------
     * 2) Touch CAD mesh generation pipeline (for future refinement)
     *    Currently used only for parameter setup and logging.
     * ------------------------------------------------------------------ */
    MeshGenerationConfig mesh_cfg;
    memset(&mesh_cfg, 0, sizeof(mesh_cfg));
    CadMeshGenerationSolver* cad_solver = cad_mesh_generation_create(&mesh_cfg);
    if (cad_solver) {
        /* FORMAT_UNKNOWN is sufficient here because OpenCascade detects the
         * actual STEP/IGES/STL format internally. */
        cad_mesh_generation_import_cad_file(cad_solver, filename, FORMAT_UNKNOWN);
        cad_mesh_generation_destroy(cad_solver);
    }

    /* ------------------------------------------------------------------
     * 3) Try Gmsh for real surface triangulation; fallback to minimal 2-tri stub
     * ------------------------------------------------------------------ */
    mesh_t* mesh = NULL;
    {
        double freq = (solver->config.frequency > 0.0) ? solver->config.frequency : 1e9;
        int density = (solver->config.mesh_density > 0) ? solver->config.mesh_density : 10;
        double wavelength = (3e8 / freq);  /* metres */
        double characteristic_length = wavelength / (double)density;

        if (gmsh_import_surface_mesh(filename, characteristic_length, &mesh) == 0 && mesh != NULL) {
            if (mom_solver_set_mesh(solver, mesh) != 0) {
                mesh_destroy(mesh);
                return -1;
            }
            fprintf(stderr, "[MoM] Using Gmsh surface mesh: %d vertices, %d triangles\n",
                    mesh->num_vertices, mesh->num_elements);
            return 0;
        }
        fprintf(stderr, "[MoM] Gmsh mesh failed or unavailable; using minimal 2-triangle mesh (3D surface current will show only 2 elements).\n");
    }

    /* ------------------------------------------------------------------
     * 4) Build minimal MoM-compatible surface mesh (two triangles) from bbox
     * ------------------------------------------------------------------ */
    mesh = mesh_create("mom_cad_mesh", MESH_TYPE_TRIANGULAR);
    if (!mesh) {
        return -1;
    }

    /* Derive a simple patch from the CAD bounding box.
     * If the bounding box is degenerate, fall back to a unit square. */
    double xmin = occt_geom.bounding_box[0];
    double ymin = occt_geom.bounding_box[1];
    double zmin = occt_geom.bounding_box[2];
    double xmax = occt_geom.bounding_box[3];
    double ymax = occt_geom.bounding_box[4];
    double zmax = occt_geom.bounding_box[5];

    if (xmax <= xmin) { xmax = xmin + 1.0; }
    if (ymax <= ymin) { ymax = ymin + 1.0; }
    if (zmax <= zmin) { zmax = zmin + 1.0; }

    /* Place a rectangular patch on the minimum-Z plane. */
    geom_point_t vpos[4];
    vpos[0].x = xmin; vpos[0].y = ymin; vpos[0].z = zmin;
    vpos[1].x = xmax; vpos[1].y = ymin; vpos[1].z = zmin;
    vpos[2].x = xmax; vpos[2].y = ymax; vpos[2].z = zmin;
    vpos[3].x = xmin; vpos[3].y = ymax; vpos[3].z = zmin;

    mesh->num_vertices = 4;
    mesh->vertices = (mesh_vertex_t*)calloc((size_t)mesh->num_vertices, sizeof(mesh_vertex_t));
    if (!mesh->vertices) {
        mesh_destroy(mesh);
        return -1;
    }

    for (int i = 0; i < 4; i++) {
        mesh->vertices[i].id = i;
        mesh->vertices[i].position = vpos[i];
        mesh->vertices[i].is_boundary = true;
        mesh->vertices[i].is_interface = false;
        mesh->vertices[i].adjacent_elements = NULL;
        mesh->vertices[i].num_adjacent_elements = 0;
    }

    /* Define edges for a rectangle with one diagonal shared by two triangles.
     *
     *  v3 -------- v2
     *  |        /  |
     *  |      /    |
     *  |    /      |
     *  v0 -------- v1
     *
     * Tri0: (v0, v1, v2)
     * Tri1: (v0, v2, v3)
     */
    mesh->num_edges = 5;
    mesh->edges = (mesh_edge_t*)calloc((size_t)mesh->num_edges, sizeof(mesh_edge_t));
    if (!mesh->edges) {
        mesh_destroy(mesh);
        return -1;
    }

    int edge_vertices[5][2] = {
        {0, 1}, /* e0 */
        {1, 2}, /* e1 */
        {2, 3}, /* e2 */
        {3, 0}, /* e3 */
        {0, 2}  /* e4 - diagonal */
    };

    for (int e = 0; e < 5; e++) {
        mesh_edge_t* edge = &mesh->edges[e];
        edge->id = e;
        edge->vertex1_id = edge_vertices[e][0];
        edge->vertex2_id = edge_vertices[e][1];
        edge->is_boundary = (e != 4); /* diagonal is interior edge */
        edge->is_interface = false;

        geom_point_t* a = &mesh->vertices[edge->vertex1_id].position;
        geom_point_t* b = &mesh->vertices[edge->vertex2_id].position;
        edge->midpoint.x = 0.5 * (a->x + b->x);
        edge->midpoint.y = 0.5 * (a->y + b->y);
        edge->midpoint.z = 0.5 * (a->z + b->z);

        double dx = b->x - a->x;
        double dy = b->y - a->y;
        double dz = b->z - a->z;
        edge->length = sqrt(dx*dx + dy*dy + dz*dz);
        if (edge->length > 0.0) {
            edge->tangent.x = dx / edge->length;
            edge->tangent.y = dy / edge->length;
            edge->tangent.z = dz / edge->length;
        }
    }

    /* Two triangular surface elements covering the rectangle. */
    mesh->num_elements = 2;
    mesh->elements = (mesh_element_t*)calloc((size_t)mesh->num_elements, sizeof(mesh_element_t));
    if (!mesh->elements) {
        mesh_destroy(mesh);
        return -1;
    }

    /* Helper to fill a triangle element. */
    int tri_vertices[2][3] = {
        {0, 1, 2}, /* Tri0 */
        {0, 2, 3}  /* Tri1 */
    };
    int tri_edges[2][3] = {
        {0, 1, 4}, /* edges (0-1), (1-2), (0-2) */
        {4, 2, 3}  /* edges (0-2), (2-3), (3-0) */
    };

    for (int t = 0; t < 2; t++) {
        mesh_element_t* elem = &mesh->elements[t];
        elem->id = t;
        elem->type = MESH_ELEMENT_TRIANGLE;
        elem->num_vertices = 3;
        elem->num_edges = 3;

        elem->vertices = (int*)calloc(3, sizeof(int));
        elem->edges = (int*)calloc(3, sizeof(int));
        if (!elem->vertices || !elem->edges) {
            mesh_destroy(mesh);
            return -1;
        }

        for (int k = 0; k < 3; k++) {
            elem->vertices[k] = tri_vertices[t][k];
            elem->edges[k] = tri_edges[t][k];
        }

        /* Compute basic geometric properties: centroid, normal, area. */
        geom_point_t* p0 = &mesh->vertices[elem->vertices[0]].position;
        geom_point_t* p1 = &mesh->vertices[elem->vertices[1]].position;
        geom_point_t* p2 = &mesh->vertices[elem->vertices[2]].position;

        double ux = p1->x - p0->x;
        double uy = p1->y - p0->y;
        double uz = p1->z - p0->z;
        double vx = p2->x - p0->x;
        double vy = p2->y - p0->y;
        double vz = p2->z - p0->z;

        /* Cross product u x v */
        double nx = uy * vz - uz * vy;
        double ny = uz * vx - ux * vz;
        double nz = ux * vy - uy * vx;
        double nlen = sqrt(nx*nx + ny*ny + nz*nz);

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

    /* Basic mesh statistics */
    mesh->type = MESH_TYPE_TRIANGULAR;
    mesh->algorithm = MESH_ALGORITHM_DELAUNAY;
    mesh->quality.min_angle = 30.0;
    mesh->quality.max_angle = 120.0;
    mesh->quality.aspect_ratio = 1.0;
    mesh->quality.skewness = 0.0;
    mesh->quality.orthogonality = 1.0;
    mesh->min_element_size = fmin(mesh->elements[0].characteristic_length,
                                  mesh->elements[1].characteristic_length);
    mesh->max_element_size = fmax(mesh->elements[0].characteristic_length,
                                  mesh->elements[1].characteristic_length);
    mesh->average_element_size = 0.5 * (mesh->min_element_size + mesh->max_element_size);

    mesh->min_bound = vpos[0];
    mesh->max_bound.x = xmax;
    mesh->max_bound.y = ymax;
    mesh->max_bound.z = zmin;

    /* Attach mesh to solver */
    if (mom_solver_set_mesh(solver, mesh) != 0) {
        mesh_destroy(mesh);
        return -1;
    }

    return 0;
}
