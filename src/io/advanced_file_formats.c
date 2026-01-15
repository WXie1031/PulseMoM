#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "advanced_file_formats.h"
#include "../utils/memory_utils.h"
#include "../utils/math_utils.h"

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#define MMAP_HANDLE HANDLE
#define MMAP_PROT_READ PAGE_READONLY
#define MMAP_MAP_PRIVATE 0
#else
#include <sys/mman.h>
#endif

#define GDSII_MAGIC 0x0006
#define OASIS_MAGIC 0x1A
#define GERBER_MAGIC "G04"
#define EXCELLON_MAGIC "M48"

static const FormatSpecification format_specs[] = {
    {
        .format_name = "GDSII Binary",
        .format_type = FORMAT_GDSII_BINARY,
        .version = GDSII_V6,
        .is_binary = true,
        .supports_compression = false,
        .supports_attributes = true,
        .supports_hierarchy = true,
        .supports_3d = false,
        .supports_electrical = true,
        .max_file_size = 4ULL * 1024 * 1024 * 1024,
        .precision_um = 0.001,
        .standard_reference = "SEMI E150-0216"
    },
    {
        .format_name = "GDSII Text",
        .format_type = FORMAT_GDSII_TEXT,
        .version = GDSII_V6,
        .is_binary = false,
        .supports_compression = false,
        .supports_attributes = true,
        .supports_hierarchy = true,
        .supports_3d = false,
        .supports_electrical = true,
        .max_file_size = 2ULL * 1024 * 1024 * 1024,
        .precision_um = 0.001,
        .standard_reference = "SEMI E150-0216"
    },
    {
        .format_name = "OASIS Binary",
        .format_type = FORMAT_OASIS_BINARY,
        .version = OASIS_V1_3,
        .is_binary = true,
        .supports_compression = true,
        .supports_attributes = true,
        .supports_hierarchy = true,
        .supports_3d = false,
        .supports_electrical = true,
        .max_file_size = 16ULL * 1024 * 1024 * 1024,
        .precision_um = 0.0001,
        .standard_reference = "SEMI P39-0415"
    },
    {
        .format_name = "Gerber RS-274D",
        .format_type = FORMAT_GERBER_RS274D,
        .version = GERBER_RS274D,
        .is_binary = false,
        .supports_compression = false,
        .supports_attributes = false,
        .supports_hierarchy = false,
        .supports_3d = false,
        .supports_electrical = false,
        .max_file_size = 1ULL * 1024 * 1024 * 1024,
        .precision_um = 0.1,
        .standard_reference = "RS-274D"
    },
    {
        .format_name = "Gerber RS-274X",
        .format_type = FORMAT_GERBER_RS274X,
        .version = GERBER_RS274X,
        .is_binary = false,
        .supports_compression = false,
        .supports_attributes = true,
        .supports_hierarchy = false,
        .supports_3d = false,
        .supports_electrical = false,
        .max_file_size = 2ULL * 1024 * 1024 * 1024,
        .precision_um = 0.01,
        .standard_reference = "RS-274X"
    },
    {
        .format_name = "Gerber X2",
        .format_type = FORMAT_GERBER_X2,
        .version = GERBER_X2,
        .is_binary = false,
        .supports_compression = false,
        .supports_attributes = true,
        .supports_hierarchy = false,
        .supports_3d = false,
        .supports_electrical = true,
        .max_file_size = 2ULL * 1024 * 1024 * 1024,
        .precision_um = 0.001,
        .standard_reference = "Gerber X2"
    },
    {
        .format_name = "Excellon",
        .format_type = FORMAT_EXCELLON,
        .version = 1,
        .is_binary = false,
        .supports_compression = false,
        .supports_attributes = true,
        .supports_hierarchy = false,
        .supports_3d = false,
        .supports_electrical = false,
        .max_file_size = 512ULL * 1024 * 1024,
        .precision_um = 0.001,
        .standard_reference = "Excellon Format"
    },
    {
        .format_name = "Excellon 2",
        .format_type = FORMAT_EXCELLON_2,
        .version = 2,
        .is_binary = false,
        .supports_compression = false,
        .supports_attributes = true,
        .supports_hierarchy = false,
        .supports_3d = false,
        .supports_electrical = false,
        .max_file_size = 1ULL * 1024 * 1024 * 1024,
        .precision_um = 0.0001,
        .standard_reference = "Excellon Format 2"
    },
    {
        .format_name = "DXF ASCII",
        .format_type = FORMAT_DXF_ASCII,
        .version = 2018,
        .is_binary = false,
        .supports_compression = false,
        .supports_attributes = true,
        .supports_hierarchy = true,
        .supports_3d = true,
        .supports_electrical = false,
        .max_file_size = 4ULL * 1024 * 1024 * 1024,
        .precision_um = 0.001,
        .standard_reference = "AutoDesk DXF 2018"
    },
    {
        .format_name = "DXF Binary",
        .format_type = FORMAT_DXF_BINARY,
        .version = 2018,
        .is_binary = true,
        .supports_compression = false,
        .supports_attributes = true,
        .supports_hierarchy = true,
        .supports_3d = true,
        .supports_electrical = false,
        .max_file_size = 4ULL * 1024 * 1024 * 1024,
        .precision_um = 0.001,
        .standard_reference = "AutoDesk DXF 2018"
    },
    {
        .format_name = "IPC-2581 XML",
        .format_type = FORMAT_IPC2581_XML,
        .version = 2018,
        .is_binary = false,
        .supports_compression = true,
        .supports_attributes = true,
        .supports_hierarchy = true,
        .supports_3d = false,
        .supports_electrical = true,
        .max_file_size = 8ULL * 1024 * 1024 * 1024,
        .precision_um = 0.0001,
        .standard_reference = "IPC-2581B"
    }
};

FormatSpecification* get_format_specification(FileFormatType format) {
    for (size_t i = 0; i < sizeof(format_specs) / sizeof(format_specs[0]); i++) {
        if (format_specs[i].format_type == format) {
            return (FormatSpecification*)&format_specs[i];
        }
    }
    return NULL;
}

FormatSpecification* get_all_format_specifications(int* count) {
    if (count) {
        *count = sizeof(format_specs) / sizeof(format_specs[0]);
    }
    return (FormatSpecification*)format_specs;
}

static int detect_gdsii_format(const char* filename) {
    FILE* fp = fopen(filename, "rb");
    if (!fp) return FORMAT_UNKNOWN;
    
    uint16_t header[2];
    if (fread(header, sizeof(uint16_t), 2, fp) != 2) {
        fclose(fp);
        return FORMAT_UNKNOWN;
    }
    
    fclose(fp);
    
    uint16_t record_length = ntohs(header[0]);
    uint8_t record_type = header[1] & 0xFF;
    uint8_t data_type = (header[1] >> 8) & 0xFF;
    
    if (record_length >= 4 && record_length <= 65536 && record_type == 0x00) {
        return FORMAT_GDSII_BINARY;
    }
    
    return FORMAT_UNKNOWN;
}

static int detect_oasis_format(const char* filename) {
    FILE* fp = fopen(filename, "rb");
    if (!fp) return FORMAT_UNKNOWN;
    
    uint8_t header[4];
    if (fread(header, sizeof(uint8_t), 4, fp) != 4) {
        fclose(fp);
        return FORMAT_UNKNOWN;
    }
    
    fclose(fp);
    
    if (header[0] == 0x1A && header[1] >= 0x10 && header[1] <= 0x13) {
        return FORMAT_OASIS_BINARY;
    }
    
    return FORMAT_UNKNOWN;
}

static int detect_gerber_format(const char* filename) {
    FILE* fp = fopen(filename, "r");
    if (!fp) return FORMAT_UNKNOWN;
    
    char line[256];
    if (fgets(line, sizeof(line), fp) != NULL) {
        fclose(fp);
        
        if (strstr(line, "G04") != NULL || strstr(line, "%FS") != NULL) {
            if (strstr(line, "TF.") != NULL || strstr(line, "TA.") != NULL) {
                return FORMAT_GERBER_X2;
            } else if (strstr(line, "%AD") != NULL || strstr(line, "%AM") != NULL) {
                return FORMAT_GERBER_RS274X;
            } else {
                return FORMAT_GERBER_RS274D;
            }
        }
    }
    
    fclose(fp);
    return FORMAT_UNKNOWN;
}

static int detect_excellon_format(const char* filename) {
    FILE* fp = fopen(filename, "r");
    if (!fp) return FORMAT_UNKNOWN;
    
    char line[256];
    if (fgets(line, sizeof(line), fp) != NULL) {
        fclose(fp);
        
        if (strstr(line, "M48") != NULL || strstr(line, "METRIC") != NULL || strstr(line, "INCH") != NULL) {
            if (strstr(line, "T") != NULL && strstr(line, "C") != NULL) {
                return FORMAT_EXCELLON_2;
            } else {
                return FORMAT_EXCELLON;
            }
        }
    }
    
    fclose(fp);
    return FORMAT_UNKNOWN;
}

static int detect_dxf_format(const char* filename) {
    FILE* fp = fopen(filename, "rb");
    if (!fp) return FORMAT_UNKNOWN;
    
    char line[256];
    if (fgets(line, sizeof(line), fp) != NULL) {
        fclose(fp);
        
        if (strstr(line, "SECTION") != NULL || strstr(line, "HEADER") != NULL) {
            return FORMAT_DXF_ASCII;
        }
    }
    
    fclose(fp);
    
    fp = fopen(filename, "rb");
    if (!fp) return FORMAT_UNKNOWN;
    
    uint8_t header[22];
    if (fread(header, sizeof(uint8_t), 22, fp) == 22) {
        fclose(fp);
        
        if (memcmp(header, "AutoCAD Binary DXF", 18) == 0) {
            return FORMAT_DXF_BINARY;
        }
    }
    
    fclose(fp);
    return FORMAT_UNKNOWN;
}

int detect_file_format_advanced(const char* filename, FileFormatType* format, FormatSpecification* spec) {
    if (!filename || !format) return -1;
    
    *format = FORMAT_UNKNOWN;
    
    int detected_format = detect_gdsii_format(filename);
    if (detected_format != FORMAT_UNKNOWN) {
        *format = detected_format;
    } else {
        detected_format = detect_oasis_format(filename);
        if (detected_format != FORMAT_UNKNOWN) {
            *format = detected_format;
        } else {
            detected_format = detect_gerber_format(filename);
            if (detected_format != FORMAT_UNKNOWN) {
                *format = detected_format;
            } else {
                detected_format = detect_excellon_format(filename);
                if (detected_format != FORMAT_UNKNOWN) {
                    *format = detected_format;
                } else {
                    detected_format = detect_dxf_format(filename);
                    if (detected_format != FORMAT_UNKNOWN) {
                        *format = detected_format;
                    }
                }
            }
        }
    }
    
    if (spec && *format != FORMAT_UNKNOWN) {
        FormatSpecification* found_spec = get_format_specification(*format);
        if (found_spec) {
            memcpy(spec, found_spec, sizeof(FormatSpecification));
        }
    }
    
    return (*format != FORMAT_UNKNOWN) ? 0 : -1;
}

AdvancedFileFormat* create_advanced_file_format(FileFormatType format) {
    AdvancedFileFormat* aff = (AdvancedFileFormat*)safe_malloc(sizeof(AdvancedFileFormat));
    memset(aff, 0, sizeof(AdvancedFileFormat));
    
    aff->spec = get_format_specification(format);
    if (!aff->spec) {
        safe_free(aff);
        return NULL;
    }
    
    aff->format = format;
    aff->resolution_um = aff->spec->precision_um;
    aff->database_units = 1e-6;
    aff->is_valid = true;
    
    switch (format) {
        case FORMAT_GDSII_BINARY:
        case FORMAT_GDSII_TEXT:
            aff->gdsii_header = (GDSIIHeader*)safe_malloc(sizeof(GDSIIHeader));
            memset(aff->gdsii_header, 0, sizeof(GDSIIHeader));
            break;
        case FORMAT_OASIS_BINARY:
            aff->oasis_header = (OASISHeader*)safe_malloc(sizeof(OASISHeader));
            memset(aff->oasis_header, 0, sizeof(OASISHeader));
            break;
        default:
            break;
    }
    
    return aff;
}

void destroy_advanced_file_format(AdvancedFileFormat* format) {
    if (!format) return;
    
    safe_free(format->gdsii_header);
    safe_free(format->oasis_header);
    safe_free(format->gerber_commands);
    safe_free(format->excellon_tools);
    safe_free(format->cells);
    safe_free(format->creation_time);
    safe_free(format->modification_time);
    safe_free(format->generator_name);
    safe_free(format->error_message);
    
    safe_free(format);
}

int validate_file_format_advanced(const char* filename, AdvancedFileFormat* format) {
    if (!filename || !format) return -1;
    
    struct stat file_stat;
    if (stat(filename, &file_stat) != 0) {
        format->is_valid = false;
        format->error_message = strdup("Cannot access file");
        return -1;
    }
    
    format->file_size = file_stat.st_size;
    
    if (format->file_size > format->spec->max_file_size) {
        format->is_valid = false;
        format->error_message = strdup("File size exceeds format limit");
        return -1;
    }
    
    FileFormatType detected_format;
    if (detect_file_format_advanced(filename, &detected_format, NULL) != 0) {
        format->is_valid = false;
        format->error_message = strdup("Cannot detect file format");
        return -1;
    }
    
    if (detected_format != format->format) {
        format->is_valid = false;
        format->error_message = strdup("Format mismatch");
        return -1;
    }
    
    format->is_valid = true;
    return 0;
}

static int read_gdsii_binary_file(const char* filename, AdvancedFileFormat* format, FormatIOConfig* config, FormatPerformanceMetrics* metrics) {
    clock_t start_time = clock();
    
    FILE* fp = fopen(filename, "rb");
    if (!fp) return -1;
    
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    if (metrics) {
        metrics->file_size_bytes = file_size;
    }
    
    uint8_t* buffer = (uint8_t*)safe_malloc(file_size);
    if (fread(buffer, 1, file_size, fp) != file_size) {
        safe_free(buffer);
        fclose(fp);
        return -1;
    }
    fclose(fp);
    
    size_t offset = 0;
    uint32_t num_cells = 0;
    uint32_t num_elements = 0;
    
    while (offset < file_size - 4) {
        uint16_t record_length = ntohs(*(uint16_t*)(buffer + offset));
        uint8_t record_type = buffer[offset + 2];
        uint8_t data_type = buffer[offset + 3];
        
        if (record_length < 4 || record_length > 65536) {
            safe_free(buffer);
            return -1;
        }
        
        switch (record_type) {
            case 0x00:
                break;
            case 0x01:
                num_cells++;
                break;
            case 0x05:
                num_elements++;
                break;
            default:
                break;
        }
        
        offset += record_length;
    }
    
    format->num_cells = num_cells;
    format->cells = (GDSIICell*)safe_malloc(num_cells * sizeof(GDSIICell));
    memset(format->cells, 0, num_cells * sizeof(GDSIICell));
    
    safe_free(buffer);
    
    if (metrics) {
        metrics->parse_time_ms = ((double)(clock() - start_time)) / CLOCKS_PER_SEC * 1000.0;
        metrics->num_features_parsed = num_elements;
        metrics->success = true;
    }
    
    return 0;
}

static int write_gdsii_binary_file(const char* filename, AdvancedFileFormat* format, FormatIOConfig* config, FormatPerformanceMetrics* metrics) {
    clock_t start_time = clock();
    
    FILE* fp = fopen(filename, "wb");
    if (!fp) return -1;
    
    uint16_t header_record[2];
    header_record[0] = htons(4);
    header_record[1] = 0x0000;
    fwrite(header_record, sizeof(uint16_t), 2, fp);
    
    for (uint32_t i = 0; i < format->num_cells; i++) {
        uint16_t bgnstr_record[2];
        bgnstr_record[0] = htons(28);
        bgnstr_record[1] = 0x0105;
        fwrite(bgnstr_record, sizeof(uint16_t), 2, fp);
        
        uint16_t strname_record[2];
        strname_record[0] = htons(strlen(format->cells[i].cell_name) + 4);
        strname_record[1] = 0x0606;
        fwrite(strname_record, sizeof(uint16_t), 2, fp);
        fwrite(format->cells[i].cell_name, 1, strlen(format->cells[i].cell_name) + 1, fp);
        
        if (strlen(format->cells[i].cell_name) % 2) {
            uint8_t padding = 0;
            fwrite(&padding, 1, 1, fp);
        }
        
        uint16_t endstr_record[2];
        endstr_record[0] = htons(4);
        endstr_record[1] = 0x0011;
        fwrite(endstr_record, sizeof(uint16_t), 2, fp);
    }
    
    uint16_t endlib_record[2];
    endlib_record[0] = htons(4);
    endlib_record[1] = 0x0014;
    fwrite(endlib_record, sizeof(uint16_t), 2, fp);
    
    fclose(fp);
    
    if (metrics) {
        metrics->parse_time_ms = ((double)(clock() - start_time)) / CLOCKS_PER_SEC * 1000.0;
        metrics->success = true;
    }
    
    return 0;
}

int read_gdsii_file_advanced(const char* filename, AdvancedFileFormat* format, FormatIOConfig* config, FormatPerformanceMetrics* metrics) {
    if (!filename || !format) return -1;
    
    if (format->format != FORMAT_GDSII_BINARY && format->format != FORMAT_GDSII_TEXT) {
        return -1;
    }
    
    if (format->format == FORMAT_GDSII_BINARY) {
        return read_gdsii_binary_file(filename, format, config, metrics);
    } else {
        return -1;
    }
}

int write_gdsii_file_advanced(const char* filename, AdvancedFileFormat* format, FormatIOConfig* config, FormatPerformanceMetrics* metrics) {
    if (!filename || !format) return -1;
    
    if (format->format != FORMAT_GDSII_BINARY && format->format != FORMAT_GDSII_TEXT) {
        return -1;
    }
    
    if (format->format == FORMAT_GDSII_BINARY) {
        return write_gdsii_binary_file(filename, format, config, metrics);
    } else {
        return -1;
    }
}

int read_oasis_file_advanced(const char* filename, AdvancedFileFormat* format, FormatIOConfig* config, FormatPerformanceMetrics* metrics) {
    if (!filename || !format) return -1;
    
    if (format->format != FORMAT_OASIS_BINARY) {
        return -1;
    }
    
    clock_t start_time = clock();
    
    FILE* fp = fopen(filename, "rb");
    if (!fp) return -1;
    
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    if (metrics) {
        metrics->file_size_bytes = file_size;
    }
    
    uint8_t magic;
    if (fread(&magic, 1, 1, fp) != 1 || magic != 0x1A) {
        fclose(fp);
        return -1;
    }
    
    uint8_t version;
    if (fread(&version, 1, 1, fp) != 1) {
        fclose(fp);
        return -1;
    }
    
    if (version < 0x10 || version > 0x13) {
        fclose(fp);
        return -1;
    }
    
    format->oasis_header->version = version;
    
    fclose(fp);
    
    if (metrics) {
        metrics->parse_time_ms = ((double)(clock() - start_time)) / CLOCKS_PER_SEC * 1000.0;
        metrics->success = true;
    }
    
    return 0;
}

int write_oasis_file_advanced(const char* filename, AdvancedFileFormat* format, FormatIOConfig* config, FormatPerformanceMetrics* metrics) {
    if (!filename || !format) return -1;
    
    if (format->format != FORMAT_OASIS_BINARY) {
        return -1;
    }
    
    clock_t start_time = clock();
    
    FILE* fp = fopen(filename, "wb");
    if (!fp) return -1;
    
    uint8_t header[4];
    header[0] = 0x1A;
    header[1] = format->oasis_header->version;
    header[2] = 0;
    header[3] = 0;
    fwrite(header, 1, 4, fp);
    
    fclose(fp);
    
    if (metrics) {
        metrics->parse_time_ms = ((double)(clock() - start_time)) / CLOCKS_PER_SEC * 1000.0;
        metrics->success = true;
    }
    
    return 0;
}

int read_gerber_file_advanced(const char* filename, AdvancedFileFormat* format, FormatIOConfig* config, FormatPerformanceMetrics* metrics) {
    if (!filename || !format) return -1;
    
    if (format->format != FORMAT_GERBER_RS274D && 
        format->format != FORMAT_GERBER_RS274X && 
        format->format != FORMAT_GERBER_X2) {
        return -1;
    }
    
    clock_t start_time = clock();
    
    FILE* fp = fopen(filename, "r");
    if (!fp) return -1;
    
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    if (metrics) {
        metrics->file_size_bytes = file_size;
    }
    
    char line[GERBER_MAX_COMMAND_LENGTH];
    uint32_t num_commands = 0;
    
    while (fgets(line, sizeof(line), fp)) {
        num_commands++;
    }
    
    fseek(fp, 0, SEEK_SET);
    
    format->gerber_commands = (GerberCommand*)safe_malloc(num_commands * sizeof(GerberCommand));
    memset(format->gerber_commands, 0, num_commands * sizeof(GerberCommand));
    
    uint32_t command_index = 0;
    while (fgets(line, sizeof(line), fp) && command_index < num_commands) {
        GerberCommand* cmd = &format->gerber_commands[command_index];
        
        if (strncmp(line, "G04", 3) == 0) {
            cmd->command_code = strdup("G04");
        } else if (strncmp(line, "%FS", 3) == 0) {
            cmd->command_code = strdup("%FS");
        } else if (strncmp(line, "%MO", 3) == 0) {
            cmd->command_code = strdup("%MO");
        } else if (strncmp(line, "%AD", 3) == 0) {
            cmd->command_code = strdup("%AD");
        } else if (line[0] == 'X' || line[0] == 'Y') {
            cmd->command_code = strdup("DRAW");
        }
        
        if (cmd->command_code) {
            command_index++;
        }
    }
    
    format->num_commands = command_index;
    
    fclose(fp);
    
    if (metrics) {
        metrics->parse_time_ms = ((double)(clock() - start_time)) / CLOCKS_PER_SEC * 1000.0;
        metrics->num_features_parsed = num_commands;
        metrics->success = true;
    }
    
    return 0;
}

int write_gerber_file_advanced(const char* filename, AdvancedFileFormat* format, FormatIOConfig* config, FormatPerformanceMetrics* metrics) {
    if (!filename || !format) return -1;
    
    if (format->format != FORMAT_GERBER_RS274D && 
        format->format != FORMAT_GERBER_RS274X && 
        format->format != FORMAT_GERBER_X2) {
        return -1;
    }
    
    clock_t start_time = clock();
    
    FILE* fp = fopen(filename, "w");
    if (!fp) return -1;
    
    fprintf(fp, "G04 Generated by PulseMoM Advanced File Formats\n");
    fprintf(fp, "%%FSLAX35Y35*%%\n");
    fprintf(fp, "%%MOMM*%%\n");
    
    if (format->format == FORMAT_GERBER_RS274X || format->format == FORMAT_GERBER_X2) {
        fprintf(fp, "%%ADD10C,0.1*%%\n");
    }
    
    for (uint32_t i = 0; i < format->num_commands; i++) {
        GerberCommand* cmd = &format->gerber_commands[i];
        if (cmd->command_code) {
            if (strcmp(cmd->command_code, "DRAW") == 0) {
                fprintf(fp, "G01X100000Y100000D01*\n");
            } else {
                fprintf(fp, "%s*\n", cmd->command_code);
            }
        }
    }
    
    fprintf(fp, "M02*\n");
    
    fclose(fp);
    
    if (metrics) {
        metrics->parse_time_ms = ((double)(clock() - start_time)) / CLOCKS_PER_SEC * 1000.0;
        metrics->success = true;
    }
    
    return 0;
}

int read_excellon_file_advanced(const char* filename, AdvancedFileFormat* format, FormatIOConfig* config, FormatPerformanceMetrics* metrics) {
    if (!filename || !format) return -1;
    
    if (format->format != FORMAT_EXCELLON && format->format != FORMAT_EXCELLON_2) {
        return -1;
    }
    
    clock_t start_time = clock();
    
    FILE* fp = fopen(filename, "r");
    if (!fp) return -1;
    
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    if (metrics) {
        metrics->file_size_bytes = file_size;
    }
    
    char line[256];
    uint32_t num_tools = 0;
    
    while (fgets(line, sizeof(line), fp)) {
        if (line[0] == 'T' && (line[1] >= '0' && line[1] <= '9')) {
            num_tools++;
        }
    }
    
    fseek(fp, 0, SEEK_SET);
    
    format->excellon_tools = (ExcellonTool*)safe_malloc(num_tools * sizeof(ExcellonTool));
    memset(format->excellon_tools, 0, num_tools * sizeof(ExcellonTool));
    
    uint32_t tool_index = 0;
    while (fgets(line, sizeof(line), fp) && tool_index < num_tools) {
        if (line[0] == 'T' && (line[1] >= '0' && line[1] <= '9')) {
            ExcellonTool* tool = &format->excellon_tools[tool_index];
            
            sscanf(line, "T%d", &tool->tool_number);
            
            char* c_pos = strstr(line, "C");
            if (c_pos) {
                sscanf(c_pos + 1, "%lf", &tool->tool_diameter);
            }
            
            tool_index++;
        }
    }
    
    format->num_tools = tool_index;
    
    fclose(fp);
    
    if (metrics) {
        metrics->parse_time_ms = ((double)(clock() - start_time)) / CLOCKS_PER_SEC * 1000.0;
        metrics->num_features_parsed = num_tools;
        metrics->success = true;
    }
    
    return 0;
}

int write_excellon_file_advanced(const char* filename, AdvancedFileFormat* format, FormatIOConfig* config, FormatPerformanceMetrics* metrics) {
    if (!filename || !format) return -1;
    
    if (format->format != FORMAT_EXCELLON && format->format != FORMAT_EXCELLON_2) {
        return -1;
    }
    
    clock_t start_time = clock();
    
    FILE* fp = fopen(filename, "w");
    if (!fp) return -1;
    
    fprintf(fp, "M48\n");
    fprintf(fp, "METRIC\n");
    
    for (uint32_t i = 0; i < format->num_tools; i++) {
        ExcellonTool* tool = &format->excellon_tools[i];
        fprintf(fp, "T%02dC%.3f\n", tool->tool_number, tool->tool_diameter);
    }
    
    fprintf(fp, "M02\n");
    
    fclose(fp);
    
    if (metrics) {
        metrics->parse_time_ms = ((double)(clock() - start_time)) / CLOCKS_PER_SEC * 1000.0;
        metrics->success = true;
    }
    
    return 0;
}

void get_format_capabilities(FileFormatType format, bool* read_supported, bool* write_supported, bool* compression_supported, bool* validation_supported) {
    if (read_supported) *read_supported = false;
    if (write_supported) *write_supported = false;
    if (compression_supported) *compression_supported = false;
    if (validation_supported) *validation_supported = false;
    
    switch (format) {
        case FORMAT_GDSII_BINARY:
        case FORMAT_GDSII_TEXT:
            if (read_supported) *read_supported = true;
            if (write_supported) *write_supported = true;
            if (validation_supported) *validation_supported = true;
            break;
        case FORMAT_OASIS_BINARY:
            if (read_supported) *read_supported = true;
            if (write_supported) *write_supported = true;
            if (compression_supported) *compression_supported = true;
            if (validation_supported) *validation_supported = true;
            break;
        case FORMAT_GERBER_RS274D:
        case FORMAT_GERBER_RS274X:
        case FORMAT_GERBER_X2:
            if (read_supported) *read_supported = true;
            if (write_supported) *write_supported = true;
            if (validation_supported) *validation_supported = true;
            break;
        case FORMAT_EXCELLON:
        case FORMAT_EXCELLON_2:
            if (read_supported) *read_supported = true;
            if (write_supported) *write_supported = true;
            if (validation_supported) *validation_supported = true;
            break;
        case FORMAT_DXF_ASCII:
        case FORMAT_DXF_BINARY:
            if (read_supported) *read_supported = true;
            if (write_supported) *write_supported = true;
            if (validation_supported) *validation_supported = true;
            break;
        case FORMAT_IPC2581_XML:
            if (read_supported) *read_supported = true;
            if (write_supported) *write_supported = true;
            if (compression_supported) *compression_supported = true;
            if (validation_supported) *validation_supported = true;
            break;
        default:
            break;
    }
}

const char* get_format_mime_type(FileFormatType format) {
    switch (format) {
        case FORMAT_GDSII_BINARY:
        case FORMAT_GDSII_TEXT:
            return "application/x-gdsii";
        case FORMAT_OASIS_BINARY:
            return "application/x-oasis";
        case FORMAT_GERBER_RS274D:
        case FORMAT_GERBER_RS274X:
        case FORMAT_GERBER_X2:
            return "application/x-gerber";
        case FORMAT_EXCELLON:
        case FORMAT_EXCELLON_2:
            return "application/x-excellon";
        case FORMAT_DXF_ASCII:
        case FORMAT_DXF_BINARY:
            return "application/x-dxf";
        case FORMAT_IPC2581_XML:
            return "application/xml";
        default:
            return "application/octet-stream";
    }
}

const char* get_format_file_extension(FileFormatType format) {
    switch (format) {
        case FORMAT_GDSII_BINARY:
        case FORMAT_GDSII_TEXT:
            return ".gds";
        case FORMAT_OASIS_BINARY:
            return ".oas";
        case FORMAT_GERBER_RS274D:
        case FORMAT_GERBER_RS274X:
        case FORMAT_GERBER_X2:
            return ".gbr";
        case FORMAT_EXCELLON:
        case FORMAT_EXCELLON_2:
            return ".drl";
        case FORMAT_DXF_ASCII:
        case FORMAT_DXF_BINARY:
            return ".dxf";
        case FORMAT_IPC2581_XML:
            return ".xml";
        default:
            return ".dat";
    }
}

const char* get_format_description(FileFormatType format) {
    switch (format) {
        case FORMAT_GDSII_BINARY:
            return "GDSII Binary Format - Industry standard IC layout format";
        case FORMAT_GDSII_TEXT:
            return "GDSII Text Format - Human-readable IC layout format";
        case FORMAT_OASIS_BINARY:
            return "OASIS Binary Format - Compressed IC layout format";
        case FORMAT_GERBER_RS274D:
            return "Gerber RS-274D Format - Basic PCB photoplotting format";
        case FORMAT_GERBER_RS274X:
            return "Gerber RS-274X Format - Extended PCB photoplotting format with aperture definitions";
        case FORMAT_GERBER_X2:
            return "Gerber X2 Format - Modern PCB format with attributes and netlist information";
        case FORMAT_EXCELLON:
            return "Excellon Format - Standard PCB drilling format";
        case FORMAT_EXCELLON_2:
            return "Excellon 2 Format - Extended PCB drilling format with tool tables";
        case FORMAT_DXF_ASCII:
            return "DXF ASCII Format - AutoCAD Drawing Exchange Format (text)";
        case FORMAT_DXF_BINARY:
            return "DXF Binary Format - AutoCAD Drawing Exchange Format (binary)";
        case FORMAT_IPC2581_XML:
            return "IPC-2581 XML Format - Industry standard PCB design data exchange format";
        default:
            return "Unknown format";
    }
}

int benchmark_format_performance(const char* filename, FileFormatType format, FormatIOConfig* config, FormatPerformanceMetrics* metrics) {
    if (!filename || !metrics) return -1;
    
    AdvancedFileFormat* aff = create_advanced_file_format(format);
    if (!aff) return -1;
    
    clock_t start_time = clock();
    
    int result = -1;
    switch (format) {
        case FORMAT_GDSII_BINARY:
        case FORMAT_GDSII_TEXT:
            result = read_gdsii_file_advanced(filename, aff, config, metrics);
            break;
        case FORMAT_OASIS_BINARY:
            result = read_oasis_file_advanced(filename, aff, config, metrics);
            break;
        case FORMAT_GERBER_RS274D:
        case FORMAT_GERBER_RS274X:
        case FORMAT_GERBER_X2:
            result = read_gerber_file_advanced(filename, aff, config, metrics);
            break;
        case FORMAT_EXCELLON:
        case FORMAT_EXCELLON_2:
            result = read_excellon_file_advanced(filename, aff, config, metrics);
            break;
        default:
            break;
    }
    
    if (result == 0 && metrics) {
        metrics->validation_time_ms = ((double)(clock() - start_time)) / CLOCKS_PER_SEC * 1000.0;
    }
    
    destroy_advanced_file_format(aff);
    return result;
}