/*********************************************************************
 * Auto-Generated API System Implementation - Commercial-Grade PEEC-MoM Architecture
 * 
 * This module implements the core API generation functionality with
 * support for multiple languages and binding types.
 * 
 * Author: PulseMoM Development Team
 * Copyright (c) 2024
 *********************************************************************/

#include "api_generator.h"
#include <stdarg.h>
#include <sys/stat.h>
#include <unistd.h>

// Default generation options
static const ApiGenerationOptions default_options = {
    .mode = API_MODE_C_INTERFACE | API_MODE_CPP_INTERFACE | API_MODE_PYTHON_BINDING,
    .target_language = API_TARGET_C99,
    .binding_type = API_BINDING_C_API,
    .generate_documentation = true,
    .generate_examples = true,
    .generate_tests = true,
    .generate_error_handling = true,
    .generate_memory_management = true,
    .generate_thread_safety = true,
    .generate_type_safety = true,
    .generate_performance_wrappers = true,
    .generate_debugging_support = true,
    .generate_profiling_support = true,
    .max_line_length = 120,
    .indent_style = "space",
    .indent_size = 4,
    .output_directory = "./generated_api",
    .namespace_prefix = "PulseMoM",
    .function_prefix = "pulse_mom_",
    .class_prefix = "PulseMoM"
};

// Global error tracking
static int g_last_error = 0;
static char g_error_message[1024] = {0};

// Error codes
#define API_GENERATOR_SUCCESS 0
#define API_GENERATOR_ERROR_MEMORY -1
#define API_GENERATOR_ERROR_INVALID_ARGUMENT -2
#define API_GENERATOR_ERROR_FILE_IO -3
#define API_GENERATOR_ERROR_PLUGIN_ANALYSIS -4
#define API_GENERATOR_ERROR_CODE_GENERATION -5
#define API_GENERATOR_ERROR_VALIDATION_FAILED -6
#define API_GENERATOR_ERROR_COMPILATION_FAILED -7

// Forward declarations
static int api_generator_create_output_directory(ApiGeneratorContext* context);
static int api_generator_analyze_plugin_interface(ApiGeneratorContext* context, PluginInterface* plugin);
static int api_generator_generate_c_header(ApiGeneratorContext* context);
static int api_generator_generate_c_implementation(ApiGeneratorContext* context);
static int api_generator_generate_cpp_header(ApiGeneratorContext* context);
static int api_generator_generate_cpp_implementation(ApiGeneratorContext* context);
static int api_generator_generate_python_bindings(ApiGeneratorContext* context);
static int api_generator_generate_matlab_bindings(ApiGeneratorContext* context);
static int api_generator_generate_documentation_files(ApiGeneratorContext* context);
static int api_generator_generate_test_files(ApiGeneratorContext* context);
static int api_generator_generate_example_files(ApiGeneratorContext* context);
static const char* api_generator_get_target_extension(ApiTargetLanguage language);
static const char* api_generator_get_language_standard(ApiTargetLanguage language);

// Context management
ApiGeneratorContext* api_generator_create(const ApiGenerationOptions* options) {
    ApiGeneratorContext* context = (ApiGeneratorContext*)calloc(1, sizeof(ApiGeneratorContext));
    if (!context) {
        g_last_error = API_GENERATOR_ERROR_MEMORY;
        return NULL;
    }
    
    // Copy options
    if (options) {
        memcpy(&context->options, options, sizeof(ApiGenerationOptions));
    } else {
        memcpy(&context->options, &default_options, sizeof(ApiGenerationOptions));
    }
    
    // Initialize context
    context->specification.generation_timestamp = time(NULL);
    strcpy(context->specification.name, "PulseMoM_Unified_API");
    strcpy(context->specification.version, "1.0.0");
    strcpy(context->specification.author, "PulseMoM Development Team");
    strcpy(context->specification.license, "Commercial");
    context->specification.mode = context->options.mode;
    context->specification.target_language = context->options.target_language;
    
    // Allocate arrays for specifications
    context->functions = (FunctionSpecification*)calloc(100, sizeof(FunctionSpecification));
    context->classes = (ClassSpecification*)calloc(50, sizeof(ClassSpecification));
    context->enums = (EnumSpecification*)calloc(50, sizeof(EnumSpecification));
    context->structures = (StructureSpecification*)calloc(50, sizeof(StructureSpecification));
    
    if (!context->functions || !context->classes || !context->enums || !context->structures) {
        api_generator_destroy(context);
        g_last_error = API_GENERATOR_ERROR_MEMORY;
        return NULL;
    }
    
    context->num_functions = 0;
    context->num_classes = 0;
    context->num_enums = 0;
    context->num_structures = 0;
    context->indentation_level = 0;
    context->generated_files_count = 0;
    
    g_last_error = API_GENERATOR_SUCCESS;
    return context;
}

void api_generator_destroy(ApiGeneratorContext* context) {
    if (!context) return;
    
    if (context->current_file) {
        fclose(context->current_file);
    }
    
    free(context->functions);
    free(context->classes);
    free(context->enums);
    free(context->structures);
    free(context);
}

// Main generation functions
int api_generator_analyze_plugins(ApiGeneratorContext* context, PluginManager* plugin_manager) {
    if (!context || !plugin_manager) {
        g_last_error = API_GENERATOR_ERROR_INVALID_ARGUMENT;
        return -1;
    }
    
    context->plugin_manager = plugin_manager;
    
    // Analyze each loaded plugin
    for (int i = 0; i < plugin_manager->num_plugins; i++) {
        PluginInterface* plugin = plugin_manager->plugins[i];
        if (!plugin) continue;
        
        int result = api_generator_analyze_plugin_interface(context, plugin);
        if (result != 0) {
            snprintf(g_error_message, sizeof(g_error_message), 
                    "Failed to analyze plugin: %s", plugin->info.name);
            g_last_error = API_GENERATOR_ERROR_PLUGIN_ANALYSIS;
            return -1;
        }
    }
    
    return 0;
}

int api_generator_generate_all(ApiGeneratorContext* context) {
    if (!context) {
        g_last_error = API_GENERATOR_ERROR_INVALID_ARGUMENT;
        return -1;
    }
    
    // Create output directory
    if (api_generator_create_output_directory(context) != 0) {
        return -1;
    }
    
    // Generate based on mode
    if (context->options.mode & API_MODE_C_INTERFACE) {
        if (api_generator_generate_c_interface(context) != 0) return -1;
    }
    
    if (context->options.mode & API_MODE_CPP_INTERFACE) {
        if (api_generator_generate_cpp_interface(context) != 0) return -1;
    }
    
    if (context->options.mode & API_MODE_PYTHON_BINDING) {
        if (api_generator_generate_python_bindings(context) != 0) return -1;
    }
    
    if (context->options.mode & API_MODE_MATLAB_BINDING) {
        if (api_generator_generate_matlab_bindings(context) != 0) return -1;
    }
    
    // Generate additional files
    if (context->options.generate_documentation) {
        if (api_generator_generate_documentation_files(context) != 0) return -1;
    }
    
    if (context->options.generate_tests) {
        if (api_generator_generate_test_files(context) != 0) return -1;
    }
    
    if (context->options.generate_examples) {
        if (api_generator_generate_example_files(context) != 0) return -1;
    }
    
    return 0;
}

int api_generator_generate_c_interface(ApiGeneratorContext* context) {
    if (!context) {
        g_last_error = API_GENERATOR_ERROR_INVALID_ARGUMENT;
        return -1;
    }
    
    // Generate C header file
    if (api_generator_generate_c_header(context) != 0) return -1;
    
    // Generate C implementation file
    if (api_generator_generate_c_implementation(context) != 0) return -1;
    
    return 0;
}

int api_generator_generate_cpp_interface(ApiGeneratorContext* context) {
    if (!context) {
        g_last_error = API_GENERATOR_ERROR_INVALID_ARGUMENT;
        return -1;
    }
    
    // Generate C++ header file
    if (api_generator_generate_cpp_header(context) != 0) return -1;
    
    // Generate C++ implementation file
    if (api_generator_generate_cpp_implementation(context) != 0) return -1;
    
    return 0;
}

// Plugin analysis function
static int api_generator_analyze_plugin_interface(ApiGeneratorContext* context, PluginInterface* plugin) {
    if (!context || !plugin) return -1;
    
    // Analyze based on plugin type
    switch (plugin->info.type) {
        case PLUGIN_TYPE_SOLVER:
            return api_generator_analyze_solver_plugin(context, plugin);
        case PLUGIN_TYPE_MESH_GENERATION:
            return api_generator_analyze_mesh_plugin(context, plugin);
        case PLUGIN_TYPE_MATERIAL:
            return api_generator_analyze_material_plugin(context, plugin);
        case PLUGIN_TYPE_PREPROCESSOR:
            return api_generator_analyze_preprocessor_plugin(context, plugin);
        case PLUGIN_TYPE_POSTPROCESSOR:
            return api_generator_analyze_postprocessor_plugin(context, plugin);
        default:
            return 0;  // Skip unknown plugin types
    }
}

int api_generator_analyze_solver_plugin(ApiGeneratorContext* context, PluginInterface* plugin) {
    if (!context || !plugin) return -1;
    
    // Add solver initialization function
    FunctionSpecification init_func = {
        .name = "solver_initialize",
        .return_type = "int",
        .parameters = "const char* solver_name, const Framework* framework",
        .description = "Initialize a solver plugin",
        .is_async = false,
        .is_thread_safe = true,
        .is_gpu_accelerated = false,
        .complexity_level = 1,
        .examples = "int result = solver_initialize(\"MoM_Solver\", framework);",
        .error_conditions = "Returns -1 on error, 0 on success",
        .performance_notes = "Lightweight operation, typically < 1ms"
    };
    api_generator_add_function(context, &init_func);
    
    // Add solver configuration function
    FunctionSpecification config_func = {
        .name = "solver_configure",
        .return_type = "int",
        .parameters = "const char* solver_name, const Configuration* config",
        .description = "Configure solver with specific parameters",
        .is_async = false,
        .is_thread_safe = false,
        .is_gpu_accelerated = false,
        .complexity_level = 2,
        .examples = "Configuration config = {0}; config.parallel_threads = 4; solver_configure(\"MoM_Solver\", &config);",
        .error_conditions = "Returns -1 on error, 0 on success",
        .performance_notes = "Medium complexity, depends on configuration"
    };
    api_generator_add_function(context, &config_func);
    
    // Add solver run function
    FunctionSpecification run_func = {
        .name = "solver_solve",
        .return_type = "int",
        .parameters = "const char* solver_name, const ProblemDefinition* problem, SolverResults* results",
        .description = "Run solver on specified problem",
        .is_async = true,
        .is_thread_safe = false,
        .is_gpu_accelerated = true,
        .complexity_level = 5,
        .examples = "ProblemDefinition problem = {0}; SolverResults results = {0}; solver_solve(\"MoM_Solver\", &problem, &results);",
        .error_conditions = "Returns -1 on error, 0 on success",
        .performance_notes = "High complexity, can take minutes to hours"
    };
    api_generator_add_function(context, &run_func);
    
    // Add solver capabilities function
    FunctionSpecification caps_func = {
        .name = "solver_get_capabilities",
        .return_type = "SolverCapabilities*",
        .parameters = "const char* solver_name",
        .description = "Get solver capabilities and limitations",
        .is_async = false,
        .is_thread_safe = true,
        .is_gpu_accelerated = false,
        .complexity_level = 1,
        .examples = "SolverCapabilities* caps = solver_get_capabilities(\"MoM_Solver\");",
        .error_conditions = "Returns NULL on error",
        .performance_notes = "Lightweight operation, typically < 1ms"
    };
    api_generator_add_function(context, &caps_func);
    
    return 0;
}

int api_generator_analyze_mesh_plugin(ApiGeneratorContext* context, PluginInterface* plugin) {
    if (!context || !plugin) return -1;
    
    // Add mesh generation function
    FunctionSpecification mesh_func = {
        .name = "mesh_generate",
        .return_type = "int",
        .parameters = "const char* plugin_name, const GeometryData* geometry, MeshData* mesh",
        .description = "Generate mesh for given geometry",
        .is_async = false,
        .is_thread_safe = false,
        .is_gpu_accelerated = false,
        .complexity_level = 4,
        .examples = "MeshData mesh = {0}; mesh_generate(\"CAD_Mesher\", &geometry, &mesh);",
        .error_conditions = "Returns -1 on error, 0 on success",
        .performance_notes = "Medium to high complexity, depends on geometry complexity"
    };
    api_generator_add_function(context, &mesh_func);
    
    return 0;
}

int api_generator_analyze_material_plugin(ApiGeneratorContext* context, PluginInterface* plugin) {
    if (!context || !plugin) return -1;
    
    // Add material property function
    FunctionSpecification mat_func = {
        .name = "material_get_properties",
        .return_type = "MaterialProperties*",
        .parameters = "const char* material_name, double frequency_hz",
        .description = "Get material properties at specified frequency",
        .is_async = false,
        .is_thread_safe = true,
        .is_gpu_accelerated = false,
        .complexity_level = 2,
        .examples = "MaterialProperties* props = material_get_properties(\"FR4\", 1e9);",
        .error_conditions = "Returns NULL on error",
        .performance_notes = "Lightweight operation, typically < 1ms"
    };
    api_generator_add_function(context, &mat_func);
    
    return 0;
}

int api_generator_analyze_preprocessor_plugin(ApiGeneratorContext* context, PluginInterface* plugin) {
    if (!context || !plugin) return -1;
    
    // Add preprocessing function
    FunctionSpecification pre_func = {
        .name = "preprocess_geometry",
        .return_type = "int",
        .parameters = "const char* plugin_name, GeometryData* geometry",
        .description = "Preprocess geometry for simulation",
        .is_async = false,
        .is_thread_safe = false,
        .is_gpu_accelerated = false,
        .complexity_level = 3,
        .examples = "preprocess_geometry(\"Geometry_Cleaner\", &geometry);",
        .error_conditions = "Returns -1 on error, 0 on success",
        .performance_notes = "Medium complexity, depends on geometry size"
    };
    api_generator_add_function(context, &pre_func);
    
    return 0;
}

int api_generator_analyze_postprocessor_plugin(ApiGeneratorContext* context, PluginInterface* plugin) {
    if (!context || !plugin) return -1;
    
    // Add postprocessing function
    FunctionSpecification post_func = {
        .name = "postprocess_results",
        .return_type = "int",
        .parameters = "const char* plugin_name, SolverResults* results",
        .description = "Postprocess solver results",
        .is_async = false,
        .is_thread_safe = false,
        .is_gpu_accelerated = false,
        .complexity_level = 3,
        .examples = "postprocess_results(\"Field_Visualizer\", &results);",
        .error_conditions = "Returns -1 on error, 0 on success",
        .performance_notes = "Medium complexity, depends on result size"
    };
    api_generator_add_function(context, &post_func);
    
    return 0;
}

// Utility functions
int api_generator_add_function(ApiGeneratorContext* context, const FunctionSpecification* func) {
    if (!context || !func || context->num_functions >= 100) return -1;
    
    context->functions[context->num_functions++] = *func;
    return 0;
}

int api_generator_add_class(ApiGeneratorContext* context, const ClassSpecification* cls) {
    if (!context || !cls || context->num_classes >= 50) return -1;
    
    context->classes[context->num_classes++] = *cls;
    return 0;
}

int api_generator_add_enum(ApiGeneratorContext* context, const EnumSpecification* enum_spec) {
    if (!context || !enum_spec || context->num_enums >= 50) return -1;
    
    context->enums[context->num_enums++] = *enum_spec;
    return 0;
}

int api_generator_add_structure(ApiGeneratorContext* context, const StructureSpecification* struct_spec) {
    if (!context || !struct_spec || context->num_structures >= 50) return -1;
    
    context->structures[context->num_structures++] = *struct_spec;
    return 0;
}

void api_generator_set_indentation(ApiGeneratorContext* context, int level) {
    if (!context) return;
    context->indentation_level = level;
}

void api_generator_write_line(ApiGeneratorContext* context, const char* format, ...) {
    if (!context || !format) return;
    
    va_list args;
    va_start(args, format);
    
    // Write indentation
    for (int i = 0; i < context->indentation_level; i++) {
        fprintf(context->current_file, "%*s", context->options.indent_size, "");
    }
    
    // Write formatted line
    vfprintf(context->current_file, format, args);
    fprintf(context->current_file, "\n");
    
    va_end(args);
}

void api_generator_write_comment(ApiGeneratorContext* context, const char* comment) {
    if (!context || !comment) return;
    
    // Write appropriate comment style based on target language
    const char* comment_style = "//";
    if (context->options.target_language == API_TARGET_C99 ||
        context->options.target_language == API_TARGET_FORTRAN77 ||
        context->options.target_language == API_TARGET_FORTRAN90 ||
        context->options.target_language == API_TARGET_FORTRAN2003) {
        comment_style = "/* */";
    }
    
    api_generator_write_line(context, "%s %s", comment_style, comment);
}

// File generation helpers
static int api_generator_create_output_directory(ApiGeneratorContext* context) {
    if (!context) return -1;
    
    struct stat st = {0};
    if (stat(context->options.output_directory, &st) == -1) {
        if (mkdir(context->options.output_directory, 0755) != 0) {
            snprintf(g_error_message, sizeof(g_error_message), 
                    "Failed to create output directory: %s", context->options.output_directory);
            g_last_error = API_GENERATOR_ERROR_FILE_IO;
            return -1;
        }
    }
    
    return 0;
}

static int api_generator_generate_c_header(ApiGeneratorContext* context) {
    if (!context) return -1;
    
    char header_path[1024];
    snprintf(header_path, sizeof(header_path), "%s/pulse_mom_api.h", context->options.output_directory);
    
    context->current_file = fopen(header_path, "w");
    if (!context->current_file) {
        g_last_error = API_GENERATOR_ERROR_FILE_IO;
        return -1;
    }
    
    // Write header guard and includes
    api_generator_write_line(context, "/*********************************************************************");
    api_generator_write_line(context, " * Auto-Generated API Header - PulseMoM Unified Framework");
    api_generator_write_line(context, " * Generated on: %s", ctime(&context->specification.generation_timestamp));
    api_generator_write_line(context, " * Target Language: C99");
    api_generator_write_line(context, " *********************************************************************/");
    api_generator_write_line(context, "");
    api_generator_write_line(context, "#ifndef PULSE_MOM_API_H");
    api_generator_write_line(context, "#define PULSE_MOM_API_H");
    api_generator_write_line(context, "");
    api_generator_write_line(context, "#include <complex.h>");
    api_generator_write_line(context, "#include <stdbool.h>");
    api_generator_write_line(context, "#include <stdint.h>");
    api_generator_write_line(context, "");
    
    // Write function declarations
    for (int i = 0; i < context->num_functions; i++) {
        api_generator_write_function_header(context, &context->functions[i]);
    }
    
    api_generator_write_line(context, "");
    api_generator_write_line(context, "#endif // PULSE_MOM_API_H");
    
    fclose(context->current_file);
    context->current_file = NULL;
    context->generated_files_count++;
    
    return 0;
}

void api_generator_write_function_header(ApiGeneratorContext* context, const FunctionSpecification* func) {
    if (!context || !func) return;
    
    if (context->options.generate_documentation) {
        api_generator_write_line(context, "/**");
        api_generator_write_line(context, " * @brief %s", func->description);
        api_generator_write_line(context, " *");
        api_generator_write_line(context, " * @param %s", func->parameters);
        api_generator_write_line(context, " * @return %s", func->return_type);
        api_generator_write_line(context, " * @note %s", func->performance_notes);
        api_generator_write_line(context, " * @warning %s", func->error_conditions);
        api_generator_write_line(context, " */");
    }
    
    api_generator_write_line(context, "%s %s(%s);", func->return_type, func->name, func->parameters);
}

// Error handling
const char* api_generator_get_error_string(int error_code) {
    switch (error_code) {
        case API_GENERATOR_SUCCESS: return "Success";
        case API_GENERATOR_ERROR_MEMORY: return "Memory allocation failed";
        case API_GENERATOR_ERROR_INVALID_ARGUMENT: return "Invalid argument";
        case API_GENERATOR_ERROR_FILE_IO: return "File I/O error";
        case API_GENERATOR_ERROR_PLUGIN_ANALYSIS: return "Plugin analysis failed";
        case API_GENERATOR_ERROR_CODE_GENERATION: return "Code generation failed";
        case API_GENERATOR_ERROR_VALIDATION_FAILED: return "Validation failed";
        case API_GENERATOR_ERROR_COMPILATION_FAILED: return "Compilation failed";
        default: return "Unknown error";
    }
}

int api_generator_get_last_error(void) {
    return g_last_error;
}