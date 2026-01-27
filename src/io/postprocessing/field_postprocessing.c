/******************************************************************************
 * Field Post-Processing - Implementation
 ******************************************************************************/

#include "field_postprocessing.h"
#include "../../discretization/mesh/core_mesh.h"
#include "../../discretization/geometry/core_geometry.h"
#include "../../common/core_common.h"
#include "../file_formats/export_vtk.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

int postprocessing_field_export(
    const postprocessing_field_data_t* field_data,
    const mesh_t* mesh,
    const char* filename,
    postprocessing_export_format_t format
) {
    if (!field_data || !mesh || !filename) {
        return -1;
    }
    
    // Route to appropriate exporter
    switch (format) {
        case POSTPROCESSING_EXPORT_VTK: {
            // Use export_vtk module
            export_vtk_options_t opts = export_vtk_get_default_options();
            return export_vtk_field(field_data, mesh, filename, &opts);
        }
        case POSTPROCESSING_EXPORT_CSV: {
            FILE* fp = fopen(filename, "w");
            if (!fp) return -1;
            
            fprintf(fp, "x,y,z,Ex_real,Ex_imag,Ey_real,Ey_imag,Ez_real,Ez_imag\n");
            for (int i = 0; i < field_data->num_points; i++) {
                fprintf(fp, "%.6e,%.6e,%.6e",
                        field_data->positions[i * 3],
                        field_data->positions[i * 3 + 1],
                        field_data->positions[i * 3 + 2]);
                for (int j = 0; j < 3; j++) {
                    fprintf(fp, ",%.6e,%.6e",
                            field_data->e_field_real[i * 3 + j],
                            field_data->e_field_imag[i * 3 + j]);
                }
                fprintf(fp, "\n");
            }
            fclose(fp);
            return 0;
        }
        default:
            return -1;  // Unsupported format
    }
}

int postprocessing_current_export(
    const postprocessing_current_distribution_t* current_dist,
    const mesh_t* mesh,
    const char* filename,
    postprocessing_export_format_t format
) {
    if (!current_dist || !mesh || !filename) {
        return -1;
    }
    
    // Route to appropriate exporter
    switch (format) {
        case POSTPROCESSING_EXPORT_VTK: {
            export_vtk_options_t opts = export_vtk_get_default_options();
            return export_vtk_current(current_dist, mesh, filename, &opts);
        }
        case POSTPROCESSING_EXPORT_CSV: {
            FILE* fp = fopen(filename, "w");
            if (!fp) return -1;
            
            fprintf(fp, "x,y,z,Jx_real,Jx_imag,Jy_real,Jy_imag,Jz_real,Jz_imag\n");
            for (int i = 0; i < current_dist->num_elements; i++) {
                // Get element center from mesh
                if (i < mesh->num_elements && mesh->elements[i].num_vertices > 0) {
                    point3d_t center = {0.0, 0.0, 0.0};
                    for (int v = 0; v < mesh->elements[i].num_vertices; v++) {
                        int vidx = mesh->elements[i].vertices[v];
                        if (vidx < mesh->num_vertices) {
                            center.x += mesh->vertices[vidx].position.x;
                            center.y += mesh->vertices[vidx].position.y;
                            center.z += mesh->vertices[vidx].position.z;
                        }
                    }
                    center.x /= mesh->elements[i].num_vertices;
                    center.y /= mesh->elements[i].num_vertices;
                    center.z /= mesh->elements[i].num_vertices;
                    
                    fprintf(fp, "%.6e,%.6e,%.6e",
                            center.x, center.y, center.z);
                    
                    // Write current components
                    for (int j = 0; j < 3; j++) {
                        double real = 0.0, imag = 0.0;
                        if (i < current_dist->num_elements) {
                            real = current_dist->current_real[i * 3 + j];
                            imag = current_dist->current_imag[i * 3 + j];
                        }
                        fprintf(fp, ",%.6e,%.6e", real, imag);
                    }
                    fprintf(fp, "\n");
                }
            }
            fclose(fp);
            return 0;
        }
        default:
            return -1;  // Unsupported format
    }
}

int postprocessing_field_calculate_power_flow(
    const postprocessing_field_data_t* field_data,
    const double* surface_normal,
    int num_points,
    double* power_real,
    double* power_imag
) {
    if (!field_data || !surface_normal || !power_real || !power_imag) {
        return -1;
    }
    
    double P_real = 0.0, P_imag = 0.0;
    
    // Compute Poynting vector: S = 0.5 * E × H*
    for (int i = 0; i < num_points; i++) {
        // E field
        double Ex_r = field_data->e_field_real[i * 3];
        double Ex_i = field_data->e_field_imag[i * 3];
        double Ey_r = field_data->e_field_real[i * 3 + 1];
        double Ey_i = field_data->e_field_imag[i * 3 + 1];
        double Ez_r = field_data->e_field_real[i * 3 + 2];
        double Ez_i = field_data->e_field_imag[i * 3 + 2];
        
        // H field
        double Hx_r = field_data->h_field_real[i * 3];
        double Hx_i = field_data->h_field_imag[i * 3];
        double Hy_r = field_data->h_field_real[i * 3 + 1];
        double Hy_i = field_data->h_field_imag[i * 3 + 1];
        double Hz_r = field_data->h_field_real[i * 3 + 2];
        double Hz_i = field_data->h_field_imag[i * 3 + 2];
        
        // S = 0.5 * E × H*
        // Sx = 0.5 * (Ey * Hz* - Ez * Hy*)
        double Sx_r = 0.5 * (Ey_r * Hz_r + Ey_i * Hz_i - Ez_r * Hy_r - Ez_i * Hy_i);
        double Sx_i = 0.5 * (Ey_i * Hz_r - Ey_r * Hz_i - Ez_i * Hy_r + Ez_r * Hy_i);
        
        double Sy_r = 0.5 * (Ez_r * Hx_r + Ez_i * Hx_i - Ex_r * Hz_r - Ex_i * Hz_i);
        double Sy_i = 0.5 * (Ez_i * Hx_r - Ez_r * Hx_i - Ex_i * Hz_r + Ex_r * Hz_i);
        
        double Sz_r = 0.5 * (Ex_r * Hy_r + Ex_i * Hy_i - Ey_r * Hx_r - Ey_i * Hx_i);
        double Sz_i = 0.5 * (Ex_i * Hy_r - Ex_r * Hy_i - Ey_i * Hx_r + Ey_r * Hx_i);
        
        // Power flow = S · n
        double nx = surface_normal[i * 3];
        double ny = surface_normal[i * 3 + 1];
        double nz = surface_normal[i * 3 + 2];
        
        P_real += Sx_r * nx + Sy_r * ny + Sz_r * nz;
        P_imag += Sx_i * nx + Sy_i * ny + Sz_i * nz;
    }
    
    *power_real = P_real;
    *power_imag = P_imag;
    
    return 0;
}

int postprocessing_field_calculate_statistics(
    const postprocessing_field_data_t* field_data,
    double stats[4]  // [min, max, mean, rms]
) {
    if (!field_data || !stats) {
        return -1;
    }
    
    double min_val = 1e308;
    double max_val = -1e308;
    double sum = 0.0;
    double sum_sq = 0.0;
    int count = 0;
    
    // Compute statistics over all field components
    for (int i = 0; i < field_data->num_points; i++) {
        for (int j = 0; j < 3; j++) {
            double real = field_data->e_field_real[i * 3 + j];
            double imag = field_data->e_field_imag[i * 3 + j];
            double mag = sqrt(real * real + imag * imag);
            
            if (mag < min_val) min_val = mag;
            if (mag > max_val) max_val = mag;
            sum += mag;
            sum_sq += mag * mag;
            count++;
        }
    }
    
    if (count > 0) {
        stats[0] = min_val;
        stats[1] = max_val;
        stats[2] = sum / count;
        stats[3] = sqrt(sum_sq / count);
    } else {
        stats[0] = stats[1] = stats[2] = stats[3] = 0.0;
    }
    
    return 0;
}

int postprocessing_field_near_to_far(
    const postprocessing_field_data_t* near_field,
    const double* observation_angles,
    int num_angles,
    postprocessing_field_data_t* far_field
) {
    if (!near_field || !observation_angles || !far_field) {
        return -1;
    }
    
    // Allocate far-field data
    far_field->num_points = num_angles;
    far_field->e_field_real = (double*)calloc(num_angles * 3, sizeof(double));
    far_field->e_field_imag = (double*)calloc(num_angles * 3, sizeof(double));
    far_field->h_field_real = (double*)calloc(num_angles * 3, sizeof(double));
    far_field->h_field_imag = (double*)calloc(num_angles * 3, sizeof(double));
    far_field->positions = (double*)calloc(num_angles * 3, sizeof(double));
    
    if (!far_field->e_field_real || !far_field->e_field_imag ||
        !far_field->h_field_real || !far_field->h_field_imag ||
        !far_field->positions) {
        return -1;
    }
    
    // Near-to-far-field transformation using standard formulas
    // E_far = (j*k*exp(-j*k*r)) / (4*π*r) * ∫ [n × (n × J) - η * n × M] * exp(j*k*r'·n) dS'
    // where J is electric current, M is magnetic current, n is observation direction,
    // r is distance, r' is source position, k is wavenumber, η is impedance
    
    // For simplicity, assume we have current distribution from near field
    // In practice, would need mesh and current coefficients
    
    // Simplified implementation: use phase factors and vector summation
    double k0 = 2.0 * M_PI * 1e9 / C0;  // Default wavenumber (1 GHz)
    double eta0 = ETA0;  // Free space impedance
    
    // For each observation angle
    for (int a = 0; a < num_angles; a++) {
        // Get observation direction from angles (theta, phi)
        double theta = observation_angles[a * 2] * M_PI / 180.0;  // Convert to radians
        double phi = observation_angles[a * 2 + 1] * M_PI / 180.0;
        
        double n[3] = {
            sin(theta) * cos(phi),
            sin(theta) * sin(phi),
            cos(theta)
        };
        
        // Set observation position (far field, large distance)
        double r_far = 1000.0;  // 1 km (far field distance)
        far_field->positions[a * 3] = r_far * n[0];
        far_field->positions[a * 3 + 1] = r_far * n[1];
        far_field->positions[a * 3 + 2] = r_far * n[2];
        
        // Compute far field by integrating near field contributions
        // Simplified: sum contributions from all near field points
        double E_far_r[3] = {0.0, 0.0, 0.0};
        double E_far_i[3] = {0.0, 0.0, 0.0};
        double H_far_r[3] = {0.0, 0.0, 0.0};
        double H_far_i[3] = {0.0, 0.0, 0.0};
        
        for (int i = 0; i < near_field->num_points; i++) {
            // Source position
            double r_src[3] = {
                near_field->positions[i * 3],
                near_field->positions[i * 3 + 1],
                near_field->positions[i * 3 + 2]
            };
            
            // Phase factor: exp(j*k*r'·n)
            double phase_arg = k0 * (r_src[0] * n[0] + r_src[1] * n[1] + r_src[2] * n[2]);
            double cos_phase = cos(phase_arg);
            double sin_phase = sin(phase_arg);
            
            // Near field values
            double En_r[3] = {
                near_field->e_field_real[i * 3],
                near_field->e_field_real[i * 3 + 1],
                near_field->e_field_real[i * 3 + 2]
            };
            double En_i[3] = {
                near_field->e_field_imag[i * 3],
                near_field->e_field_imag[i * 3 + 1],
                near_field->e_field_imag[i * 3 + 2]
            };
            
            // Far field contribution (simplified Green's function)
            // E_far ≈ (j*k*exp(-j*k*r)) / (4*π*r) * E_near * exp(j*k*r'·n)
            double factor = k0 / (4.0 * M_PI * r_far);
            
            // Multiply by phase factor
            for (int j = 0; j < 3; j++) {
                // Complex multiplication: (En_r + j*En_i) * (cos_phase + j*sin_phase)
                double En_phased_r = En_r[j] * cos_phase - En_i[j] * sin_phase;
                double En_phased_i = En_r[j] * sin_phase + En_i[j] * cos_phase;
                
                // Apply Green's function factor: j*k/(4πr) = k/(4πr) * (0 + j)
                E_far_r[j] += -factor * En_phased_i;  // Real part of j * complex
                E_far_i[j] += factor * En_phased_i;   // Imaginary part of j * complex
            }
        }
        
        // Store far field
        for (int j = 0; j < 3; j++) {
            far_field->e_field_real[a * 3 + j] = E_far_r[j];
            far_field->e_field_imag[a * 3 + j] = E_far_i[j];
            
            // H_far = n × E_far / η0
            int j1 = (j + 1) % 3;
            int j2 = (j + 2) % 3;
            far_field->h_field_real[a * 3 + j] = (n[j1] * E_far_r[j2] - n[j2] * E_far_r[j1]) / eta0;
            far_field->h_field_imag[a * 3 + j] = (n[j1] * E_far_i[j2] - n[j2] * E_far_i[j1]) / eta0;
        }
    }
    
    return 0;
}

int postprocessing_field_calculate_radiation_pattern(
    const postprocessing_field_data_t* far_field,
    const double* theta_angles,
    const double* phi_angles,
    int num_theta,
    int num_phi,
    double* gain_pattern
) {
    if (!far_field || !theta_angles || !phi_angles || !gain_pattern) {
        return -1;
    }
    
    // Calculate gain pattern from far-field data
    // Gain = 4π * r² * |E|² / P_rad
    
    // Calculate total radiated power
    double P_rad = 0.0;
    for (int i = 0; i < far_field->num_points; i++) {
        double Ex_r = far_field->e_field_real[i * 3];
        double Ex_i = far_field->e_field_imag[i * 3];
        double Ey_r = far_field->e_field_real[i * 3 + 1];
        double Ey_i = far_field->e_field_imag[i * 3 + 1];
        double Ez_r = far_field->e_field_real[i * 3 + 2];
        double Ez_i = far_field->e_field_imag[i * 3 + 2];
        
        double E_mag_sq = (Ex_r*Ex_r + Ex_i*Ex_i) + 
                         (Ey_r*Ey_r + Ey_i*Ey_i) + 
                         (Ez_r*Ez_r + Ez_i*Ez_i);
        P_rad += E_mag_sq;
    }
    
    if (P_rad <= 0.0) {
        P_rad = 1.0;  // Avoid division by zero
    }
    
    // Calculate gain for each angle
    int idx = 0;
    for (int t = 0; t < num_theta; t++) {
        for (int p = 0; p < num_phi; p++) {
            // Find corresponding field point (simplified)
            int field_idx = (t * num_phi + p) % far_field->num_points;
            
            double Ex_r = far_field->e_field_real[field_idx * 3];
            double Ex_i = far_field->e_field_imag[field_idx * 3];
            double Ey_r = far_field->e_field_real[field_idx * 3 + 1];
            double Ey_i = far_field->e_field_imag[field_idx * 3 + 1];
            double Ez_r = far_field->e_field_real[field_idx * 3 + 2];
            double Ez_i = far_field->e_field_imag[field_idx * 3 + 2];
            
            double E_mag_sq = (Ex_r*Ex_r + Ex_i*Ex_i) + 
                             (Ey_r*Ey_r + Ey_i*Ey_i) + 
                             (Ez_r*Ez_r + Ez_i*Ez_i);
            
            // Gain in dB: 10*log10(4π * E² / P_rad)
            double gain_linear = 4.0 * M_PI * E_mag_sq / P_rad;
            gain_pattern[idx] = 10.0 * log10(fmax(gain_linear, 1e-12));  // Avoid log(0)
            
            idx++;
        }
    }
    
    return 0;
}

int postprocessing_field_calculate_emc_metrics(
    const postprocessing_field_data_t* field_data,
    double frequency,
    double* metrics
) {
    if (!field_data || !metrics) {
        return -1;
    }
    
    // Calculate EMC/EMI metrics
    double max_E = 0.0;
    double max_H = 0.0;
    double total_power = 0.0;
    
    for (int i = 0; i < field_data->num_points; i++) {
        // E field magnitude
        double Ex = field_data->e_field_real[i * 3];
        double Ey = field_data->e_field_real[i * 3 + 1];
        double Ez = field_data->e_field_real[i * 3 + 2];
        double E_mag = sqrt(Ex*Ex + Ey*Ey + Ez*Ez);
        if (E_mag > max_E) max_E = E_mag;
        
        // H field magnitude
        double Hx = field_data->h_field_real[i * 3];
        double Hy = field_data->h_field_real[i * 3 + 1];
        double Hz = field_data->h_field_real[i * 3 + 2];
        double H_mag = sqrt(Hx*Hx + Hy*Hy + Hz*Hz);
        if (H_mag > max_H) max_H = H_mag;
    }
    
    metrics[0] = max_E;      // max_E_field
    metrics[1] = max_H;       // max_H_field
    metrics[2] = total_power; // total_power
    
    return 0;
}
