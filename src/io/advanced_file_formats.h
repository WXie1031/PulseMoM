#ifndef ADVANCED_FILE_FORMATS_H
#define ADVANCED_FILE_FORMATS_H

#include <stdint.h>
#include <stdbool.h>
#include <complex.h>
// #include "pcb_structure.h"  // File not found, commented out

#define GDSII_MAX_RECORD_SIZE 65536
#define GDSII_MAX_STRING_LENGTH 1024
#define OASIS_MAX_TABLE_SIZE 1000000
#define GERBER_MAX_COMMAND_LENGTH 1024
#define EXCELLON_MAX_TOOL_NUMBER 999

typedef enum {
    FORMAT_UNKNOWN = 0,
    FORMAT_GDSII_BINARY,
    FORMAT_GDSII_TEXT,
    FORMAT_OASIS_BINARY,
    FORMAT_OASIS_TEXT,
    FORMAT_GERBER_RS274D,
    FORMAT_GERBER_RS274X,
    FORMAT_GERBER_X2,
    FORMAT_EXCELLON,
    FORMAT_EXCELLON_2,
    FORMAT_DXF_ASCII,
    FORMAT_DXF_BINARY,
    FORMAT_IPC2581_XML,
    FORMAT_ODBPP,
    FORMAT_KICAD_PCB,
    FORMAT_ALLEGRO_BRD,
    FORMAT_ALTIUM_PCBDOC,
    FORMAT_EAGLE_XML,
    FORMAT_SIP_LAYOUT,
    FORMAT_SYSTEM_C
} FileFormatType;

typedef enum {
    GDSII_V3 = 3,
    GDSII_V4 = 4,
    GDSII_V5 = 5,
    GDSII_V6 = 6,
    GDSII_V7 = 7
} GDSIIVersion;

typedef enum {
    OASIS_V1_0 = 0x10,
    OASIS_V1_1 = 0x11,
    OASIS_V1_2 = 0x12,
    OASIS_V1_3 = 0x13
} OASISVersion;

typedef enum {
    GERBER_RS274D = 0,
    GERBER_RS274X = 1,
    GERBER_X2 = 2
} GerberVersion;

typedef struct {
    uint32_t signature;
    uint8_t version;
    uint8_t record_type;
    uint16_t data_length;
    uint32_t timestamp;
    uint32_t modification_time;
    uint32_t access_time;
    uint16_t library_size;
} GDSIIHeader;

typedef struct {
    uint32_t magic_number;
    uint8_t version;
    uint8_t unit;
    uint16_t reserved;
    uint32_t offset_strings;
    uint32_t offset_cells;
    uint32_t offset_geometry;
    uint32_t file_size;
} OASISHeader;

typedef struct {
    char* format_name;
    FileFormatType format_type;
    uint32_t version;
    bool is_binary;
    bool supports_compression;
    bool supports_attributes;
    bool supports_hierarchy;
    bool supports_3d;
    bool supports_electrical;
    size_t max_file_size;
    double precision_um;
    char* standard_reference;
} FormatSpecification;

typedef struct {
    char* cell_name;
    uint32_t cell_id;
    double origin_x;
    double origin_y;
    double width;
    double height;
    uint32_t num_elements;
    uint32_t* element_ids;
    bool is_top_level;
    uint32_t parent_cell_id;
    uint32_t* child_cell_ids;
    uint32_t num_children;
} GDSIICell;

typedef struct {
    uint16_t record_type;
    uint16_t data_type;
    uint32_t data_length;
    uint8_t* data;
    char* string_data;
    int16_t* short_data;
    int32_t* long_data;
    double* double_data;
} GDSIIRecord;

typedef struct {
    uint8_t info_type;
    uint8_t info_length;
    uint32_t table_offset;
    uint32_t table_size;
    uint8_t* table_data;
} OASISTableInfo;

typedef struct {
    uint8_t geometry_type;
    uint8_t info_byte;
    uint16_t layer_id;
    uint16_t datatype;
    uint32_t num_vertices;
    double* x_vertices;
    double* y_vertices;
    double width;
    double height;
    double rotation;
    double magnification;
    uint32_t reference_cell_id;
    uint8_t* property_data;
    uint32_t property_size;
} OASISGeometry;

typedef struct {
    char* command_code;
    char* parameters;
    double x_coord;
    double y_coord;
    double i_param;
    double j_param;
    double aperture_size;
    char* aperture_shape;
    uint16_t layer_id;
    uint16_t polarity;
    bool is_region;
    bool is_arc;
    bool is_circle;
    bool is_line;
    uint32_t interpolation_points;
} GerberCommand;

typedef struct {
    uint32_t tool_number;
    double tool_diameter;
    double tool_angle;
    char* tool_shape;
    uint32_t spindle_speed;
    double feed_rate;
    double retract_height;
    bool is_plated;
    bool is_via;
    uint32_t layer_id;
} ExcellonTool;

typedef struct {
    FormatSpecification* spec;
    GDSIIHeader* gdsii_header;
    OASISHeader* oasis_header;
    GerberCommand* gerber_commands;
    ExcellonTool* excellon_tools;
    uint32_t num_commands;
    uint32_t num_tools;
    uint32_t num_cells;
    GDSIICell* cells;
    double resolution_um;
    double database_units;
    char* creation_time;
    char* modification_time;
    char* generator_name;
    uint32_t file_size;
    bool is_compressed;
    bool is_valid;
    char* error_message;
} AdvancedFileFormat;

typedef struct {
    char* layer_name;
    uint32_t layer_id;
    uint16_t gdsii_layer;
    uint16_t gdsii_datatype;
    uint16_t oasis_layer;
    uint16_t oasis_datatype;
    double thickness_um;
    double elevation_um;
    char* material_name;
    char* purpose;
    bool is_negative;
    bool is_boundary;
    double conductivity;
    double epsilon_r;
    double loss_tangent;
} FormatLayerMapping;

typedef struct {
    AdvancedFileFormat* format;
    FormatLayerMapping* layer_mappings;
    uint32_t num_layer_mappings;
    void* pcb_model;  // PCBEMModel* - type not available, using void*
    double min_feature_size_um;
    double max_feature_size_um;
    double bounding_box[4];
    uint32_t num_vertices;
    uint32_t num_polygons;
    uint32_t num_paths;
    uint32_t num_texts;
    uint32_t num_references;
    bool has_electrical_info;
    bool has_3d_info;
    bool has_material_info;
} FormatGeometryData;

typedef struct {
    uint32_t num_threads;
    size_t chunk_size;
    bool use_memory_mapping;
    bool use_compression;
    bool validate_on_load;
    bool enable_error_recovery;
    double timeout_seconds;
    uint32_t max_memory_mb;
    char* temp_directory;
} FormatIOConfig;

typedef struct {
    double parse_time_ms;
    double validation_time_ms;
    double conversion_time_ms;
    size_t peak_memory_bytes;
    size_t file_size_bytes;
    uint32_t num_errors;
    uint32_t num_warnings;
    uint32_t num_features_parsed;
    bool success;
    char* error_details;
} FormatPerformanceMetrics;

FormatSpecification* get_format_specification(FileFormatType format);
FormatSpecification* get_all_format_specifications(int* count);

AdvancedFileFormat* create_advanced_file_format(FileFormatType format);
void destroy_advanced_file_format(AdvancedFileFormat* format);

int detect_file_format_advanced(const char* filename, FileFormatType* format, FormatSpecification* spec);
int validate_file_format_advanced(const char* filename, AdvancedFileFormat* format);

int read_gdsii_file_advanced(const char* filename, AdvancedFileFormat* format, FormatIOConfig* config, FormatPerformanceMetrics* metrics);
int write_gdsii_file_advanced(const char* filename, AdvancedFileFormat* format, FormatIOConfig* config, FormatPerformanceMetrics* metrics);

int read_oasis_file_advanced(const char* filename, AdvancedFileFormat* format, FormatIOConfig* config, FormatPerformanceMetrics* metrics);
int write_oasis_file_advanced(const char* filename, AdvancedFileFormat* format, FormatIOConfig* config, FormatPerformanceMetrics* metrics);

int read_gerber_file_advanced(const char* filename, AdvancedFileFormat* format, FormatIOConfig* config, FormatPerformanceMetrics* metrics);
int write_gerber_file_advanced(const char* filename, AdvancedFileFormat* format, FormatIOConfig* config, FormatPerformanceMetrics* metrics);

int read_excellon_file_advanced(const char* filename, AdvancedFileFormat* format, FormatIOConfig* config, FormatPerformanceMetrics* metrics);
int write_excellon_file_advanced(const char* filename, AdvancedFileFormat* format, FormatIOConfig* config, FormatPerformanceMetrics* metrics);

FormatGeometryData* convert_format_to_geometry(AdvancedFileFormat* format, void* pcb_model);  // PCBEMModel* - type not available
int convert_geometry_to_format(FormatGeometryData* geometry, AdvancedFileFormat* format);

int perform_format_validation_advanced(AdvancedFileFormat* format, char** error_messages, uint32_t* num_errors);
int perform_format_error_recovery(AdvancedFileFormat* format, char** recovered_data, uint32_t* recovered_size);

int compare_format_versions(AdvancedFileFormat* format1, AdvancedFileFormat* format2, char** differences);
int merge_format_files(AdvancedFileFormat** formats, uint32_t num_formats, AdvancedFileFormat* merged_format);

int optimize_format_file_advanced(const char* input_filename, const char* output_filename, FileFormatType target_format, FormatIOConfig* config);
int batch_convert_formats(const char** input_files, uint32_t num_files, FileFormatType target_format, const char* output_directory, FormatIOConfig* config);

void get_format_capabilities(FileFormatType format, bool* read_supported, bool* write_supported, bool* compression_supported, bool* validation_supported);
const char* get_format_mime_type(FileFormatType format);
const char* get_format_file_extension(FileFormatType format);
const char* get_format_description(FileFormatType format);

int benchmark_format_performance(const char* filename, FileFormatType format, FormatIOConfig* config, FormatPerformanceMetrics* metrics);
int compare_format_efficiency(FileFormatType format1, FileFormatType format2, const char* test_file, FormatPerformanceMetrics* metrics1, FormatPerformanceMetrics* metrics2);

#endif