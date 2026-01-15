#ifndef FORMAT_VALIDATION_H
#define FORMAT_VALIDATION_H

#include <stdint.h>
#include <stdbool.h>
#include "advanced_file_formats.h"
#include "parallel_io.h"

typedef enum {
    VALIDATION_LEVEL_BASIC = 0,
    VALIDATION_LEVEL_STANDARD = 1,
    VALIDATION_LEVEL_STRICT = 2,
    VALIDATION_LEVEL_PEDANTIC = 3
} ValidationLevel;

typedef enum {
    ERROR_TYPE_NONE = 0,
    ERROR_TYPE_SYNTAX,
    ERROR_TYPE_SEMANTIC,
    ERROR_TYPE_GEOMETRY,
    ERROR_TYPE_TOPOLOGY,
    ERROR_TYPE_ELECTRICAL,
    ERROR_TYPE_MATERIAL,
    ERROR_TYPE_HIERARCHY,
    ERROR_TYPE_COMPRESSION,
    ERROR_TYPE_MEMORY,
    ERROR_TYPE_IO,
    ERROR_TYPE_VERSION,
    ERROR_TYPE_CHECKSUM
} ErrorType;

typedef enum {
    RECOVERY_ACTION_IGNORE = 0,
    RECOVERY_ACTION_WARN,
    RECOVERY_ACTION_FIX,
    RECOVERY_ACTION_SKIP,
    RECOVERY_ACTION_ABORT,
    RECOVERY_ACTION_REPLACE,
    RECOVERY_ACTION_INTERPOLATE,
    RECOVERY_ACTION_EXTRAPOLATE,
    RECOVERY_ACTION_USE_DEFAULT
} RecoveryAction;

typedef struct {
    uint32_t error_id;
    ErrorType error_type;
    uint64_t file_offset;
    uint32_t line_number;
    uint32_t column_number;
    char* error_message;
    char* error_details;
    char* suggested_fix;
    RecoveryAction recommended_action;
    bool is_fatal;
    bool is_recoverable;
    double severity_score;
    char* affected_element;
    char* context_info;
} FormatError;

typedef struct {
    uint32_t num_errors;
    uint32_t num_warnings;
    uint32_t max_errors;
    FormatError* errors;
    bool has_fatal_errors;
    bool validation_passed;
    double overall_score;
    char* validation_report;
    ValidationLevel validation_level;
    uint64_t validation_time_ms;
} ValidationResult;

typedef struct {
    ValidationLevel level;
    bool enable_syntax_check;
    bool enable_semantic_check;
    bool enable_geometry_check;
    bool enable_topology_check;
    bool enable_electrical_check;
    bool enable_material_check;
    bool enable_hierarchy_check;
    bool enable_compression_check;
    bool enable_version_check;
    bool enable_checksum_check;
    bool enable_auto_fix;
    bool enable_auto_recovery;
    uint32_t max_errors;
    uint32_t max_warnings;
    double max_severity_threshold;
    bool stop_on_fatal_error;
    bool collect_statistics;
    bool generate_detailed_report;
    bool enable_profiling;
    uint32_t timeout_seconds;
} ValidationConfig;

typedef struct {
    uint32_t num_syntax_errors;
    uint32_t num_semantic_errors;
    uint32_t num_geometry_errors;
    uint32_t num_topology_errors;
    uint32_t num_electrical_errors;
    uint32_t num_material_errors;
    uint32_t num_hierarchy_errors;
    uint32_t num_compression_errors;
    uint32_t num_version_errors;
    uint32_t num_checksum_errors;
    uint32_t num_auto_fixed;
    uint32_t num_auto_recovered;
    double average_severity_score;
    double max_severity_score;
    uint64_t total_elements_checked;
    uint64_t total_memory_used;
    double validation_speed_mbps;
    double coverage_percentage;
} ValidationStatistics;

typedef struct {
    FormatError* error;
    RecoveryAction action_taken;
    bool recovery_successful;
    char* recovery_details;
    uint64_t recovery_time_ms;
    char* recovered_data;
    uint32_t recovered_data_size;
    bool requires_manual_review;
    char* manual_review_notes;
} ErrorRecoveryResult;

typedef struct {
    ValidationConfig* config;
    ValidationResult* result;
    ValidationStatistics* statistics;
    ErrorRecoveryResult** recovery_results;
    uint32_t num_recovery_results;
    AdvancedFileFormat* format;
    ParallelIOProcessor* io_processor;
    bool is_running;
    bool should_stop;
    double progress_percentage;
    char* current_operation;
    uint64_t start_time;
    uint64_t elapsed_time_ms;
} FormatValidator;

FormatValidator* create_format_validator(ValidationLevel level);
void destroy_format_validator(FormatValidator* validator);

int configure_validation(FormatValidator* validator, ValidationConfig* config);
int start_format_validation(FormatValidator* validator, AdvancedFileFormat* format);
int wait_for_validation_completion(FormatValidator* validator);
int stop_format_validation(FormatValidator* validator);

ValidationResult* perform_format_validation(AdvancedFileFormat* format, ValidationConfig* config, ParallelIOProcessor* io_processor);
void destroy_validation_result(ValidationResult* result);

int validate_gdsii_format(AdvancedFileFormat* format, ValidationConfig* config, ValidationResult* result);
int validate_oasis_format(AdvancedFileFormat* format, ValidationConfig* config, ValidationResult* result);
int validate_gerber_format(AdvancedFileFormat* format, ValidationConfig* config, ValidationResult* result);
int validate_excellon_format(AdvancedFileFormat* format, ValidationConfig* config, ValidationResult* result);
int validate_dxf_format(AdvancedFileFormat* format, ValidationConfig* config, ValidationResult* result);
int validate_ipc2581_format(AdvancedFileFormat* format, ValidationConfig* config, ValidationResult* result);

int validate_format_syntax(AdvancedFileFormat* format, ValidationConfig* config, ValidationResult* result);
int validate_format_semantics(AdvancedFileFormat* format, ValidationConfig* config, ValidationResult* result);
int validate_format_geometry(AdvancedFileFormat* format, ValidationConfig* config, ValidationResult* result);
int validate_format_topology(AdvancedFileFormat* format, ValidationConfig* config, ValidationResult* result);
int validate_format_electrical(AdvancedFileFormat* format, ValidationConfig* config, ValidationResult* result);
int validate_format_material(AdvancedFileFormat* format, ValidationConfig* config, ValidationResult* result);
int validate_format_hierarchy(AdvancedFileFormat* format, ValidationConfig* config, ValidationResult* result);

ErrorRecoveryResult* perform_error_recovery(FormatError* error, AdvancedFileFormat* format, RecoveryAction action);
void destroy_error_recovery_result(ErrorRecoveryResult* result);

int perform_auto_error_recovery(FormatValidator* validator);
int perform_manual_error_recovery(FormatValidator* validator, uint32_t error_id, RecoveryAction action);

int generate_validation_report(FormatValidator* validator, const char* filename);
int generate_error_recovery_report(FormatValidator* validator, const char* filename);

int compare_validation_results(ValidationResult* result1, ValidationResult* result2, char** comparison_report);
int merge_validation_results(ValidationResult** results, uint32_t num_results, ValidationResult* merged_result);

int benchmark_validation_performance(FormatValidator* validator, const char* filename, ValidationStatistics* statistics);
int optimize_validation_parameters(FormatValidator* validator, const char* filename, ValidationConfig* optimized_config);

const char* get_error_type_string(ErrorType type);
const char* get_recovery_action_string(RecoveryAction action);
const char* get_validation_level_string(ValidationLevel level);

double calculate_error_severity(FormatError* error);
double calculate_validation_score(ValidationResult* result);
bool is_error_recoverable(FormatError* error);
bool requires_manual_intervention(FormatError* error);

int suggest_recovery_action(FormatError* error, AdvancedFileFormat* format);
int implement_recovery_action(FormatError* error, AdvancedFileFormat* format, RecoveryAction action, ErrorRecoveryResult* result);

int validate_format_compliance(AdvancedFileFormat* format, const char* standard_reference, ValidationResult* result);
int validate_format_interoperability(AdvancedFileFormat* format, FileFormatType* target_formats, uint32_t num_formats, ValidationResult* result);

#endif