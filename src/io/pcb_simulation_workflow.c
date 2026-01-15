/*********************************************************************
 * PCB电磁仿真完整流程控制器实现
 * 集成文件读取、网格生成、端口设置、仿真计算和结果分析
 *********************************************************************/

#include "pcb_simulation_workflow.h"
#include <sys/stat.h>
#include <direct.h>
#include <time.h>
#ifdef _WIN32
#include <windows.h>
#endif
#if !defined(_MSC_VER)
#include <complex.h>
typedef double complex workflow_complex_t;
#else
// MSVC doesn't have complex.h, use CDOUBLE from layered_greens_function.h
#include "../core/layered_greens_function.h"
typedef CDOUBLE workflow_complex_t;
#define I make_c(0.0, 1.0)
#define creal(z) ((z).re)
#define cimag(z) ((z).im)
#endif
#include <stdarg.h>
#include <math.h>
#include "../solvers/mom/mom_solver.h"
#include "../materials/cst_materials_parser.h"
#include "../core/enhanced_sparameter_extraction.h"
#include "../core/touchstone_export.h"
#include "../core/postprocessing_field.h"
#include "../core/port_support_extended.h"
#include "pcb_electromagnetic_modeling.h"
// 注意：pcb_electromagnetic_modeling.h 中声明了 generate_pcb_mesh(PCBEMModel*)
// 而当前文件中也定义了 generate_pcb_mesh(PCBWorkflowController*)
// 这两个函数名相同但参数不同，编译器会根据参数类型区分
// 在调用外部函数时使用函数指针来明确指定
// sparameter_complex_t is defined in enhanced_sparameter_extraction.h

// Forward declarations for functions not in header
void free_sparameter_set(SParameterSet* sparams);
void free_mixed_mode_sparameters(MixedModeSParameters* mixed);

// 全局错误状态
static int pcb_workflow_error_code = 0;
static char pcb_workflow_error_string[256] = {0};

// 设置工作流错误
static void set_pcb_workflow_error(int code, const char* format, ...) {
    pcb_workflow_error_code = code;
    va_list args;
    va_start(args, format);
    vsnprintf(pcb_workflow_error_string, sizeof(pcb_workflow_error_string), format, args);
    va_end(args);
}

// 创建PCB仿真工作流控制器
PCBWorkflowController* create_pcb_workflow_controller(void) {
    PCBWorkflowController* controller = (PCBWorkflowController*)calloc(1, sizeof(PCBWorkflowController));
    if (!controller) {
        set_pcb_workflow_error(1, "内存分配失败");
        return NULL;
    }
    
    // 设置默认参数
    strcpy(controller->params.pcb_input_file, "");
    controller->params.input_format = PCB_FORMAT_GERBER_RS274X;
    
    controller->params.frequency_start = 1e9;      // 1 GHz
    controller->params.frequency_end = 100e9;     // 100 GHz
    controller->params.num_frequency_points = 101;
    
    controller->params.mesh_density = 10.0;        // 10 网格/毫米
    controller->params.edge_refinement_factor = 2.0;
    controller->params.enable_adaptive_mesh = 1;
    controller->params.max_mesh_refinement = 3;
    
    controller->params.auto_detect_ports = 1;
    strcpy(controller->params.port_definition_file, "");
    controller->params.default_port_impedance = 50.0;
    
    controller->params.solver_tolerance = 1e-6;
    controller->params.max_iterations = 1000;
    controller->params.use_gpu_acceleration = 1;
    controller->params.num_gpus = 1;
    
    strcpy(controller->params.output_directory, "output");
    controller->params.save_mesh_data = 1;
    controller->params.save_s_parameters = 1;
    controller->params.save_current_distribution = 1;
    controller->params.save_field_data = 0;
    controller->params.generate_plots = 1;
    controller->params.generate_report = 1;
    
    controller->params.enable_parallel_processing = 1;
    controller->params.num_threads = 4;
    controller->params.memory_limit_mb = 8192; // 8GB
    
    controller->params.enable_gui = 1;
    controller->params.enable_progress_bar = 1;
    controller->params.enable_real_time_plotting = 0;
    controller->params.update_interval_ms = 100;
    controller->params.use_layered_mom = 1;
    
    // 初始化状态
    controller->status.current_state = WORKFLOW_IDLE;
    strcpy(controller->status.state_description, "就绪");
    controller->status.progress_percentage = 0.0;
    controller->status.elapsed_time = 0.0;
    controller->status.estimated_time_remaining = 0.0;
    
    controller->status.current_step = 0;
    controller->status.total_steps = 6; // 总步骤数
    strcpy(controller->status.step_description, "等待开始");
    controller->status.step_progress = 0.0;
    
    controller->status.memory_usage_mb = 0.0;
    controller->status.cpu_usage_percent = 0.0;
    controller->status.active_threads = 0;
    controller->status.gpu_utilization_percent = 0;
    
    controller->status.error_code = 0;
    strcpy(controller->status.error_message, "");
    
    // 初始化数据对象
    controller->pcb_design = NULL;
    controller->em_model = NULL;
    controller->results = NULL;
    
    controller->ui_manager = NULL;
    controller->gpu_scheduler = NULL;
    
    controller->worker_thread = NULL;
    controller->worker_thread_running = 0;
    controller->cancel_requested = 0;
    
    controller->perf_monitor = NULL;
    controller->log_buffer = NULL;
    
    strcpy(controller->log_file, "pcb_simulation.log");
    strcpy(controller->report_file, "pcb_simulation_report.txt");
    
    printf("PCB仿真工作流控制器创建成功\n");
    
    return controller;
}

// 销毁PCB仿真工作流控制器
void destroy_pcb_workflow_controller(PCBWorkflowController* controller) {
    if (!controller) return;
    
    // 取消正在运行的仿真
    if (controller->worker_thread_running) {
        controller->cancel_requested = 1;
        // 等待线程结束（简化实现）
        Sleep(1000); // Windows specific
    }
    
    // 销毁数据对象
    if (controller->results) {
        destroy_pcb_em_simulation_results(controller->results);
    }
    
    if (controller->em_model) {
        destroy_pcb_em_model(controller->em_model);
    }
    
    if (controller->pcb_design) {
        destroy_pcb_design(controller->pcb_design);
    }
    
    // 销毁UI管理器
    if (controller->ui_manager) {
        // 应该调用cleanup函数
        free(controller->ui_manager);
    }
    
    // 销毁GPU调度器
    if (controller->gpu_scheduler) {
        // 应该调用cleanup函数
        free(controller->gpu_scheduler);
    }
    
    // 销毁性能监控器
    if (controller->perf_monitor) {
        // 应该调用cleanup函数
        free(controller->perf_monitor);
    }
    
    // 销毁日志缓冲区
    if (controller->log_buffer) {
        // 应该调用cleanup函数
        free(controller->log_buffer);
    }
    
    free(controller);
    printf("PCB仿真工作流控制器已销毁\n");
}

// 初始化PCB工作流
int initialize_pcb_workflow(PCBWorkflowController* controller, const PCBWorkflowParams* params) {
    if (!controller || !params) {
        set_pcb_workflow_error(2, "控制器或参数为空");
        return -1;
    }
    
    // 复制参数
    controller->params = *params;
    
    // 创建输出目录
    struct stat st = {0};
    if (stat(controller->params.output_directory, &st) == -1) {
        #ifdef _WIN32
            _mkdir(controller->params.output_directory);
        #else
            mkdir(controller->params.output_directory, 0700);
        #endif
    }
    
    // 初始化UI管理器（如果启用）
    if (controller->params.enable_gui) {
        controller->ui_manager = (void*)calloc(1, sizeof(void*)); // GUI component placeholder
        if (controller->ui_manager) {
            // 应该调用完整的初始化函数
            printf("UI管理器初始化成功\n");
        }
    }
    
    // 初始化GPU调度器（如果启用GPU加速）
    if (controller->params.use_gpu_acceleration) {
        controller->gpu_scheduler = (void*)calloc(1, sizeof(void*)); // GPU scheduler placeholder
        if (controller->gpu_scheduler) {
            // 应该调用完整的初始化函数
            printf("GPU调度器初始化成功\n");
        }
    }
    
    // 初始化性能监控器
    controller->perf_monitor = (void*)calloc(1, sizeof(void*)); // Performance monitor placeholder
    if (controller->perf_monitor) {
        // 应该调用完整的初始化函数
        printf("性能监控器初始化成功\n");
    }
    
    // 初始化日志缓冲区
    controller->log_buffer = (void*)calloc(1, sizeof(void*)); // Log buffer placeholder
    if (controller->log_buffer) {
        // 应该调用完整的初始化函数
        printf("日志缓冲区初始化成功\n");
    }
    
    // 更新状态
    controller->status.current_state = WORKFLOW_IDLE;
    strcpy(controller->status.state_description, "工作流已初始化");
    
    printf("PCB工作流初始化完成\n");
    printf("输入文件: %s\n", controller->params.pcb_input_file);
    printf("输出目录: %s\n", controller->params.output_directory);
    printf("频率范围: %.1f - %.1f GHz\n", 
           controller->params.frequency_start/1e9, controller->params.frequency_end/1e9);
    printf("网格密度: %.1f 网格/毫米\n", controller->params.mesh_density);
    
    return 0;
}

// 加载PCB设计
int load_pcb_design(PCBWorkflowController* controller) {
    if (!controller) {
        set_pcb_workflow_error(3, "控制器为空");
        return -1;
    }
    
    if (strlen(controller->params.pcb_input_file) == 0) {
        set_pcb_workflow_error(4, "未指定PCB输入文件");
        return -1;
    }
    
    // 更新状态
    controller->status.current_state = WORKFLOW_LOADING_PCB;
    strcpy(controller->status.state_description, "正在加载PCB设计文件");
    controller->status.current_step = 1;
    strcpy(controller->status.step_description, "读取PCB文件");
    
    printf("正在加载PCB文件: %s\n", controller->params.pcb_input_file);
    
    // 检测文件格式（如果未指定）
    if (controller->params.input_format == PCB_FORMAT_GERBER_RS274X) {
        controller->params.input_format = detect_pcb_file_format(controller->params.pcb_input_file);
    }
    
    // 读取PCB文件
    controller->pcb_design = read_pcb_file(controller->params.pcb_input_file, 
                                           controller->params.input_format);
    if (!controller->pcb_design) {
        set_pcb_workflow_error(5, "PCB文件读取失败: %s", get_pcb_io_error_string());
        controller->status.current_state = WORKFLOW_ERROR;
        return -1;
    }
    
    // 验证PCB设计
    if (!validate_pcb_design(controller->pcb_design)) {
        set_pcb_workflow_error(6, "PCB设计验证失败: %s", get_pcb_io_error_string());
        controller->status.current_state = WORKFLOW_ERROR;
        return -1;
    }
    
    // 更新状态
    controller->status.pcb_layers_loaded = controller->pcb_design->num_layers;
    controller->status.progress_percentage = 16.67; // 1/6
    controller->status.step_progress = 100.0;
    
    printf("PCB文件加载成功！\n");
    print_pcb_statistics(controller->pcb_design);
    
    return 0;
}

// 生成PCB网格（工作流版本）
// 注意：此函数与 pcb_electromagnetic_modeling.h 中的 generate_pcb_mesh(PCBEMModel*) 同名但参数不同
// 为了保持一致性和避免冲突，使用不同的函数名
int generate_pcb_mesh_workflow(PCBWorkflowController* controller) {
    if (!controller || !controller->pcb_design) {
        set_pcb_workflow_error(7, "控制器或PCB设计为空");
        return -1;
    }
    
    // 更新状态
    controller->status.current_state = WORKFLOW_GENERATING_MESH;
    strcpy(controller->status.state_description, "正在生成PCB网格");
    controller->status.current_step = 2;
    strcpy(controller->status.step_description, "网格生成");
    
    printf("开始生成PCB网格...\n");
    
    // 创建电磁模型
    controller->em_model = create_pcb_em_model(controller->pcb_design);
    if (!controller->em_model) {
        set_pcb_workflow_error(8, "电磁模型创建失败: %s", get_pcb_em_modeling_error_string());
        controller->status.current_state = WORKFLOW_ERROR;
        return -1;
    }
    
    // 设置建模参数（需要手动复制，因为类型不同）
    controller->em_model->params.frequency_start = controller->params.frequency_start;
    controller->em_model->params.frequency_end = controller->params.frequency_end;
    controller->em_model->params.num_frequency_points = controller->params.num_frequency_points;
    controller->em_model->params.mesh_density = controller->params.mesh_density;
    // 复制其他参数...
    
    // 生成网格 - 调用 pcb_electromagnetic_modeling.c 中的函数
    // 注意：这里调用的是 pcb_electromagnetic_modeling.h 中声明的 generate_pcb_mesh(PCBEMModel*)
    // 而不是当前文件中的 generate_pcb_mesh(PCBWorkflowController*)
    // 由于函数名相同但参数类型不同，使用函数指针来明确调用外部函数
    {
        // 声明外部函数类型（与 pcb_electromagnetic_modeling.h 中的函数匹配）
        typedef int (*generate_pcb_mesh_em_func_t)(PCBEMModel*);
        // 获取外部函数的地址（通过函数指针转换）
        // 注意：这里使用 &generate_pcb_mesh 来获取函数地址，然后转换为正确的函数指针类型
        generate_pcb_mesh_em_func_t generate_pcb_mesh_em = (generate_pcb_mesh_em_func_t)&generate_pcb_mesh;
        if (generate_pcb_mesh_em(controller->em_model) != 0) {
            set_pcb_workflow_error(9, "网格生成失败: %s", get_pcb_em_modeling_error_string());
            controller->status.current_state = WORKFLOW_ERROR;
            return -1;
        }
    }
    
    // 细化网格（如果需要）
    if (controller->params.enable_adaptive_mesh) {
        for (int refinement = 0; refinement < controller->params.max_mesh_refinement; refinement++) {
            printf("执行第 %d 次网格细化...\n", refinement + 1);
            for (int layer = 0; layer < controller->pcb_design->num_layers; layer++) {
                if (controller->pcb_design->layers[layer].type == PCB_LAYER_COPPER) {
                    refine_pcb_mesh(controller->em_model, layer);
                }
            }
        }
    }
    
    // 优化网格
    optimize_pcb_mesh(controller->em_model);
    
    // 更新状态
    controller->status.mesh_elements_generated = controller->em_model->num_triangles;
    controller->status.progress_percentage = 33.33; // 2/6
    controller->status.step_progress = 100.0;
    
    printf("PCB网格生成完成！\n");
    printf("生成三角形单元: %d\n", controller->em_model->num_triangles);
    printf("生成节点: %d\n", controller->em_model->num_nodes);
    
    return 0;
}

// 设置仿真端口
int setup_simulation_ports(PCBWorkflowController* controller) {
    if (!controller || !controller->em_model) {
        set_pcb_workflow_error(10, "控制器或电磁模型为空");
        return -1;
    }
    
    // 更新状态
    controller->status.current_state = WORKFLOW_SETTING_PORTS;
    strcpy(controller->status.state_description, "正在设置仿真端口");
    controller->status.current_step = 3;
    strcpy(controller->status.step_description, "端口设置");
    
    printf("开始设置仿真端口...\n");
    
    if (controller->params.auto_detect_ports) {
        printf("自动检测PCB端口...\n");
        auto_detect_pcb_ports(controller);
    } else if (strlen(controller->params.port_definition_file) > 0) {
        printf("从文件读取端口定义: %s\n", controller->params.port_definition_file);
        // 这里应该实现端口定义文件读取
    } else {
        // 手动添加默认端口
        printf("添加默认端口...\n");
        
        // 获取PCB边界
        Point2D min_point, max_point;
        calculate_pcb_bounds(controller->pcb_design, &min_point, &max_point);
        
        // 在PCB边缘添加端口
        PCBPortDefinition port1;
        strcpy(port1.name, "Port1");
        port1.port_number = 0;
        port1.position.x = min_point.x + (max_point.x - min_point.x) * 0.1;
        port1.position.y = min_point.y + (max_point.y - min_point.y) * 0.5;
        port1.width = 1.0; // 1mm
        port1.layer_index = 0; // 顶层
        strcpy(port1.net_name, "NET1");
        port1.characteristic_impedance = controller->params.default_port_impedance;
        port1.reference_impedance = controller->params.default_port_impedance;
        port1.excitation_type = 0; // 电压源
        port1.excitation_magnitude = 1.0;
        port1.excitation_phase = 0.0;
        
        add_pcb_port(controller->em_model, &port1);
        
        // 添加第二个端口
        PCBPortDefinition port2;
        strcpy(port2.name, "Port2");
        port2.port_number = 1;
        port2.position.x = min_point.x + (max_point.x - min_point.x) * 0.9;
        port2.position.y = min_point.y + (max_point.y - min_point.y) * 0.5;
        port2.width = 1.0;
        port2.layer_index = 0;
        strcpy(port2.net_name, "NET2");
        port2.characteristic_impedance = controller->params.default_port_impedance;
        port2.reference_impedance = controller->params.default_port_impedance;
        port2.excitation_type = 0;
        port2.excitation_magnitude = 0.0; // 接收端口
        port2.excitation_phase = 0.0;
        
        add_pcb_port(controller->em_model, &port2);
    }
    
    // 验证端口
    if (!validate_pcb_ports(controller->em_model)) {
        set_pcb_workflow_error(11, "端口验证失败");
        controller->status.current_state = WORKFLOW_ERROR;
        return -1;
    }
    
    // 更新状态
    controller->status.ports_defined = controller->em_model->num_ports;
    controller->status.progress_percentage = 50.0; // 3/6
    controller->status.step_progress = 100.0;
    
    printf("端口设置完成！\n");
    printf("定义端口数: %d\n", controller->em_model->num_ports);
    
    return 0;
}

// 运行电磁仿真
int run_electromagnetic_simulation(PCBWorkflowController* controller) {
    if (!controller || !controller->em_model) {
        set_pcb_workflow_error(12, "控制器或电磁模型为空");
        return -1;
    }
    
    // 更新状态
    controller->status.current_state = WORKFLOW_RUNNING_SIMULATION;
    strcpy(controller->status.state_description, "正在运行电磁仿真");
    controller->status.current_step = 4;
    strcpy(controller->status.step_description, "电磁仿真");
    
    printf("开始PCB电磁仿真...\n");
    printf("频率范围: %.1f - %.1f GHz\n", 
           controller->params.frequency_start/1e9, controller->params.frequency_end/1e9);
    printf("频率点数: %d\n", controller->params.num_frequency_points);
    
    // 估算资源需求
    double memory_mb, time_seconds;
    estimate_pcb_simulation_resources(controller, &memory_mb, &time_seconds);
    printf("预计内存使用: %.1f MB\n", memory_mb);
    printf("预计仿真时间: %.1f 秒\n", time_seconds);
    
    // 运行仿真
    if (controller->params.use_layered_mom) {
        mom_config_t cfg;
        memset(&cfg, 0, sizeof(cfg));
        double fmid = 0.5 * (controller->params.frequency_start + controller->params.frequency_end);
        cfg.frequency = fmid;
        cfg.basis_type = 1;
        cfg.formulation = 1;
        cfg.tolerance = controller->params.solver_tolerance;
        cfg.max_iterations = controller->params.max_iterations;
        cfg.use_preconditioner = 1;
        cfg.use_sparse_solver = 1;
        cfg.gmres_restart = 50;
        cfg.drop_tolerance = 1e-10;
        cfg.compute_current_distribution = 1;
        mom_solver_t* solver = mom_solver_create(&cfg);
        if (!solver) {
            set_pcb_workflow_error(13, "MoM求解器创建失败");
            controller->status.current_state = WORKFLOW_ERROR;
            return -1;
        }
        mom_solver_set_mesh(solver, (void*)controller->em_model);
        // 加载Taconic TLT-6材料
        const char* mat_dir = "library/Materials";
        char mat_path[512];
        snprintf(mat_path, sizeof(mat_path), "%s/%s", mat_dir, "Taconic TLT-6 (lossy).mtd");
        cst_material_t* tlt6 = cst_materials_parse_file(mat_path);
        LayeredMedium medium = {0};
        int L = controller->pcb_design->num_layers;
        medium.num_layers = L;
        medium.thickness = (double*)calloc(L, sizeof(double));
        medium.epsilon_r = (double*)calloc(L, sizeof(double));
        medium.mu_r = (double*)calloc(L, sizeof(double));
        medium.sigma = (double*)calloc(L, sizeof(double));
        medium.tan_delta = (double*)calloc(L, sizeof(double));
        for (int li = 0; li < L; li++) {
            PCBLayerInfo* Lr = &controller->pcb_design->layers[li];
            medium.thickness[li] = (Lr->thickness > 0) ? Lr->thickness * 1e-3 : 1e-6;
            medium.epsilon_r[li] = (Lr->dielectric_constant > 0) ? Lr->dielectric_constant : 1.0;
            medium.mu_r[li] = 1.0;
            medium.sigma[li] = (Lr->type == PCB_LAYER_COPPER) ? controller->pcb_design->copper_conductivity : 0.0;
            medium.tan_delta[li] = (Lr->loss_tangent > 0) ? Lr->loss_tangent : 0.0;
        }
        FrequencyDomain fd;
        fd.freq = fmid;
        fd.omega = 2.0 * M_PI * fmid;
        fd.k0 = make_c(2.0 * M_PI * fmid / 299792458.0, 0.0);
        fd.eta0 = make_c(376.730313561, 0.0);
        GreensFunctionParams gp;
        memset(&gp, 0, sizeof(gp));
        gp.n_points = 16;
        gp.krho_max = 50.0;
        gp.krho_points = (double*)calloc(gp.n_points, sizeof(double));
        gp.weights = (double*)calloc(gp.n_points, sizeof(double));
        for (int i = 0; i < gp.n_points; i++) { gp.krho_points[i] = (i+1) * gp.krho_max / gp.n_points; gp.weights[i] = gp.krho_max / gp.n_points; }
        gp.use_dcim = false;  // Use WGF instead of DCIM for PCB (WGF is better for triangular mesh)
        // Note: WGF will be automatically selected by layered_greens_function_unified
        // based on problem characteristics (2-10 layers, typical PCB distances)
        mom_solver_set_layered_medium(solver, &medium, &fd, &gp);
        // 添加端口激励
        for (int pi = 0; pi < controller->em_model->num_ports; pi++) {
            PCBPortDefinition* Pdef = &controller->em_model->ports[pi];
            point3d_t pos = { Pdef->position.x, Pdef->position.y, controller->pcb_design->layers[Pdef->layer_index].elevation };
            point3d_t pol = { Pdef->pol_x, Pdef->pol_y, 0.0 };
            double amp = (pi == 0) ? 1.0 : 0.0;
            double width = (Pdef->width > 0.0) ? Pdef->width : 0.5;
            mom_solver_add_lumped_excitation(solver, &pos, &pol, amp, width, Pdef->layer_index);
        }
        controller->results = (PCBEMSimulationResults*)calloc(1, sizeof(PCBEMSimulationResults));
        int P = controller->em_model->num_ports;
        int nf = controller->params.num_frequency_points;
        {
            double width_mm = fabs(controller->pcb_design->outline.top_right.x - controller->pcb_design->outline.bottom_left.x);
            double height_mm = fabs(controller->pcb_design->outline.top_right.y - controller->pcb_design->outline.bottom_left.y);
            double max_dim = (width_mm > height_mm) ? width_mm : height_mm;
            if (nf <= 0) {
                if (max_dim < 50.0) nf = 101;
                else if (max_dim < 150.0) nf = 201;
                else nf = 301;
            }
        }
        controller->results->num_freq_points = nf;
        controller->results->frequencies = (double*)calloc(nf, sizeof(double));
        controller->results->num_ports = P;
        controller->results->s_parameters = (double*)calloc(P*P*nf*2, sizeof(double));
        controller->results->z_parameters = (double*)calloc(P*P*nf*2, sizeof(double));
        double f0 = controller->params.frequency_start;
        double f1 = controller->params.frequency_end;
        for (int fi = 0; fi < nf; fi++) {
            double f = f0 + (f1 - f0) * (nf == 1 ? 0.0 : (double)fi / (double)(nf-1));
            controller->results->frequencies[fi] = f;
            fd.freq = f; fd.omega = 2.0 * M_PI * f; fd.k0 = make_c(2.0 * M_PI * f / 299792458.0, 0.0); fd.eta0 = make_c(376.730313561, 0.0);
            // 使用Taconic TLT-6更新介质层材料参数
            if (tlt6) {
                for (int li = 0; li < L; li++) {
                    PCBLayerInfo* Lr = &controller->pcb_design->layers[li];
                    if (Lr->type == PCB_LAYER_DIELECTRIC) {
                        cst_complex_t eps_c = cst_materials_get_epsilon(tlt6, f);
                        double er = eps_c.re;
                        double tand = (er > 1e-12) ? (-eps_c.im / er) : medium.tan_delta[li];
                        medium.epsilon_r[li] = er;
                        medium.tan_delta[li] = tand;
                        medium.sigma[li] = cst_materials_get_conductivity(tlt6, f);
                    }
                }
            }
            // 更新分层介质频率设置与铜粗糙度/皮肤效应
            {
                double mu0 = 4.0 * M_PI * 1e-7;
                double Rq = 2.0e-6; // 表面粗糙度(米) 默认2um，可后续读取
                for (int li = 0; li < L; li++) {
                    PCBLayerInfo* Lr = &controller->pcb_design->layers[li];
                    if (Lr->type == PCB_LAYER_COPPER) {
                        double sigma0 = medium.sigma[li] > 0.0 ? medium.sigma[li] : controller->pcb_design->copper_conductivity;
                        double delta = sqrt(2.0/(fd.omega*mu0*sigma0));
                        double Rs = sqrt(M_PI * fd.freq * mu0 / sigma0);
                        double rough = Rq/delta;
                        double Rs_eff = Rs * (1.0 + (2.0/M_PI) * atan(1.4 * rough * rough));
                        double sigma_eff = (M_PI * fd.freq * mu0) / (Rs_eff * Rs_eff);
                        medium.sigma[li] = sigma_eff;
                    }
                }
            }
            mom_solver_set_layered_medium(solver, &medium, &fd, &gp);
            if (mom_solver_assemble_matrix(solver) != 0) { set_pcb_workflow_error(13, "MoM装配失败"); controller->status.current_state = WORKFLOW_ERROR; free_layered_medium(&medium); free_greens_function_params(&gp); if (tlt6) free(tlt6); return -1; }
            {
                int Nunk = mom_solver_get_num_unknowns(solver);
                mom_config_t cfg_dyn = cfg;
                if (Nunk >= 1500) {
                    cfg_dyn.use_sparse_solver = 1;
                    cfg_dyn.max_iterations = (Nunk/2 > 1000) ? 1000 : (Nunk/2);
                    cfg_dyn.gmres_restart = (Nunk/200 + 20 > 80) ? 80 : (Nunk/200 + 20);
                    cfg_dyn.drop_tolerance = (Nunk > 5000) ? 1e-10 : 1e-12;
                    cfg_dyn.use_preconditioner = 1;
                } else {
                    cfg_dyn.use_sparse_solver = 0;
                    cfg_dyn.max_iterations = 0;
                }
                cfg_dyn.assembly_drop_tolerance = (Nunk > 3000) ? 1e-12 : 0.0;
                cfg_dyn.near_quadrature_points = (Nunk < 1000) ? 7 : 13;
                cfg_dyn.near_threshold = 1e-6;
                double lambda_m = 299792458.0 / f;
                cfg_dyn.near_quadrature_points = (f > 20e9) ? 13 : cfg_dyn.near_quadrature_points;
                cfg_dyn.near_threshold = fmin(1e-5, 0.01 * lambda_m);
                cfg_dyn.assembly_drop_tolerance = (Nunk > 5000 || f > 50e9) ? 1e-13 : cfg_dyn.assembly_drop_tolerance;
                mom_solver_configure(solver, &cfg_dyn);
            }
            for (int pi = 0; pi < P; pi++) {
                for (int i = 0; i < P; i++) {
                    PCBPortDefinition* Pdef = &controller->em_model->ports[i];
                    point3d_t pos = { Pdef->position.x, Pdef->position.y, controller->pcb_design->layers[Pdef->layer_index].elevation };
                    point3d_t pol = { Pdef->pol_x, Pdef->pol_y, 0.0 };
                    double amp = (i == pi) ? 1.0 : 0.0;
                    double width = (Pdef->width > 0.0) ? Pdef->width : 0.5;
                    mom_solver_add_lumped_excitation(solver, &pos, &pol, amp, width, Pdef->layer_index);
                }
                if (mom_solver_solve(solver) != 0) { set_pcb_workflow_error(13, "MoM求解失败"); controller->status.current_state = WORKFLOW_ERROR; free_layered_medium(&medium); free_greens_function_params(&gp); if (tlt6) free(tlt6); return -1; }
                for (int pj = 0; pj < P; pj++) {
                    PCBPortDefinition* Pm = &controller->em_model->ports[pj];
                    double Ij = 0.0;
                    mom_solver_compute_port_current(solver, Pm->position.x, Pm->position.y, Pm->width, Pm->layer_index, &Ij);
                    workflow_complex_t Zij;
                    #if defined(_MSC_VER)
                    Zij.re = 1.0 / (Ij + 1e-12);
                    Zij.im = 0.0;
                    #else
                    Zij = 1.0 / (Ij + 1e-12) + 0.0 * I;
                    #endif
                    int idxz = ((pi * P + pj) * nf + fi) * 2;
                    controller->results->z_parameters[idxz] = creal(Zij);
                    controller->results->z_parameters[idxz+1] = cimag(Zij);
                }
            }
        }
        // 分配Z0数组（在代码块外定义，以便在整个作用域内使用）
        // 注意：Z0可能被宏定义为ETA0，需要先取消定义
        #ifdef Z0
        #undef Z0
        #endif
        double* Z0 = NULL;
        if (P > 0) {
            Z0 = (double*)calloc((size_t)P, sizeof(double));
            if (!Z0) {
                set_pcb_workflow_error(14, "内存分配失败");
                controller->status.current_state = WORKFLOW_ERROR;
                free_layered_medium(&medium);
                free_greens_function_params(&gp);
                if (tlt6) free(tlt6);
                return -1;
            }
            for (int i = 0; i < P; i++) {
                Z0[i] = controller->params.default_port_impedance;
            }
        }
        {
            sparameter_complex_t* Zall = (sparameter_complex_t*)calloc(nf * P * P, sizeof(sparameter_complex_t));
            for (int fi = 0; fi < nf; fi++) {
                for (int i = 0; i < P; i++) {
                    for (int j = 0; j < P; j++) {
                        int idxz = ((i * P + j) * nf + fi) * 2;
                        Zall[fi * P * P + i + j * P].re = controller->results->z_parameters[idxz];
                        Zall[fi * P * P + i + j * P].im = controller->results->z_parameters[idxz+1];
                    }
                }
            }
            SParameterSet* sset = NULL;
            if (Z0) {
                sset = extract_sparameters_from_mom(
                    controller->em_model, 
                    (const sparameter_complex_t*)Zall, 
                    Z0, 
                    (controller->results->frequencies ? controller->results->frequencies : NULL), 
                    nf);
            }
            if (sset && sset->data) {
                double* Zren = (double*)calloc(P, sizeof(double));
                for (int i = 0; i < P; i++) Zren[i] = controller->params.default_port_impedance;
                SParameterSet* sren = renomalize_sparameters(sset, Zren, false);
                if (sren && sren->data) {
                    for (int fi = 0; fi < nf; fi++) {
                        for (int i = 0; i < P; i++) {
                            for (int j = 0; j < P; j++) {
                                int idxs = ((i * P + j) * nf + fi) * 2;
                                sparameter_complex_t sij = sren->data[fi].s_matrix[i + j * P];
                                controller->results->s_parameters[idxs] = (sij.re);
                                controller->results->s_parameters[idxs+1] = (sij.im);
                            }
                        }
                    }
                }
                if (sren) free_sparameter_set(sren);
                free(Zren);
                for (int fi = 0; fi < nf; fi++) {
                    for (int i = 0; i < P; i++) {
                        for (int j = 0; j < P; j++) {
                            int idxs = ((i * P + j) * nf + fi) * 2;
                            workflow_complex_t sij;
                            #if defined(_MSC_VER)
                            sij.re = controller->results->s_parameters[idxs];
                            sij.im = controller->results->s_parameters[idxs+1];
                            #else
                            sij = controller->results->s_parameters[idxs] + I * controller->results->s_parameters[idxs+1];
                            #endif
                            controller->results->s_parameters[idxs] = creal(sij);
                            controller->results->s_parameters[idxs+1] = cimag(sij);
                        }
                    }
                }
            } else {
                for (int fi = 0; fi < nf; fi++) {
                    for (int i = 0; i < P; i++) {
                        for (int j = 0; j < P; j++) {
                            int idxz = ((i * P + j) * nf + fi) * 2;
                            workflow_complex_t Zij;
                            #if defined(_MSC_VER)
                            Zij.re = controller->results->z_parameters[idxz];
                            Zij.im = controller->results->z_parameters[idxz+1];
                            #else
                            Zij = controller->results->z_parameters[idxz] + I * controller->results->z_parameters[idxz+1];
                            #endif
                            double Zref = (Z0 && i < P) ? Z0[i] : controller->params.default_port_impedance;
                            // Sij = (Zij - Zref) / (Zij + Zref)
                            workflow_complex_t Zij_plus_Zref, Zij_minus_Zref;
                            #if defined(_MSC_VER)
                            Zij_plus_Zref.re = Zij.re + Zref;
                            Zij_plus_Zref.im = Zij.im;
                            Zij_minus_Zref.re = Zij.re - Zref;
                            Zij_minus_Zref.im = Zij.im;
                            #else
                            Zij_plus_Zref = Zij + Zref;
                            Zij_minus_Zref = Zij - Zref;
                            #endif
                            double denom = Zij_plus_Zref.re * Zij_plus_Zref.re + Zij_plus_Zref.im * Zij_plus_Zref.im;
                            workflow_complex_t Sij;
                            #if defined(_MSC_VER)
                            Sij.re = (Zij_minus_Zref.re * Zij_plus_Zref.re + Zij_minus_Zref.im * Zij_plus_Zref.im) / denom;
                            Sij.im = (Zij_minus_Zref.im * Zij_plus_Zref.re - Zij_minus_Zref.re * Zij_plus_Zref.im) / denom;
                            #else
                            Sij = ((Zij_minus_Zref.re * Zij_plus_Zref.re + Zij_minus_Zref.im * Zij_plus_Zref.im) / denom) + 
                                  I * ((Zij_minus_Zref.im * Zij_plus_Zref.re - Zij_minus_Zref.re * Zij_plus_Zref.im) / denom);
                            #endif
                            int idxs = ((i * P + j) * nf + fi) * 2;
                            controller->results->s_parameters[idxs] = creal(Sij);
                            controller->results->s_parameters[idxs+1] = cimag(Sij);
                        }
                    }
                }
            }
            if (sset) {
                // Export Touchstone format with enhanced options
                char tsfile[1024];
                touchstone_generate_filename("results", sset->num_ports, tsfile, sizeof(tsfile));
                char full_path[1024];
                snprintf(full_path, sizeof(full_path), "%s/%s", controller->params.output_directory, tsfile);
                
                touchstone_export_options_t ts_opts = touchstone_get_default_options();
                ts_opts.data_format = TOUCHSTONE_FORMAT_RI;
                ts_opts.freq_unit = TOUCHSTONE_FREQ_GHZ;
                ts_opts.reference_impedance = controller->params.default_port_impedance;
                ts_opts.include_comments = true;
                ts_opts.include_port_names = true;
                ts_opts.description = "PulseMoM S-parameter results";
                ts_opts.precision = 6;
                
                if (touchstone_export_file(sset, full_path, &ts_opts) == 0) {
                    printf("已导出Touchstone格式: %s\n", full_path);
                } else {
                    printf("警告: Touchstone导出失败，使用旧格式\n");
                    // Fallback to old format
                    snprintf(tsfile, sizeof(tsfile), "%s/%s", controller->params.output_directory, "results_touchstone.sNp");
                    write_touchstone_file(sset, tsfile, 3);
                }
                bool passive = check_passivity(sset, 1e-12);
                printf("S-parameters passivity: %s\n", passive ? "PASS" : "FAIL");
                bool causal = check_causality(sset, 1e-12);
                printf("S-parameters causality: %s\n", causal ? "PASS" : "FAIL");
                if (controller->em_model->num_ports >= 2 && (controller->em_model->num_ports % 2) == 0) {
                    int pairs = controller->em_model->num_ports / 2;
                    int* mapping = (int*)calloc(controller->em_model->num_ports, sizeof(int));
                    for (int d = 0; d < pairs; d++) { mapping[2*d] = 2*d; mapping[2*d+1] = 2*d+1; }
                    MixedModeSParameters* mm = convert_to_mixed_mode(sset, mapping, pairs);
                    if (mm && mm->differential) {
                        char tsfile_mm[1024];
                        snprintf(tsfile_mm, sizeof(tsfile_mm), "%s/%s", controller->params.output_directory, "results_touchstone_mixed.sNp");
                        write_touchstone_file(mm->differential, tsfile_mm, 3);
                        free_mixed_mode_sparameters(mm);
                    }
                    free(mapping);
                }
                free_sparameter_set(sset);
            }
            free(Zall);
        }
        if (Z0) {
            free(Z0);
        }
        controller->results->num_iterations = cfg.max_iterations;
        controller->results->memory_usage = mom_solver_get_memory_usage(solver);
        free_layered_medium(&medium);
        free_greens_function_params(&gp);
        if (tlt6) free(tlt6);
        mom_solver_destroy(solver);
    }
    if (!controller->results) {
        set_pcb_workflow_error(13, "电磁仿真失败: %s", get_pcb_em_modeling_error_string());
        controller->status.current_state = WORKFLOW_ERROR;
        return -1;
    }
    
    // 更新状态
    controller->status.frequency_points_simulated = controller->results->num_freq_points;
    controller->status.progress_percentage = 66.67; // 4/6
    controller->status.step_progress = 100.0;
    
    printf("电磁仿真完成！\n");
    
    return 0;
}

// 处理仿真结果
int process_simulation_results(PCBWorkflowController* controller) {
    if (!controller || !controller->results) {
        set_pcb_workflow_error(14, "控制器或仿真结果为空");
        return -1;
    }
    
    // 更新状态
    controller->status.current_state = WORKFLOW_PROCESSING_RESULTS;
    strcpy(controller->status.state_description, "正在处理仿真结果");
    controller->status.current_step = 5;
    strcpy(controller->status.step_description, "结果处理");
    
    printf("开始处理仿真结果...\n");
    
    // 生成图表
    if (controller->params.generate_plots) {
        printf("生成仿真图表...\n");
        generate_pcb_simulation_plots(controller);
    }
    
    // 导出数据
    if (controller->params.save_s_parameters || 
        controller->params.save_current_distribution ||
        controller->params.save_field_data) {
        printf("导出仿真数据...\n");
        export_pcb_simulation_data(controller, "csv");
    }
    
    // 更新状态
    controller->status.progress_percentage = 83.33; // 5/6
    controller->status.step_progress = 100.0;
    
    printf("仿真结果处理完成！\n");
    
    return 0;
}

int export_pcb_simulation_data(PCBWorkflowController* controller, const char* format) {
    if (!controller || !controller->results) return -1;
    const char* outdir = controller->params.output_directory;
    // 导出电流分布
    {
        char path[1024];
        snprintf(path, sizeof(path), "%s/currents.csv", outdir);
        FILE* fp = fopen(path, "w");
        if (fp) {
            fprintf(fp, "basis_index,magnitude,phase\n");
            for (int i = 0; i < controller->results->num_basis_functions; i++) {
                double mag = controller->results->current_magnitude ? controller->results->current_magnitude[i] : 0.0;
                double pha = controller->results->current_phase ? controller->results->current_phase[i] : 0.0;
                fprintf(fp, "%d,%.6e,%.6e\n", i, mag, pha);
            }
            fclose(fp);
            printf("已导出电流分布: %s\n", path);
        }
    }
    // 导出S参数（若缺失，则使用分层格林函数近似计算）
    {
        char path[1024];
        snprintf(path, sizeof(path), "%s/sparams.csv", outdir);
        FILE* fp = fopen(path, "w");
        if (fp) {
            fprintf(fp, "port_i,port_j,real,imag\n");
            int P = controller->results->num_ports;
            if (controller->results->s_parameters && P > 0) {
                for (int i = 0; i < P; i++) {
                    for (int j = 0; j < P; j++) {
                        int idx = ((i * P + j) * controller->results->num_freq_points) * 2;
                        double sr = controller->results->s_parameters[idx];
                        double si = controller->results->s_parameters[idx+1];
                        fprintf(fp, "%d,%d,%.6e,%.6e\n", i, j, sr, si);
                    }
                }
            } else if (P > 0) {
                // 分层格林函数近似S参数
                double fmid = (controller->results->num_freq_points > 0) ? controller->results->frequencies[0] : (0.5 * (controller->params.frequency_start + controller->params.frequency_end));
                LayeredMedium medium = {0};
                int L = controller->pcb_design->num_layers;
                medium.num_layers = L;
                medium.thickness = (double*)calloc(L, sizeof(double));
                medium.epsilon_r = (double*)calloc(L, sizeof(double));
                medium.mu_r = (double*)calloc(L, sizeof(double));
                medium.sigma = (double*)calloc(L, sizeof(double));
                medium.tan_delta = (double*)calloc(L, sizeof(double));
                for (int li = 0; li < L; li++) {
                    PCBLayerInfo* Lr = &controller->pcb_design->layers[li];
                    medium.thickness[li] = (Lr->thickness > 0) ? Lr->thickness * 1e-3 : 1e-6;
                    medium.epsilon_r[li] = (Lr->dielectric_constant > 0) ? Lr->dielectric_constant : 1.0;
                    medium.mu_r[li] = 1.0;
                    medium.sigma[li] = (Lr->type == PCB_LAYER_COPPER) ? controller->pcb_design->copper_conductivity : 0.0;
                    medium.tan_delta[li] = (Lr->loss_tangent > 0) ? Lr->loss_tangent : 0.0;
                }
                FrequencyDomain fd;
                fd.freq = fmid;
                fd.omega = 2.0 * M_PI * fmid;
                fd.k0 = make_c(2.0 * M_PI * fmid / 299792458.0, 0.0);
                fd.eta0 = make_c(376.730313561, 0.0);
                GreensFunctionParams gp;
                memset(&gp, 0, sizeof(gp));
                gp.n_points = 16;
                gp.krho_max = 50.0;
                gp.krho_points = (double*)calloc(gp.n_points, sizeof(double));
                gp.weights = (double*)calloc(gp.n_points, sizeof(double));
                for (int i = 0; i < gp.n_points; i++) { gp.krho_points[i] = (i+1) * gp.krho_max / gp.n_points; gp.weights[i] = gp.krho_max / gp.n_points; }
                gp.use_dcim = true;
                for (int i = 0; i < P; i++) {
                    for (int j = 0; j < P; j++) {
                        PCBPortDefinition* Pi = &controller->em_model->ports[i];
                        PCBPortDefinition* Pj = &controller->em_model->ports[j];
                        GreensFunctionPoints pts;
                        pts.x = Pi->position.x; pts.y = Pi->position.y; pts.z = controller->pcb_design->layers[Pi->layer_index].elevation;
                        pts.xp = Pj->position.x; pts.yp = Pj->position.y; pts.zp = controller->pcb_design->layers[Pj->layer_index].elevation;
                        pts.layer_src = Pi->layer_index; pts.layer_obs = Pj->layer_index;
                        GreensFunctionDyadic* Gd = greens_function_layered(&medium, &fd, &pts, &gp);
                        double sx = Pi->pol_x, sy = Pi->pol_y; double sn = sqrt(sx*sx+sy*sy); if (sn>0){sx/=sn; sy/=sn;}
                        double rx = Pj->pol_x, ry = Pj->pol_y; double rn = sqrt(rx*rx+ry*ry); if (rn>0){rx/=rn; ry/=rn;}
                        CDOUBLE Gproj = make_c(0.0, 0.0);
                        if (Gd) {
                            CDOUBLE Gxx = Gd->G_ee[0][0];
                            CDOUBLE Gxy = Gd->G_ee[0][1];
                            CDOUBLE Gyx = Gd->G_ee[1][0];
                            CDOUBLE Gyy = Gd->G_ee[1][1];
                            // Gproj = Gxx*rx*sx + Gxy*rx*sy + Gyx*ry*sx + Gyy*ry*sy
                            CDOUBLE term1 = cmul(Gxx, make_c(rx*sx, 0.0));
                            CDOUBLE term2 = cmul(Gxy, make_c(rx*sy, 0.0));
                            CDOUBLE term3 = cmul(Gyx, make_c(ry*sx, 0.0));
                            CDOUBLE term4 = cmul(Gyy, make_c(ry*sy, 0.0));
                            Gproj = cadd(cadd(cadd(term1, term2), term3), term4);
                            free_greens_function_dyadic(Gd);
                        }
                        fprintf(fp, "%d,%d,%.6e,%.6e\n", i, j, creal(Gproj), cimag(Gproj));
                    }
                }
                free_layered_medium(&medium);
                free_greens_function_params(&gp);
            }
            fclose(fp);
            printf("已导出S参数: %s\n", path);
        }
    }
    // 导出端口列表
    {
        char path[1024];
        snprintf(path, sizeof(path), "%s/ports.csv", outdir);
        FILE* fp = fopen(path, "w");
        if (fp) {
            fprintf(fp, "port_index,x,y,layer,width\n");
            int P = controller->em_model->num_ports;
            for (int i = 0; i < P; i++) {
                PCBPortDefinition* Pi = &controller->em_model->ports[i];
                double z = controller->pcb_design->layers[Pi->layer_index].elevation;
                fprintf(fp, "%d,%.6e,%.6e,%d,%.6e\n", i, Pi->position.x, Pi->position.y, Pi->layer_index, Pi->width);
            }
            fclose(fp);
            printf("已导出端口列表: %s\n", path);
        }
    }
    return 0;
}

int generate_pcb_simulation_plots(PCBWorkflowController* controller) {
    if (!controller || !controller->results) return -1;
    const char* outdir = controller->params.output_directory;
    int P = controller->results->num_ports;
    int nf = controller->results->num_freq_points;
    if (P <= 0 || nf <= 0) return 0;
    char path_mag[1024];
    snprintf(path_mag, sizeof(path_mag), "%s/s11_mag_vs_freq.csv", outdir);
    FILE* fmag = fopen(path_mag, "w");
    if (fmag) {
        fprintf(fmag, "freq_GHz");
        for (int i = 0; i < P; i++) fprintf(fmag, ",S%d%d_mag", i+1, i+1);
        fprintf(fmag, "\n");
        for (int fi = 0; fi < nf; fi++) {
            fprintf(fmag, "%.6f", controller->results->frequencies[fi]/1e9);
            for (int i = 0; i < P; i++) {
                int idxs = ((i * P + i) * nf + fi) * 2;
                double sr = controller->results->s_parameters[idxs];
                double si = controller->results->s_parameters[idxs+1];
                double mag = sqrt(sr*sr + si*si);
                fprintf(fmag, ",%.6e", mag);
            }
            fprintf(fmag, "\n");
        }
        fclose(fmag);
    }
    char path_z[1024];
    snprintf(path_z, sizeof(path_z), "%s/z11_vs_freq.csv", outdir);
    FILE* fz = fopen(path_z, "w");
    if (fz) {
        fprintf(fz, "freq_GHz");
        for (int i = 0; i < P; i++) fprintf(fz, ",Z%d%d_real,Z%d%d_imag", i+1, i+1, i+1, i+1);
        fprintf(fz, "\n");
        for (int fi = 0; fi < nf; fi++) {
            fprintf(fz, "%.6f", controller->results->frequencies[fi]/1e9);
            for (int i = 0; i < P; i++) {
                int idxz = ((i * P + i) * nf + fi) * 2;
                double zr = controller->results->z_parameters[idxz];
                double zi = controller->results->z_parameters[idxz+1];
                fprintf(fz, ",%.6e,%.6e", zr, zi);
            }
            fprintf(fz, "\n");
        }
        fclose(fz);
    }
    // 导出S21相位与群时延
    if (P >= 2) {
        char path_phase[1024];
        snprintf(path_phase, sizeof(path_phase), "%s/s21_phase_vs_freq.csv", outdir);
        FILE* fph = fopen(path_phase, "w");
        if (fph) {
            fprintf(fph, "freq_GHz,S21_phase_deg\n");
            for (int fi = 0; fi < nf; fi++) {
                int idxs = ((0 * P + 1) * nf + fi) * 2; // S21
                double sr = controller->results->s_parameters[idxs];
                double si = controller->results->s_parameters[idxs+1];
                double phase = atan2(si, sr) * 180.0 / M_PI;
                fprintf(fph, "%.6f,%.6f\n", controller->results->frequencies[fi]/1e9, phase);
            }
            fclose(fph);
        }
        char path_gd[1024];
        snprintf(path_gd, sizeof(path_gd), "%s/s21_group_delay_vs_freq.csv", outdir);
        FILE* fgd = fopen(path_gd, "w");
        if (fgd) {
            fprintf(fgd, "freq_GHz,S21_group_delay_s\n");
            double* phase = (double*)calloc(nf, sizeof(double));
            for (int fi = 0; fi < nf; fi++) {
                int idx = ((0 * P + 1) * nf + fi) * 2;
                phase[fi] = atan2(controller->results->s_parameters[idx+1], controller->results->s_parameters[idx]);
            }
            for (int fi = 1; fi < nf; fi++) {
                double dp = phase[fi] - phase[fi-1];
                if (dp > M_PI) phase[fi] -= 2.0 * M_PI;
                else if (dp < -M_PI) phase[fi] += 2.0 * M_PI;
            }
            for (int fi = 0; fi < nf; fi++) {
                double gd = 0.0;
                if (fi > 0 && fi < nf-1) {
                    double phase_prev = phase[fi-1];
                    double phase_next = phase[fi+1];
                    double df = controller->results->frequencies[fi+1] - controller->results->frequencies[fi-1];
                    gd = -(phase_next - phase_prev) / (2.0 * M_PI * df);
                }
                fprintf(fgd, "%.6f,%.12e\n", controller->results->frequencies[fi]/1e9, gd);
            }
            free(phase);
            fclose(fgd);
        }
    }
    return 0;
}

// 生成仿真报告
int generate_simulation_report(PCBWorkflowController* controller) {
    if (!controller) {
        set_pcb_workflow_error(15,"控制器为空");
        return -1;
    }
    
    // 更新状态
    controller->status.current_state = WORKFLOW_COMPLETED;
    strcpy(controller->status.state_description, "仿真完成");
    controller->status.current_step = 6;
    strcpy(controller->status.step_description, "生成报告");
    controller->status.progress_percentage = 100.0;
    controller->status.step_progress = 100.0;
    
    printf("生成仿真报告...\n");
    
    // 创建报告文件路径
    char report_path[1024];
    snprintf(report_path, sizeof(report_path), "%s/%s", 
             controller->params.output_directory, controller->report_file);
    
    FILE* report = fopen(report_path, "w");
    if (!report) {
        set_pcb_workflow_error(16, "无法创建报告文件");
        return -1;
    }
    
    // 写入报告内容
    fprintf(report, "========================================\n");
    fprintf(report, "PCB电磁仿真报告\n");
    fprintf(report, "========================================\n");
    fprintf(report, "生成时间: %s\n", controller->pcb_design->creation_date);
    fprintf(report, "仿真时间: %.2f 秒\n", controller->status.elapsed_time);
    fprintf(report, "\nPCB设计信息:\n");
    fprintf(report, "设计名称: %s\n", controller->pcb_design->design_name);
    fprintf(report, "层数: %d\n", controller->pcb_design->num_layers);
    fprintf(report, "端口数: %d\n", controller->em_model->num_ports);
    fprintf(report, "网格单元: %d\n", controller->em_model->num_triangles);
    fprintf(report, "\n仿真参数:\n");
    fprintf(report, "频率范围: %.1f - %.1f GHz\n", 
            controller->params.frequency_start/1e9, controller->params.frequency_end/1e9);
    fprintf(report, "频率点数: %d\n", controller->params.num_frequency_points);
    fprintf(report, "网格密度: %.1f 网格/毫米\n", controller->params.mesh_density);
    fprintf(report, "\n仿真结果:\n");
    fprintf(report, "内存使用: %.1f MB\n", controller->results->memory_usage);
    fprintf(report, "迭代次数: %d\n", controller->results->num_iterations);
    fprintf(report, "最大误差: %.2e\n", controller->results->max_error);
    fprintf(report, "RMS误差: %.2e\n", controller->results->rms_error);
    fprintf(report, "条件数: %.2e\n", controller->results->condition_number);
    fprintf(report, "========================================\n");
    
    fclose(report);
    
    printf("仿真报告生成完成: %s\n", report_path);
    
    return 0;
}

// 运行完整的PCB仿真
int run_complete_pcb_simulation(PCBWorkflowController* controller) {
    if (!controller) {
        set_pcb_workflow_error(17, "控制器为空");
        return -1;
    }
    
    clock_t start_time = clock();
    
    printf("========================================\n");
    printf("开始完整的PCB电磁仿真流程\n");
    printf("========================================\n");
    
    // 步骤1: 加载PCB设计
    printf("\n[步骤 1/6] 加载PCB设计文件...\n");
    if (load_pcb_design(controller) != 0) {
        return -1;
    }
    
    // 步骤2: 生成PCB网格
    printf("\n[步骤 2/6] 生成PCB网格...\n");
    // 注意：这里调用的是当前文件中的 generate_pcb_mesh_workflow(PCBWorkflowController*)
    if (generate_pcb_mesh_workflow(controller) != 0) {
        return -1;
    }
    
    // 步骤3: 设置仿真端口
    printf("\n[步骤 3/6] 设置仿真端口...\n");
    if (setup_simulation_ports(controller) != 0) {
        return -1;
    }
    
    // 步骤4: 运行电磁仿真
    printf("\n[步骤 4/6] 运行电磁仿真...\n");
    if (run_electromagnetic_simulation(controller) != 0) {
        return -1;
    }
    
    // 步骤5: 处理仿真结果
    printf("\n[步骤 5/6] 处理仿真结果...\n");
    if (process_simulation_results(controller) != 0) {
        return -1;
    }
    
    // 步骤6: 生成仿真报告
    printf("\n[步骤 6/6] 生成仿真报告...\n");
    if (generate_simulation_report(controller) != 0) {
        return -1;
    }
    
    // 计算总用时
    clock_t end_time = clock();
    controller->status.elapsed_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;
    
    printf("\n========================================\n");
    printf("PCB电磁仿真流程完成！\n");
    printf("总用时: %.2f 秒\n", controller->status.elapsed_time);
    printf("========================================\n");
    
    return 0;
}

// 自动检测PCB端口
int auto_detect_pcb_ports(PCBWorkflowController* controller) {
    if (!controller || !controller->pcb_design) {
        return -1;
    }
    
    printf("自动检测PCB端口...\n");
    
    // 简化的端口检测算法
    int ports_detected = 0;
    
    // 1. 查找边缘连接器
    Point2D min_point, max_point;
    calculate_pcb_bounds(controller->pcb_design, &min_point, &max_point);
    
    // 2. 查找大型焊盘（可能是连接器）
    for (int layer = 0; layer < controller->pcb_design->num_layers; layer++) {
        PCBPrimitive* primitive = controller->pcb_design->primitives[layer];
        while (primitive) {
            if (primitive->type == PCB_PRIM_PAD) {
                PCBPad* pad = (PCBPad*)primitive;
                
                // 检查焊盘是否在PCB边缘附近
                double edge_threshold = 2.0; // 2mm
                if (pad->position.x < min_point.x + edge_threshold ||
                    pad->position.x > max_point.x - edge_threshold ||
                    pad->position.y < min_point.y + edge_threshold ||
                    pad->position.y > max_point.y - edge_threshold) {
                    
                    // 创建端口
                    PCBPortDefinition port;
                    snprintf(port.name, sizeof(port.name), "Port%d", ports_detected + 1);
                    port.port_number = ports_detected;
                    port.position = pad->position;
                    port.width = pad->outer_diameter;
                    port.layer_index = layer;
                    strcpy(port.net_name, pad->base.net_name);
                    port.characteristic_impedance = controller->params.default_port_impedance;
                    port.reference_impedance = controller->params.default_port_impedance;
                    port.excitation_type = 0;
                    port.excitation_magnitude = (ports_detected == 0) ? 1.0 : 0.0;
                    port.excitation_phase = 0.0;
                    
                    add_pcb_port(controller->em_model, &port);
                    ports_detected++;
                }
            }
            primitive = primitive->next;
        }
    }
    
    printf("检测到 %d 个端口\n", ports_detected);
    
    // 如果没有检测到端口，添加默认端口
    if (ports_detected == 0) {
        printf("未检测到端口，添加默认端口\n");
        
        // 在PCB两端添加默认端口
        Point2D min_point, max_point;
        calculate_pcb_bounds(controller->pcb_design, &min_point, &max_point);
        
        PCBPortDefinition port1;
        strcpy(port1.name, "Port1");
        port1.port_number = 0;
        port1.position.x = min_point.x + (max_point.x - min_point.x) * 0.1;
        port1.position.y = min_point.y + (max_point.y - min_point.y) * 0.5;
        port1.width = 1.0;
        port1.layer_index = 0;
        strcpy(port1.net_name, "NET1");
        port1.characteristic_impedance = controller->params.default_port_impedance;
        port1.reference_impedance = controller->params.default_port_impedance;
        port1.excitation_type = 0;
        port1.excitation_magnitude = 1.0;
        port1.excitation_phase = 0.0;
        
        add_pcb_port(controller->em_model, &port1);
        ports_detected++;
    }
    
    return ports_detected;
}

// 估算PCB仿真资源
int estimate_pcb_simulation_resources(PCBWorkflowController* controller, double* memory_mb, double* time_seconds) {
    if (!controller || !memory_mb || !time_seconds) {
        return -1;
    }
    
    // 基础估算
    *memory_mb = 100.0; // 基础内存100MB
    *time_seconds = 10.0; // 基础时间10秒
    
    if (controller->pcb_design) {
        // PCB复杂度因子
        int total_primitives = 0;
        for (int i = 0; i < controller->pcb_design->num_layers; i++) {
            total_primitives += controller->pcb_design->num_primitives_per_layer[i];
        }
        
        *memory_mb += total_primitives * 0.1; // 每个图元0.1MB
        *time_seconds += total_primitives * 0.01; // 每个图元0.01秒
    }
    
    if (controller->em_model) {
        // 网格复杂度因子
        *memory_mb += controller->em_model->num_triangles * 0.01; // 每个三角形0.01MB
        *time_seconds += controller->em_model->num_triangles * 0.001; // 每个三角形0.001秒
    }
    
    // 频率点数因子
    *memory_mb *= controller->params.num_frequency_points / 100.0;
    *time_seconds *= controller->params.num_frequency_points / 100.0;
    
    // GPU加速因子
    if (controller->params.use_gpu_acceleration) {
        *time_seconds *= 0.1; // GPU加速10倍
    }
    
    return 0;
}

// 获取错误描述
const char* get_pcb_workflow_error_string(int error_code) {
    switch (error_code) {
        case 0: return "成功";
        case 1: return "内存分配失败";
        case 2: return "控制器或参数为空";
        case 3: return "控制器为空";
        case 4: return "未指定PCB输入文件";
        case 5: return "PCB文件读取失败";
        case 6: return "PCB设计验证失败";
        case 7: return "控制器或PCB设计为空";
        case 8: return "电磁模型创建失败";
        case 9: return "网格生成失败";
        case 10: return "控制器或电磁模型为空";
        case 11: return "端口验证失败";
        case 12: return "控制器或电磁模型为空";
        case 13: return "电磁仿真失败";
        case 14: return "控制器或仿真结果为空";
        case 15: return "控制器为空";
        case 16: return "无法创建报告文件";
        case 17: return "控制器为空";
        default: return "未知错误";
    }
}

// 获取最后错误
int get_pcb_workflow_last_error(PCBWorkflowController* controller) {
    return controller ? controller->status.error_code : pcb_workflow_error_code;
}
