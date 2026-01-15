/*********************************************************************
 * PCB电磁仿真的GPU加速优化内核
 * 针对PCB几何特性的专用CUDA内核优化
 *********************************************************************/

#ifndef PCB_GPU_ACCELERATION_H
#define PCB_GPU_ACCELERATION_H

#include "cuda_runtime.h"
#include "device_launch_parameters.h"
#include "cublas_v2.h"
#include "cusolverDn.h"
#include <cuComplex.h>

// PCB几何加速结构
typedef struct {
    // PCB层信息
    int num_layers;
    double* layer_elevation;      // 层高度 (GPU)
    double* layer_thickness;      // 层厚度 (GPU)
    double* layer_conductivity;   // 层导电率 (GPU)
    double* layer_permittivity;   // 层介电常数 (GPU)
    
    // PCB几何数据
    int* layer_triangle_offsets;  // 每层的三角形偏移 (GPU)
    int* layer_triangle_counts;   // 每层的三角形数量 (GPU)
    double* pcb_vertices;         // PCB顶点坐标 (GPU)
    int* pcb_triangles;           // PCB三角形索引 (GPU)
    
    // 网格优化数据
    int* triangle_neighbors;      // 三角形邻居 (GPU)
    int* edge_connections;        // 边连接信息 (GPU)
    double* triangle_areas;       // 三角形面积 (GPU)
    
    // 电磁参数
    double* frequency_points;     // 频率点 (GPU)
    int num_frequencies;
    
    // 端口信息
    int* port_triangles;          // 端口三角形 (GPU)
    double* port_positions;       // 端口位置 (GPU)
    double* port_impedances;      // 端口阻抗 (GPU)
    int num_ports;
    
} PCBGPUAccelerationData;

// PCB专用CUDA内核配置
#define PCB_BLOCK_SIZE 128
#define PCB_WARP_SIZE 32
#define PCB_MAX_LAYERS 32
#define PCB_MAX_PORTS 64
#define PCB_SHARED_MEMORY_SIZE 16384  // 16KB共享内存

// PCB优化的CUDA内核声明

// 几何处理内核
__global__ void pcb_layer_geometry_kernel(
    const double* vertices, const int* triangles,
    const int* layer_offsets, const int* layer_counts,
    double* layer_areas, int num_layers, int num_triangles);

__global__ void pcb_triangle_area_kernel(
    const double* vertices, const int* triangles,
    double* areas, int num_triangles);

__global__ void pcb_edge_detection_kernel(
    const int* triangles, const double* vertices,
    int* edge_flags, int num_triangles, int num_vertices);

__global__ void pcb_via_connection_kernel(
    const double* via_positions, const double* via_radii,
    const int* layer_connections, const double* vertices,
    int* connection_matrix, int num_vias, int num_vertices);

// 电磁计算内核
__global__ void pcb_green_function_kernel(
    const double* source_points, const double* field_points,
    const double* layer_params, const double frequency,
    cuDoubleComplex* green_matrix, int num_sources, int num_fields);

__global__ void pcb_impedance_matrix_kernel(
    const double* triangles, const double* vertices,
    const double* areas, const double* conductivities,
    const double* permittivities, const double frequency,
    cuDoubleComplex* impedance_matrix, int num_triangles);

__global__ void pcb_sparameter_kernel(
    const cuDoubleComplex* impedance_matrix,
    const cuDoubleComplex* port_excitations,
    const double* port_impedances, const int* port_triangles,
    cuDoubleComplex* s_parameters, int matrix_size, int num_ports);

__global__ void pcb_current_distribution_kernel(
    const cuDoubleComplex* impedance_matrix,
    const cuDoubleComplex* voltage_vector,
    cuDoubleComplex* current_vector, int matrix_size);

__global__ void pcb_field_calculation_kernel(
    const cuDoubleComplex* current_distribution,
    const double* field_points, const double* source_triangles,
    const double* layer_params, const double frequency,
    cuDoubleComplex* electric_field, cuDoubleComplex* magnetic_field,
    int num_currents, int num_field_points);

// 优化算法内核
__global__ void pcb_adaptive_mesh_kernel(
    const double* error_estimates, const double* triangle_areas,
    const int* refinement_flags, int* new_triangles,
    int* refinement_count, double refinement_threshold,
    int num_triangles);

__global__ void pcb_matrix_precondition_kernel(
    cuDoubleComplex* matrix, const double* diagonal_scaling,
    int matrix_size, int preconditioner_type);

__global__ void pcb_iterative_solver_kernel(
    const cuDoubleComplex* matrix, const cuDoubleComplex* rhs,
    cuDoubleComplex* solution, double tolerance, int max_iterations,
    int* convergence_flag, int matrix_size);

// 多层PCB专用内核
__global__ void pcb_layered_green_function_kernel(
    const double* source_points, const double* field_points,
    const double* layer_elevations, const double* layer_thickness,
    const double* layer_conductivity, const double* layer_permittivity,
    const double frequency, cuDoubleComplex* green_function,
    int num_sources, int num_fields, int num_layers);

__global__ void pcb_coupling_calculation_kernel(
    const cuDoubleComplex* current_layer1, const cuDoubleComplex* current_layer2,
    const double* layer_distance, const double frequency,
    cuDoubleComplex* coupling_matrix, int num_triangles_layer1,
    int num_triangles_layer2);

// 频域扫描优化内核
__global__ void pcb_frequency_sweep_kernel(
    const double* frequencies, const cuDoubleComplex* frequency_response,
    cuDoubleComplex* broadband_response, int num_frequencies,
    int response_size, int sweep_type);

__global__ void pcb_sparameter_interpolation_kernel(
    const double* freq_points, const cuDoubleComplex* s_parameters,
    const double* target_frequencies, cuDoubleComplex* interpolated_sparameters,
    int num_original_points, int num_target_points, int num_ports);

// 内存管理函数
int pcb_gpu_allocate_memory(PCBGPUAccelerationData* gpu_data, int max_triangles, int max_vertices);
void pcb_gpu_free_memory(PCBGPUAccelerationData* gpu_data);
int pcb_gpu_copy_data_to_device(PCBGPUAccelerationData* gpu_data, const PCBEMModel* model);
int pcb_gpu_copy_results_to_host(PCBGPUAccelerationData* gpu_data, PCBEMSimulationResults* results);

// 性能优化函数
int pcb_optimize_block_size(int num_triangles);
double pcb_estimate_gpu_memory_requirements(const PCBEMModel* model);
double pcb_estimate_gpu_computation_time(const PCBEMModel* model);
int pcb_select_optimal_gpu_configuration(PCBGPUAccelerationData* gpu_data);

// 专用求解器函数
int pcb_gpu_solve_impedance_matrix(PCBGPUAccelerationData* gpu_data, 
                                   cuDoubleComplex* impedance_matrix, 
                                   cuDoubleComplex* rhs_vector, 
                                   cuDoubleComplex* solution_vector);

int pcb_gpu_calculate_sparameters(PCBGPUAccelerationData* gpu_data,
                                  const cuDoubleComplex* impedance_matrix,
                                  const PCBPortDefinition* ports,
                                  cuDoubleComplex* s_parameters);

int pcb_gpu_current_distribution(PCBGPUAccelerationData* gpu_data,
                               const cuDoubleComplex* impedance_matrix,
                               const cuDoubleComplex* port_excitations,
                               cuDoubleComplex* current_distribution);

// 验证和调试函数
int pcb_validate_gpu_results(const PCBEMSimulationResults* gpu_results,
                           const PCBEMSimulationResults* cpu_results,
                           double tolerance);
void pcb_print_gpu_memory_usage(const PCBGPUAccelerationData* gpu_data);
void pcb_profile_gpu_performance(PCBGPUAccelerationData* gpu_data);

// 多GPU支持函数
int pcb_multi_gpu_distribute_work(PCBGPUAccelerationData* gpu_data_array, int num_gpus,
                                  const PCBEMModel* model);
int pcb_multi_gpu_collect_results(PCBGPUAccelerationData* gpu_data_array, int num_gpus,
                                 PCBEMSimulationResults* combined_results);

// 错误处理函数
const char* pcb_gpu_get_error_string(cudaError_t error);
const char* pcb_cublas_get_error_string(cublasStatus_t error);
const char* pcb_cusolver_get_error_string(cusolverStatus_t error);
int pcb_gpu_check_memory_errors(const PCBGPUAccelerationData* gpu_data);

#endif // PCB_GPU_ACCELERATION_H