/*********************************************************************
 * PCB电磁仿真完整流程控制器
 * 集成文件读取、网格生成、端口设置、仿真计算和结果分析
 *********************************************************************/

#ifndef PCB_SIMULATION_WORKFLOW_H
#define PCB_SIMULATION_WORKFLOW_H

#include "pcb_file_io.h"
// 注意：需要包含 pcb_electromagnetic_modeling.h 以获取完整的类型定义
// 虽然存在函数名冲突（generate_pcb_mesh），但通过不同的参数类型可以区分
// 在源文件中使用函数指针来明确调用外部函数
#include "pcb_electromagnetic_modeling.h"
#include "../core/gpu_parallelization_optimized.h"
// #include "../gui/advanced_ui_system.h"  // GUI system not available in core library

// 仿真工作流状态枚举
typedef enum {
    WORKFLOW_IDLE,
    WORKFLOW_LOADING_PCB,
    WORKFLOW_GENERATING_MESH,
    WORKFLOW_SETTING_PORTS,
    WORKFLOW_RUNNING_SIMULATION,
    WORKFLOW_PROCESSING_RESULTS,
    WORKFLOW_COMPLETED,
    WORKFLOW_ERROR,
    WORKFLOW_CANCELLED
} WorkflowState;

// 仿真工作流参数
typedef struct {
    // 输入文件
    char pcb_input_file[512];      // PCB输入文件路径
    PCBFileFormat input_format;    // 输入文件格式
    
    // 仿真设置
    double frequency_start;        // 起始频率 (Hz)
    double frequency_end;          // 结束频率 (Hz)
    int num_frequency_points;      // 频率点数
    
    // 网格设置
    double mesh_density;           // 网格密度 (网格/毫米)
    double edge_refinement_factor; // 边缘细化因子
    int enable_adaptive_mesh;      // 启用自适应网格
    int max_mesh_refinement;       // 最大网格细化次数
    
    // 端口设置
    int auto_detect_ports;         // 自动检测端口
    char port_definition_file[512]; // 端口定义文件
    double default_port_impedance; // 默认端口阻抗
    
    // 求解器设置
    double solver_tolerance;       // 求解器容差
    int max_iterations;            // 最大迭代次数
    int use_gpu_acceleration;      // 使用GPU加速
    int num_gpus;                  // GPU数量
    
    // 输出设置
    char output_directory[512];    // 输出目录
    int save_mesh_data;            // 保存网格数据
    int save_s_parameters;         // 保存S参数
    int save_current_distribution; // 保存电流分布
    int save_field_data;           // 保存场数据
    int generate_plots;            // 生成图表
    int generate_report;           // 生成报告
    
    // 性能设置
    int enable_parallel_processing; // 启用并行处理
    int num_threads;               // 线程数量
    int memory_limit_mb;           // 内存限制 (MB)
    
    // 用户界面设置
    int enable_gui;                // 启用GUI
    int enable_progress_bar;       // 启用进度条
    int enable_real_time_plotting; // 启用实时绘图
    int update_interval_ms;        // 更新间隔 (毫秒)
    int use_layered_mom;           // 使用分层介质MoM求解
    
} PCBWorkflowParams;

// 仿真工作流状态信息
typedef struct {
    WorkflowState current_state;   // 当前状态
    char state_description[256];   // 状态描述
    double progress_percentage;    // 进度百分比
    double elapsed_time;           // 已用时间 (秒)
    double estimated_time_remaining; // 预计剩余时间
    
    // 详细进度
    int current_step;              // 当前步骤
    int total_steps;               // 总步骤数
    char step_description[256];    // 步骤描述
    double step_progress;          // 步骤进度
    
    // 性能指标
    double memory_usage_mb;        // 内存使用 (MB)
    double cpu_usage_percent;      // CPU使用率
    int active_threads;            // 活动线程数
    int gpu_utilization_percent;   // GPU利用率
    
    // 错误信息
    int error_code;                // 错误代码
    char error_message[512];       // 错误消息
    
    // 统计信息
    int pcb_layers_loaded;         // 已加载的PCB层数
    int mesh_elements_generated;   // 生成的网格单元数
    int ports_defined;             // 定义的端口数
    int frequency_points_simulated; // 仿真的频率点数
    
} PCBWorkflowStatus;

// PCB仿真工作流控制器
typedef struct {
    PCBWorkflowParams params;      // 工作流参数
    PCBWorkflowStatus status;      // 工作流状态
    
    // 数据对象
    PCBDesign* pcb_design;         // PCB设计数据
    PCBEMModel* em_model;          // 电磁模型
    PCBEMSimulationResults* results; // 仿真结果
    
    // 运行时对象 (GUI components not available in core library)
    void* ui_manager;              // UI管理器 (placeholder)
    void* gpu_scheduler;           // GPU调度器 (placeholder)
    
    // 工作线程
    void* worker_thread;           // 工作线程句柄
    int worker_thread_running;     // 工作线程运行状态
    int cancel_requested;          // 取消请求
    
    // 性能监控 (GUI components not available)
    void* perf_monitor;            // 性能监控器 (placeholder)
    void* log_buffer;              // 日志缓冲区 (placeholder)
    
    // 文件路径
    char log_file[512];            // 日志文件路径
    char report_file[512];         // 报告文件路径
    
} PCBWorkflowController;

// 工作流函数声明

// 生命周期管理
PCBWorkflowController* create_pcb_workflow_controller(void);
void destroy_pcb_workflow_controller(PCBWorkflowController* controller);
int initialize_pcb_workflow(PCBWorkflowController* controller, const PCBWorkflowParams* params);

// 主要工作流步骤
int run_complete_pcb_simulation(PCBWorkflowController* controller);
int load_pcb_design(PCBWorkflowController* controller);
// 注意：此函数与 pcb_electromagnetic_modeling.h 中的 generate_pcb_mesh(PCBEMModel*) 同名但参数不同
// 在源文件中使用函数指针来明确调用外部函数，避免编译器警告
int generate_pcb_mesh_workflow(PCBWorkflowController* controller);
int setup_simulation_ports(PCBWorkflowController* controller);
int run_electromagnetic_simulation(PCBWorkflowController* controller);
int process_simulation_results(PCBWorkflowController* controller);
int generate_simulation_report(PCBWorkflowController* controller);

// 工作流控制
int start_pcb_simulation_workflow(PCBWorkflowController* controller);
int pause_pcb_simulation_workflow(PCBWorkflowController* controller);
int resume_pcb_simulation_workflow(PCBWorkflowController* controller);
int cancel_pcb_simulation_workflow(PCBWorkflowController* controller);
int get_pcb_workflow_status(PCBWorkflowController* controller, PCBWorkflowStatus* status);

// 辅助功能
int auto_detect_pcb_ports(PCBWorkflowController* controller);
int optimize_pcb_simulation_parameters(PCBWorkflowController* controller);
int validate_pcb_simulation_setup(PCBWorkflowController* controller);
int estimate_pcb_simulation_resources(PCBWorkflowController* controller, double* memory_mb, double* time_seconds);

// 结果处理
int export_pcb_simulation_data(PCBWorkflowController* controller, const char* format);
int generate_pcb_simulation_plots(PCBWorkflowController* controller);
int compare_pcb_simulation_results(PCBWorkflowController* controller1, PCBWorkflowController* controller2);

// 批处理支持
int run_batch_pcb_simulations(const char** pcb_files, int num_files, const PCBWorkflowParams* params);
int create_pcb_simulation_queue(const char** pcb_files, int num_files);
int process_pcb_simulation_queue(void);

// 高级功能
int run_pcb_parameter_sweep(PCBWorkflowController* controller, const char* parameter_name, 
                           double start_value, double end_value, int num_points);
int optimize_pcb_design_for_target_response(PCBWorkflowController* controller, 
                                           const double* target_s_parameters, int num_freq_points);

// 错误处理
const char* get_pcb_workflow_error_string(int error_code);
int get_pcb_workflow_last_error(PCBWorkflowController* controller);
void log_pcb_workflow_message(PCBWorkflowController* controller, int log_level, const char* format, ...);

// 工具函数
int convert_pcb_file_format(const char* input_file, const char* output_file, 
                           PCBFileFormat input_format, PCBFileFormat output_format);
int merge_multiple_pcb_files(const char** input_files, int num_files, const char* output_file);
int extract_pcb_layer(const char* input_file, const char* output_file, int layer_index);

#endif // PCB_SIMULATION_WORKFLOW_H
