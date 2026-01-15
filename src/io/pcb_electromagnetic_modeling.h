/*********************************************************************
 * PCB电磁建模接口
 * 将PCB几何数据转换为电磁计算模型
 *********************************************************************/

#ifndef PCB_ELECTROMAGNETIC_MODELING_H
#define PCB_ELECTROMAGNETIC_MODELING_H

#include "pcb_file_io.h"
#include "../core/gpu_parallelization_optimized.h"
// Note: Triangle is already defined in basis_functions.h, don't redefine
#include "../core/basis_functions.h"
#include "../core/core_mesh.h"

// Forward declarations for mesh types (using existing definitions)
// Triangle is already defined in basis_functions.h
// Edge and Node are type aliases for mesh types
#ifndef Edge
typedef mesh_edge_t Edge;
#endif
#ifndef Node
typedef mesh_vertex_t Node;
#endif

// PCB电磁建模参数
typedef struct {
    double frequency_start;    // 起始频率 (Hz)
    double frequency_end;      // 结束频率 (Hz)
    int num_frequency_points;  // 频率点数
    double mesh_density;       // 网格密度 (每毫米网格数)
    double edge_mesh_factor;   // 边缘网格细化因子
    int enable_adaptive_mesh;  // 启用自适应网格
    double adaptive_threshold; // 自适应阈值
    int max_mesh_refinement;   // 最大网格细化次数
    
    // 电磁仿真参数
    double solver_tolerance;   // 求解器容差
    int max_iterations;        // 最大迭代次数
    int enable_preconditioner; // 启用预条件
    int enable_fast_multipole; // 启用快速多极子
    int enable_full_mom;       // 启用完整MoM求解
    
    // GPU加速参数
    int use_gpu_acceleration;    // 使用GPU加速
    int num_gpus;              // GPU数量
    int gpu_memory_limit;      // GPU内存限制 (MB)
    
    // 输出选项
    int save_mesh_data;        // 保存网格数据
    int save_current_distribution; // 保存电流分布
    int save_field_data;       // 保存场数据
    int enable_real_time_plot; // 启用实时绘图
    
} PCBEMModelingParams;

// PCB电磁仿真结果
typedef struct {
    // S参数
    double* frequencies;       // 频率数组
    double* s_parameters;      // S参数矩阵 (complex)
    int num_ports;             // 端口数量
    int num_freq_points;       // 频率点数
    
    // 阻抗参数
    double* z_parameters;      // Z参数矩阵 (complex)
    double* y_parameters;      // Y参数矩阵 (complex)
    
    // 电流分布
    double* current_magnitude; // 电流幅度
    double* current_phase;     // 电流相位
    int num_basis_functions;   // 基函数数量
    
    // 电磁场分布
    double* e_field;           // 电场分布 (complex)
    double* h_field;           // 磁场分布 (complex)
    int num_field_points;      // 场点数
    
    // 性能指标
    double simulation_time;    // 仿真时间 (秒)
    double memory_usage;       // 内存使用 (MB)
    int convergence_status;    // 收敛状态
    int num_iterations;        // 实际迭代次数
    
    // 误差分析
    double max_error;          // 最大误差
    double rms_error;          // RMS误差
    double condition_number;   // 条件数
    
} PCBEMSimulationResults;

// PCB端口定义
typedef struct {
    char name[64];             // 端口名称
    int port_number;           // 端口号
    Point2D position;          // 端口位置
    double width;              // 端口宽度
    int layer_index;           // 所在层
    char net_name[64];         // 连接网络
    double characteristic_impedance; // 特性阻抗
    double reference_impedance;     // 参考阻抗
    int excitation_type;       // 激励类型: 0=电压源, 1=电流源
    double excitation_magnitude;    // 激励幅度
    double excitation_phase;        // 激励相位
    double pol_x;
    double pol_y;
    
} PCBPortDefinition;

// PCB-specific Triangle structure (compatible with basis_functions.h Triangle)
typedef struct {
    int v1, v2, v3;            // Vertex indices (for compatibility with existing code)
    int layer_index;           // Layer index
    int material_id;           // Material property index
    // Also support basis_functions.h format
    double x[3];               // Triangle vertices (x coordinates)
    double y[3];               // Triangle vertices (y coordinates)
    double z[3];               // Triangle vertices (z coordinates)
    int layer;                 // Layer index (alternative name)
} PCBTriangle;

// PCB-specific Node structure (compatible with mesh_vertex_t)
typedef struct {
    double x, y, z;            // Coordinates (for compatibility)
    // Also support mesh_vertex_t format
    geom_point_t position;     // Position (mesh_vertex_t format)
    int id;                    // Node ID
    bool is_boundary;          // Boundary flag
} PCBNode;

// PCB电磁模型
typedef struct {
    // 原始PCB数据
    PCBDesign* pcb_design;     // PCB设计数据
    
    // 网格数据
    PCBTriangle* triangles;     // 三角形网格 (using PCB-specific structure)
    int num_triangles;         // 三角形数量
    Edge* edges;               // 边缘信息
    int num_edges;             // 边缘数量
    PCBNode* nodes;            // 节点信息 (using PCB-specific structure)
    int num_nodes;             // 节点数量
    
    // 电磁参数
    PCBEMModelingParams params; // 建模参数
    
    // 端口定义
    PCBPortDefinition* ports;  // 端口定义数组
    int num_ports;             // 端口数量
    
    // 材料属性
    double* layer_conductivity; // 层导电率
    double* layer_permittivity; // 层介电常数
    double* layer_permeability; // 层磁导率
    double* layer_loss_tangent; // 层损耗角正切
    
    // 几何变换矩阵
    double* transformation_matrix; // 几何变换矩阵
    double* scaling_factors;       // 缩放因子
    
    // 边界条件
    int* boundary_conditions;      // 边界条件类型
    double* boundary_values;       // 边界条件值
    
} PCBEMModel;

// PCB电磁建模函数声明

// 模型创建和销毁
PCBEMModel* create_pcb_em_model(PCBDesign* pcb_design);
void destroy_pcb_em_model(PCBEMModel* model);

// 网格生成
int generate_pcb_mesh(PCBEMModel* model);
int refine_pcb_mesh(PCBEMModel* model, int target_layer);
int optimize_pcb_mesh(PCBEMModel* model);

// 端口定义
int add_pcb_port(PCBEMModel* model, const PCBPortDefinition* port);
int remove_pcb_port(PCBEMModel* model, int port_number);
int validate_pcb_ports(const PCBEMModel* model);

// 材料属性设置
int set_layer_electromagnetic_properties(PCBEMModel* model, int layer_index,
                                         double conductivity, double permittivity,
                                         double permeability, double loss_tangent);

// 电磁仿真
PCBEMSimulationResults* run_pcb_em_simulation(PCBEMModel* model);
void destroy_pcb_em_simulation_results(PCBEMSimulationResults* results);

// 结果后处理
int calculate_pcb_s_parameters(PCBEMModel* model, PCBEMSimulationResults* results);
int calculate_pcb_current_distribution(PCBEMModel* model, PCBEMSimulationResults* results);
int calculate_pcb_field_distribution(PCBEMModel* model, PCBEMSimulationResults* results);

// 性能分析
double estimate_pcb_simulation_memory(const PCBEMModel* model);
double estimate_pcb_simulation_time(const PCBEMModel* model);
int analyze_pcb_model_complexity(const PCBEMModel* model);

// 结果可视化
int generate_pcb_current_plot(const PCBEMModel* model, const PCBEMSimulationResults* results,
                              const char* output_filename, int layer_index);
int generate_pcb_field_plot(const PCBEMModel* model, const PCBEMSimulationResults* results,
                            const char* output_filename, int field_component);

// 高级分析
int calculate_pcb_characteristic_impedance(PCBEMModel* model, int port_index);
int calculate_pcb_effective_permittivity(PCBEMModel* model, int layer_index);
int calculate_pcb_loss_tangent(PCBEMModel* model, int layer_index);

// 优化建议
char** generate_pcb_design_recommendations(const PCBEMModel* model, int* num_recommendations);
void free_pcb_design_recommendations(char** recommendations, int num_recommendations);

// 导出功能
int export_pcb_em_model(const PCBEMModel* model, const char* filename);
int export_pcb_simulation_results(const PCBEMSimulationResults* results, const char* filename);

// 错误处理
const char* get_pcb_em_modeling_error_string(void);
int get_pcb_em_modeling_error_code(void);

#endif // PCB_ELECTROMAGNETIC_MODELING_H
