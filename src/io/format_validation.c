#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>
#include "format_validation.h"
#include "../utils/memory_utils.h"
#include "../utils/math_utils.h"

#define MAX_ERROR_MESSAGES 10000
#define MAX_RECOVERY_ATTEMPTS 10
#define SEVERITY_THRESHOLD_FATAL 0.9
#define SEVERITY_THRESHOLD_WARNING 0.3
#define VALIDATION_TIMEOUT_DEFAULT 300

FormatValidator* create_format_validator(ValidationLevel level) {
    FormatValidator* validator = (FormatValidator*)safe_malloc(sizeof(FormatValidator));
    memset(validator, 0, sizeof(FormatValidator));
    
    validator->config = (ValidationConfig*)safe_malloc(sizeof(ValidationConfig));
    memset(validator->config, 0, sizeof(ValidationConfig));
    
    validator->config->level = level;
    validator->config->max_errors = MAX_ERROR_MESSAGES;
    validator->config->max_warnings = MAX_ERROR_MESSAGES;
    validator->config->max_severity_threshold = SEVERITY_THRESHOLD_FATAL;
    validator->config->stop_on_fatal_error = true;
    validator->config->enable_auto_fix = true;
    validator->config->enable_auto_recovery = true;
    validator->config->collect_statistics = true;
    validator->config->generate_detailed_report = true;
    validator->config->enable_profiling = true;
    validator->config->timeout_seconds = VALIDATION_TIMEOUT_DEFAULT;
    
    validator->result = (ValidationResult*)safe_malloc(sizeof(ValidationResult));
    memset(validator->result, 0, sizeof(ValidationResult));
    
    validator->result->errors = (FormatError*)safe_malloc(MAX_ERROR_MESSAGES * sizeof(FormatError));
    memset(validator->result->errors, 0, MAX_ERROR_MESSAGES * sizeof(FormatError));
    
    validator->result->max_errors = MAX_ERROR_MESSAGES;
    validator->result->validation_level = level;
    
    validator->statistics = (ValidationStatistics*)safe_malloc(sizeof(ValidationStatistics));
    memset(validator->statistics, 0, sizeof(ValidationStatistics));
    
    validator->recovery_results = (ErrorRecoveryResult**)safe_malloc(MAX_ERROR_MESSAGES * sizeof(ErrorRecoveryResult*));
    memset(validator->recovery_results, 0, MAX_ERROR_MESSAGES * sizeof(ErrorRecoveryResult*));
    
    validator->is_running = false;
    validator->should_stop = false;
    validator->progress_percentage = 0.0;
    validator->start_time = 0;
    validator->elapsed_time_ms = 0;
    
    return validator;
}

void destroy_format_validator(FormatValidator* validator) {
    if (!validator) return;
    
    for (uint32_t i = 0; i < validator->num_recovery_results; i++) {
        if (validator->recovery_results[i]) {
            safe_free(validator->recovery_results[i]->recovery_details);
            safe_free(validator->recovery_results[i]->recovered_data);
            safe_free(validator->recovery_results[i]->manual_review_notes);
            safe_free(validator->recovery_results[i]);
        }
    }
    
    for (uint32_t i = 0; i < validator->result->num_errors; i++) {
        safe_free(validator->result->errors[i].error_message);
        safe_free(validator->result->errors[i].error_details);
        safe_free(validator->result->errors[i].suggested_fix);
        safe_free(validator->result->errors[i].affected_element);
        safe_free(validator->result->errors[i].context_info);
    }
    
    safe_free(validator->result->errors);
    safe_free(validator->result->validation_report);
    safe_free(validator->result);
    safe_free(validator->config);
    safe_free(validator->statistics);
    safe_free(validator->recovery_results);
    safe_free(validator);
}

int configure_validation(FormatValidator* validator, ValidationConfig* config) {
    if (!validator || !config) return -1;
    
    memcpy(validator->config, config, sizeof(ValidationConfig));
    validator->result->validation_level = config->level;
    
    return 0;
}

static void add_error(ValidationResult* result, ErrorType type, const char* message, const char* details, 
                     uint64_t offset, uint32_t line, uint32_t column, bool is_fatal) {
    if (!result || result->num_errors >= result->max_errors) return;
    
    FormatError* error = &result->errors[result->num_errors];
    
    error->error_id = result->num_errors;
    error->error_type = type;
    error->file_offset = offset;
    error->line_number = line;
    error->column_number = column;
    error->error_message = strdup(message);
    error->error_details = details ? strdup(details) : NULL;
    error->is_fatal = is_fatal;
    error->is_recoverable = !is_fatal;
    error->severity_score = is_fatal ? 1.0 : 0.5;
    
    if (is_fatal) {
        result->has_fatal_errors = true;
    }
    
    result->num_errors++;
}

static int validate_gdsii_header(AdvancedFileFormat* format, ValidationResult* result) {
    if (!format || !format->gdsii_header) return -1;
    
    GDSIIHeader* header = format->gdsii_header;
    
    if (header->version < GDSII_V3 || header->version > GDSII_V7) {
        add_error(result, ERROR_TYPE_VERSION, "Invalid GDSII version", NULL, 0, 0, 0, false);
    }
    
    if (header->record_type != 0x00) {
        add_error(result, ERROR_TYPE_SYNTAX, "Invalid header record type", NULL, 0, 0, 0, false);
    }
    
    if (header->data_length < 4 || header->data_length > GDSII_MAX_RECORD_SIZE) {
        add_error(result, ERROR_TYPE_SYNTAX, "Invalid header data length", NULL, 0, 0, 0, true);
    }
    
    return 0;
}

static int validate_gdsii_records(AdvancedFileFormat* format, ValidationResult* result) {
    if (!format) return -1;
    
    for (uint32_t i = 0; i < format->num_cells; i++) {
        GDSIICell* cell = &format->cells[i];
        
        if (strlen(cell->cell_name) == 0) {
            add_error(result, ERROR_TYPE_SYNTAX, "Empty cell name", NULL, 0, 0, 0, false);
        }
        
        if (cell->width < 0 || cell->height < 0) {
            add_error(result, ERROR_TYPE_GEOMETRY, "Invalid cell dimensions", NULL, 0, 0, 0, false);
        }
        
        if (cell->num_elements == 0) {
            add_error(result, ERROR_TYPE_GEOMETRY, "Empty cell", cell->cell_name, 0, 0, 0, false);
        }
    }
    
    return 0;
}

int validate_gdsii_format(AdvancedFileFormat* format, ValidationConfig* config, ValidationResult* result) {
    if (!format || !result) return -1;
    
    clock_t start_time = clock();
    
    if (config->enable_syntax_check) {
        validate_gdsii_header(format, result);
        validate_gdsii_records(format, result);
    }
    
    if (config->enable_semantic_check) {
        for (uint32_t i = 0; i < format->num_cells; i++) {
            GDSIICell* cell = &format->cells[i];
            
            if (cell->parent_cell_id != 0 && cell->parent_cell_id >= format->num_cells) {
                add_error(result, ERROR_TYPE_HIERARCHY, "Invalid parent cell reference", cell->cell_name, 0, 0, 0, false);
            }
            
            for (uint32_t j = 0; j < cell->num_children; j++) {
                if (cell->child_cell_ids[j] >= format->num_cells) {
                    add_error(result, ERROR_TYPE_HIERARCHY, "Invalid child cell reference", cell->cell_name, 0, 0, 0, false);
                }
            }
        }
    }
    
    if (config->enable_geometry_check) {
        for (uint32_t i = 0; i < format->num_cells; i++) {
            GDSIICell* cell = &format->cells[i];
            
            if (cell->width > 0 && cell->height > 0) {
                double aspect_ratio = cell->width / cell->height;
                if (aspect_ratio > 1000.0 || aspect_ratio < 0.001) {
                    add_error(result, ERROR_TYPE_GEOMETRY, "Extreme cell aspect ratio", cell->cell_name, 0, 0, 0, false);
                }
            }
        }
    }
    
    result->validation_time_ms = ((double)(clock() - start_time)) / CLOCKS_PER_SEC * 1000.0;
    
    if (result->num_errors == 0) {
        result->validation_passed = true;
        result->overall_score = 1.0;
    } else {
        result->validation_passed = false;
        result->overall_score = 1.0 - ((double)result->num_errors / (result->num_errors + result->num_warnings + 1));
    }
    
    return 0;
}

int validate_oasis_format(AdvancedFileFormat* format, ValidationConfig* config, ValidationResult* result) {
    if (!format || !result) return -1;
    
    clock_t start_time = clock();
    
    if (config->enable_syntax_check) {
        if (!format->oasis_header) {
            add_error(result, ERROR_TYPE_SYNTAX, "Missing OASIS header", NULL, 0, 0, 0, true);
        } else {
            if (format->oasis_header->version < OASIS_V1_0 || format->oasis_header->version > OASIS_V1_3) {
                add_error(result, ERROR_TYPE_VERSION, "Invalid OASIS version", NULL, 0, 0, 0, false);
            }
        }
    }
    
    result->validation_time_ms = ((double)(clock() - start_time)) / CLOCKS_PER_SEC * 1000.0;
    
    return 0;
}

int validate_gerber_format(AdvancedFileFormat* format, ValidationConfig* config, ValidationResult* result) {
    if (!format || !result) return -1;
    
    clock_t start_time = clock();
    
    if (config->enable_syntax_check) {
        if (!format->gerber_commands || format->num_commands == 0) {
            add_error(result, ERROR_TYPE_SYNTAX, "No Gerber commands found", NULL, 0, 0, 0, true);
        }
        
        for (uint32_t i = 0; i < format->num_commands; i++) {
            GerberCommand* cmd = &format->gerber_commands[i];
            
            if (!cmd->command_code) {
                add_error(result, ERROR_TYPE_SYNTAX, "Missing command code", NULL, 0, i, 0, false);
            }
            
            if (cmd->x_coord < 0 || cmd->y_coord < 0) {
                add_error(result, ERROR_TYPE_GEOMETRY, "Negative coordinates", cmd->command_code, 0, i, 0, false);
            }
        }
    }
    
    if (config->enable_semantic_check) {
        bool has_format_statement = false;
        bool has_unit_statement = false;
        
        for (uint32_t i = 0; i < format->num_commands; i++) {
            GerberCommand* cmd = &format->gerber_commands[i];
            
            if (strcmp(cmd->command_code, "%FS") == 0) {
                has_format_statement = true;
            } else if (strcmp(cmd->command_code, "%MO") == 0) {
                has_unit_statement = true;
            }
        }
        
        if (!has_format_statement) {
            add_error(result, ERROR_TYPE_SEMANTIC, "Missing format statement", NULL, 0, 0, 0, false);
        }
        
        if (!has_unit_statement) {
            add_error(result, ERROR_TYPE_SEMANTIC, "Missing unit statement", NULL, 0, 0, 0, false);
        }
    }
    
    result->validation_time_ms = ((double)(clock() - start_time)) / CLOCKS_PER_SEC * 1000.0;
    
    return 0;
}

int validate_excellon_format(AdvancedFileFormat* format, ValidationConfig* config, ValidationResult* result) {
    if (!format || !result) return -1;
    
    clock_t start_time = clock();
    
    if (config->enable_syntax_check) {
        if (!format->excellon_tools || format->num_tools == 0) {
            add_error(result, ERROR_TYPE_SYNTAX, "No Excellon tools defined", NULL, 0, 0, 0, true);
        }
        
        for (uint32_t i = 0; i < format->num_tools; i++) {
            ExcellonTool* tool = &format->excellon_tools[i];
            
            if (tool->tool_number == 0 || tool->tool_number > EXCELLON_MAX_TOOL_NUMBER) {
                add_error(result, ERROR_TYPE_SYNTAX, "Invalid tool number", NULL, 0, 0, 0, false);
            }
            
            if (tool->tool_diameter <= 0) {
                add_error(result, ERROR_TYPE_GEOMETRY, "Invalid tool diameter", NULL, 0, 0, 0, false);
            }
        }
    }
    
    result->validation_time_ms = ((double)(clock() - start_time)) / CLOCKS_PER_SEC * 1000.0;
    
    return 0;
}

ErrorRecoveryResult* perform_error_recovery(FormatError* error, AdvancedFileFormat* format, RecoveryAction action) {
    if (!error) return NULL;
    
    ErrorRecoveryResult* result = (ErrorRecoveryResult*)safe_malloc(sizeof(ErrorRecoveryResult));
    memset(result, 0, sizeof(ErrorRecoveryResult));
    
    result->action_taken = action;
    result->recovery_successful = false;
    result->requires_manual_review = false;
    
    clock_t start_time = clock();
    
    switch (action) {
        case RECOVERY_ACTION_IGNORE:
            result->recovery_successful = true;
            result->recovery_details = strdup("Error ignored as requested");
            break;
            
        case RECOVERY_ACTION_USE_DEFAULT:
            if (error->error_type == ERROR_TYPE_VERSION) {
                result->recovery_successful = true;
                result->recovery_details = strdup("Using default version");
            }
            break;
            
        case RECOVERY_ACTION_FIX:
            if (error->error_type == ERROR_TYPE_SYNTAX && format) {
                result->recovery_successful = true;
                result->recovery_details = strdup("Attempted automatic syntax fix");
            }
            break;
            
        case RECOVERY_ACTION_SKIP:
            result->recovery_successful = true;
            result->recovery_details = strdup("Skipped problematic element");
            break;
            
        default:
            result->requires_manual_review = true;
            result->recovery_details = strdup("Manual intervention required");
            break;
    }
    
    result->recovery_time_ms = ((double)(clock() - start_time)) / CLOCKS_PER_SEC * 1000.0;
    
    return result;
}

void destroy_error_recovery_result(ErrorRecoveryResult* result) {
    if (!result) return;
    
    safe_free(result->recovery_details);
    safe_free(result->recovered_data);
    safe_free(result->manual_review_notes);
    safe_free(result);
}

int perform_auto_error_recovery(FormatValidator* validator) {
    if (!validator || !validator->result) return -1;
    
    uint32_t num_recovered = 0;
    
    for (uint32_t i = 0; i < validator->result->num_errors; i++) {
        FormatError* error = &validator->result->errors[i];
        
        if (!error->is_recoverable) continue;
        
        RecoveryAction action = RECOVERY_ACTION_USE_DEFAULT;
        
        if (validator->config->enable_auto_fix && error->error_type == ERROR_TYPE_SYNTAX) {
            action = RECOVERY_ACTION_FIX;
        } else if (error->error_type == ERROR_TYPE_GEOMETRY) {
            action = RECOVERY_ACTION_SKIP;
        } else {
            action = RECOVERY_ACTION_IGNORE;
        }
        
        ErrorRecoveryResult* recovery = perform_error_recovery(error, validator->format, action);
        if (recovery && recovery->recovery_successful) {
            validator->recovery_results[validator->num_recovery_results++] = recovery;
            num_recovered++;
            
            if (validator->statistics) {
                validator->statistics->num_auto_recovered++;
            }
        } else {
            destroy_error_recovery_result(recovery);
        }
    }
    
    return num_recovered;
}

int perform_format_validation_advanced(AdvancedFileFormat* format, char** error_messages, uint32_t* num_errors) {
    if (!format || !num_errors) return -1;
    
    ValidationConfig config;
    memset(&config, 0, sizeof(config));
    config.level = VALIDATION_LEVEL_STANDARD;
    config.enable_syntax_check = true;
    config.enable_semantic_check = true;
    config.enable_geometry_check = true;
    config.enable_auto_fix = true;
    config.enable_auto_recovery = true;
    config.stop_on_fatal_error = false;
    config.max_errors = MAX_ERROR_MESSAGES;
    
    FormatValidator* validator = create_format_validator(VALIDATION_LEVEL_STANDARD);
    configure_validation(validator, &config);
    
    int result = 0;
    switch (format->format) {
        case FORMAT_GDSII_BINARY:
        case FORMAT_GDSII_TEXT:
            result = validate_gdsii_format(format, &config, validator->result);
            break;
        case FORMAT_OASIS_BINARY:
            result = validate_oasis_format(format, &config, validator->result);
            break;
        case FORMAT_GERBER_RS274D:
        case FORMAT_GERBER_RS274X:
        case FORMAT_GERBER_X2:
            result = validate_gerber_format(format, &config, validator->result);
            break;
        case FORMAT_EXCELLON:
        case FORMAT_EXCELLON_2:
            result = validate_excellon_format(format, &config, validator->result);
            break;
        default:
            result = -1;
            break;
    }
    
    if (result == 0) {
        perform_auto_error_recovery(validator);
    }
    
    *num_errors = validator->result->num_errors;
    
    if (error_messages && validator->result->num_errors > 0) {
        *error_messages = (char*)safe_malloc(validator->result->num_errors * 512);
        (*error_messages)[0] = '\0';
        
        for (uint32_t i = 0; i < validator->result->num_errors; i++) {
            char error_line[512];
            snprintf(error_line, sizeof(error_line), "Error %d: %s\n", 
                    validator->result->errors[i].error_id, 
                    validator->result->errors[i].error_message);
            strcat(*error_messages, error_line);
        }
    }
    
    destroy_format_validator(validator);
    return result;
}

const char* get_error_type_string(ErrorType type) {
    switch (type) {
        case ERROR_TYPE_NONE: return "None";
        case ERROR_TYPE_SYNTAX: return "Syntax";
        case ERROR_TYPE_SEMANTIC: return "Semantic";
        case ERROR_TYPE_GEOMETRY: return "Geometry";
        case ERROR_TYPE_TOPOLOGY: return "Topology";
        case ERROR_TYPE_ELECTRICAL: return "Electrical";
        case ERROR_TYPE_MATERIAL: return "Material";
        case ERROR_TYPE_HIERARCHY: return "Hierarchy";
        case ERROR_TYPE_COMPRESSION: return "Compression";
        case ERROR_TYPE_MEMORY: return "Memory";
        case ERROR_TYPE_IO: return "I/O";
        case ERROR_TYPE_VERSION: return "Version";
        case ERROR_TYPE_CHECKSUM: return "Checksum";
        default: return "Unknown";
    }
}

const char* get_recovery_action_string(RecoveryAction action) {
    switch (action) {
        case RECOVERY_ACTION_IGNORE: return "Ignore";
        case RECOVERY_ACTION_WARN: return "Warn";
        case RECOVERY_ACTION_FIX: return "Fix";
        case RECOVERY_ACTION_SKIP: return "Skip";
        case RECOVERY_ACTION_ABORT: return "Abort";
        case RECOVERY_ACTION_REPLACE: return "Replace";
        case RECOVERY_ACTION_INTERPOLATE: return "Interpolate";
        case RECOVERY_ACTION_EXTRAPOLATE: return "Extrapolate";
        case RECOVERY_ACTION_USE_DEFAULT: return "Use Default";
        default: return "Unknown";
    }
}

const char* get_validation_level_string(ValidationLevel level) {
    switch (level) {
        case VALIDATION_LEVEL_BASIC: return "Basic";
        case VALIDATION_LEVEL_STANDARD: return "Standard";
        case VALIDATION_LEVEL_STRICT: return "Strict";
        case VALIDATION_LEVEL_PEDANTIC: return "Pedantic";
        default: return "Unknown";
    }
}

double calculate_error_severity(FormatError* error) {
    if (!error) return 0.0;
    
    double base_severity = error->severity_score;
    
    if (error->is_fatal) {
        base_severity += 0.5;
    }
    
    switch (error->error_type) {
        case ERROR_TYPE_SYNTAX:
            base_severity += 0.1;
            break;
        case ERROR_TYPE_SEMANTIC:
            base_severity += 0.2;
            break;
        case ERROR_TYPE_GEOMETRY:
            base_severity += 0.3;
            break;
        case ERROR_TYPE_TOPOLOGY:
            base_severity += 0.4;
            break;
        case ERROR_TYPE_ELECTRICAL:
            base_severity += 0.5;
            break;
        default:
            break;
    }
    
    return (base_severity > 1.0) ? 1.0 : base_severity;
}

double calculate_validation_score(ValidationResult* result) {
    if (!result || result->num_errors == 0) return 1.0;
    
    double total_severity = 0.0;
    for (uint32_t i = 0; i < result->num_errors; i++) {
        total_severity += calculate_error_severity(&result->errors[i]);
    }
    
    double max_possible_severity = result->num_errors * 1.5;
    double score = 1.0 - (total_severity / max_possible_severity);
    
    return (score < 0.0) ? 0.0 : score;
}