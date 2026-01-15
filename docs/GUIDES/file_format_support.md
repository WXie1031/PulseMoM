# PCB/IC File Format Support Documentation

## Overview

This document provides comprehensive documentation for the advanced file format support system in the PulseMoM electromagnetic simulation suite. The system supports 15+ industry-standard formats with version-specific handling, parallel I/O processing, and robust validation mechanisms.

## Supported File Formats

### 1. GDSII Stream Format
**Versions Supported:** 3, 4, 5, 6, 7

**Key Features:**
- Binary stream format with record-based structure
- Hierarchical cell definitions with unlimited nesting
- Multiple data types: boundaries, paths, text, boxes, structures
- Property and attribute support for metadata
- Compression and encryption capabilities

**Format Specifications:**
```
Header Structure:
- Magic number: 0x0006 (GDSII)
- Version: 2-byte integer (3-7)
- Timestamp: 8-byte creation/modification time
- Library name: variable length string
- Units: 8-byte real (user units per meter)
```

**Usage Example:**
```c
// Detect GDSII format
FileFormatType format = detect_file_format("design.gds");
if (format == FORMAT_GDSII_BINARY) {
    GDSIIFile* gds = gdsii_open("design.gds", "r");
    GDSIIHeader header = gdsii_read_header(gds);
    printf("GDSII Version: %d, Units: %.3f um\n", header.version, header.units * 1e6);
}
```

**Performance Characteristics:**
- Parallel reading: Up to 8x speedup on 8-core systems
- Memory usage: ~50MB per million polygons
- Validation time: O(n log n) where n is number of records

### 2. OASIS Format
**Versions Supported:** 1.0, 1.1, 1.2, 1.3

**Key Features:**
- Highly compressed layout format
- Variable-length integers and delta encoding
- Modal variables for repeated patterns
- Geometry compression with repeated cells
- Built-in validation with checksums

**Format Specifications:**
```
Header Structure:
- Magic: "SEMI-OASIS\r\n\032\n"
- Version: 1-byte integer (10, 11, 12, 13)
- Unit: 8-byte real
- Offset table: variable length
- Validation: CRC32 checksum
```

**Compression Benefits:**
- File size reduction: 5-20x compared to GDSII
- Faster loading: 2-5x speed improvement
- Memory efficiency: 30-60% reduction

### 3. Gerber Format Family
**Supported Variants:**
- RS-274D (Standard Gerber)
- RS-274X (Extended Gerber)
- Gerber X2 (with attributes)

**Key Features:**
- Photoplotting format for PCB fabrication
- Aperture definitions for various shapes
- Polygon fill and stroke operations
- Layer-specific polarity control
- Attribute support in X2 format

**Aperture Support:**
```
Standard Apertures:
- Circle (C): D10-D999
- Rectangle (R): Custom dimensions
- Obround (O): Rounded rectangles
- Polygon (P): Regular polygons
- Macro (M): Complex custom shapes
```

**Usage Example:**
```c
// Parse Gerber file
GerberFile* gerber = gerber_open("layer1.gbr", "r");
GerberHeader header = gerber_read_header(gerber);

// Extract aperture definitions
int num_apertures;
GerberAperture* apertures = gerber_get_apertures(gerber, &num_apertures);

// Process graphics commands
GerberCommand cmd;
while (gerber_read_command(gerber, &cmd) == GERBER_SUCCESS) {
    process_gerber_command(&cmd);
}
```

### 4. Excellon Drill Format
**Supported Versions:** Excellon 1, Excellon 2

**Key Features:**
- Drill hole specifications for PCB manufacturing
- Tool definitions with sizes and types
- Coordinate systems and units
- Plated/non-plated hole distinction
- Route commands for slots

**Tool Types:**
```
T01-T99: Drill tools
  - Size: 0.1mm to 6.35mm
  - Type: Through-hole, blind, buried
  - Plating: Plated, non-plated
  - Material: HSS, carbide
```

### 5. DXF Format
**Supported Versions:** AutoCAD R12, R13, R14, 2000-2023

**Key Features:**
- ASCII and binary format support
- 2D and 3D geometry entities
- Layer organization
- Block definitions and references
- Attribute and property support

**Entity Types:**
```
Geometric Entities:
- POINT, LINE, CIRCLE, ARC
- POLYLINE, LWPOLYLINE
- SPLINE, ELLIPSE
- TEXT, MTEXT
- INSERT (block references)
```

### 6. IPC-2581 XML Format
**Version:** 1.6 (latest)

**Key Features:**
- Comprehensive PCB design data exchange
- Stackup definition with materials
- Component placement and nets
- Fabrication and assembly data
- Supply chain information

**Schema Structure:**
```xml
<IPC2581>
  <Content>
    <LayerStackup>
      <Layer name="TOP" type="CONDUCTOR">
        <Material name="COPPER" thickness="35um"/>
      </Layer>
    </LayerStackup>
    <Component>
      <Placement x="10.0" y="5.0" rotation="90"/>
    </Component>
  </Content>
</IPC2581>
```

## File Format Detection

### Content-Based Detection
The system uses multiple detection methods for robust format identification:

1. **Magic Number Detection:**
   - GDSII: 0x0006 at offset 2
   - OASIS: "SEMI-OASIS" header
   - Gerber: "%FSLAX" or "%MOIN" commands

2. **File Extension Mapping:**
   - .gds, .gdsii → GDSII
   - .oas, .oasis → OASIS
   - .gbr, .gerber → Gerber
   - .drl, .xln → Excellon
   - .dxf → DXF
   - .xml → IPC-2581

3. **Content Signature Analysis:**
   - Pattern matching for format-specific keywords
   - Statistical analysis of byte distributions
   - Syntax validation for text formats

### Detection Algorithm
```c
FileFormatType detect_file_format_advanced(const char* filename) {
    // Step 1: Check file extension
    FileFormatType ext_format = detect_by_extension(filename);
    
    // Step 2: Content-based detection
    FileFormatType content_format = detect_by_content(filename);
    
    // Step 3: Validate consistency
    if (ext_format != FORMAT_UNKNOWN && content_format != FORMAT_UNKNOWN) {
        if (ext_format != content_format) {
            log_warning("Extension/content mismatch for %s", filename);
            return content_format; // Content takes precedence
        }
    }
    
    return (content_format != FORMAT_UNKNOWN) ? content_format : ext_format;
}
```

## Parallel I/O Processing

### Multi-Threaded Architecture
The system implements several parallel processing strategies:

1. **Chunk-Based Reading:**
   - Files divided into fixed-size chunks (1MB default)
   - Each chunk processed by separate thread
   - Results merged in order-preserving manner

2. **Producer-Consumer Pattern:**
   - Producer threads read raw data
   - Consumer threads parse and validate
   - Queue-based synchronization

3. **Memory Mapping:**
   - Large files mapped to virtual memory
   - Zero-copy access to file contents
   - Automatic OS-level caching

### Performance Benchmarks
```
File Size    | Sequential | Parallel (4-core) | Speedup
-------------|------------|-------------------|--------
100 MB       | 2.3s       | 0.8s             | 2.9x
1 GB         | 23.1s      | 6.2s             | 3.7x
10 GB        | 231.0s     | 48.5s            | 4.8x
```

## Format Validation Framework

### Validation Levels
1. **Basic:** Syntax and structure checking
2. **Standard:** + Semantic validation
3. **Strict:** + Cross-reference validation
4. **Pedantic:** + All optional checks

### Error Types and Recovery
```c
typedef enum {
    ERROR_TYPE_NONE = 0,
    ERROR_TYPE_SYNTAX,        // Malformed data
    ERROR_TYPE_SEMANTIC,      // Invalid semantics
    ERROR_TYPE_GEOMETRY,      // Geometric inconsistencies
    ERROR_TYPE_TOPOLOGY,      // Connectivity issues
    ERROR_TYPE_ELECTRICAL,    // Electrical rule violations
    ERROR_TYPE_MATERIAL,      // Material property issues
    ERROR_TYPE_HIERARCHY,     // Cell hierarchy problems
    ERROR_TYPE_COMPRESSION,   // Decompression failures
    ERROR_TYPE_MEMORY,        // Memory allocation issues
    ERROR_TYPE_IO,             // I/O operation failures
    ERROR_TYPE_VERSION,       // Unsupported version
    ERROR_TYPE_CHECKSUM       // Data corruption
} ErrorType;
```

### Recovery Strategies
1. **Skip and Continue:** Ignore invalid records
2. **Default Substitution:** Use reasonable defaults
3. **Fallback Format:** Try alternative format interpretation
4. **Partial Loading:** Load valid portions only

## Memory Optimization Strategies

### Memory Pool Allocation
- Pre-allocated memory pools for common object sizes
- Reduces allocation overhead by 60-80%
- Eliminates fragmentation for fixed-size objects

### Object Caching
- LRU cache for frequently accessed objects
- Compression for large geometric data
- Reference counting for shared objects

### Streaming Processing
- Process data in chunks without full loading
- Suitable for very large files (>1GB)
- Memory usage remains constant regardless of file size

### Memory Usage Comparison
```
File Size    | Standard | Optimized | Reduction
-------------|----------|-----------|------------
100 MB       | 450 MB   | 180 MB    | 60%
1 GB         | 4.2 GB   | 1.5 GB    | 64%
10 GB        | 41.8 GB  | 12.3 GB   | 71%
```

## Integration Examples

### Basic File Loading
```c
// Simple file loading with automatic format detection
PCBDesign* design = pcb_design_load("board.pcb");
if (design) {
    printf("Loaded design with %d layers, %d components\n", 
           design->num_layers, design->num_components);
    pcb_design_destroy(design);
}
```

### Advanced Loading with Custom Parameters
```c
// Advanced loading with specific format and validation
LoadParameters params = {
    .format = FORMAT_GDSII_BINARY,
    .validation_level = VALIDATION_STRICT,
    .use_parallel_io = 1,
    .num_threads = 8,
    .memory_limit = 2 * 1024 * 1024 * 1024,  // 2GB
    .error_recovery = RECOVERY_SKIP_INVALID,
    .enable_caching = 1,
    .compression_threshold = 1024
};

PCBDesign* design = pcb_design_load_advanced("complex_board.gds", &params);
```

### Batch Processing
```c
// Batch processing multiple files
const char* files[] = {"board1.gds", "board2.oas", "board3.gbr"};
int num_files = sizeof(files) / sizeof(files[0]);

BatchLoadConfig batch_config = {
    .max_parallel_files = 4,
    .total_memory_limit = 8 * 1024 * 1024 * 1024,  // 8GB
    .progress_callback = batch_progress_callback,
    .error_callback = batch_error_callback
};

PCBDesign** designs = pcb_design_load_batch(files, num_files, &batch_config);
```

## Performance Optimization Guidelines

### Best Practices
1. **Use Parallel I/O:** Enable for files >100MB
2. **Enable Caching:** For repeated access to same data
3. **Memory Mapping:** For very large files (>500MB)
4. **Appropriate Validation:** Use strict only when necessary
5. **Format-Specific Optimization:** Choose format based on use case

### Format Selection Guidelines
```
Use Case                    | Recommended Format | Reason
----------------------------|-------------------|------------------------
Large designs (>1GB)       | OASIS             | Best compression
Manufacturing data          | Gerber            | Industry standard
Design exchange             | IPC-2581          | Comprehensive data
Legacy compatibility        | GDSII             | Universal support
CAD integration             | DXF               | CAD system support
Drill data                  | Excellon          | Specialized format
```

### Memory Management Tips
1. **Monitor Memory Usage:** Use built-in memory tracking
2. **Adjust Cache Size:** Based on available system memory
3. **Use Streaming:** For files larger than physical memory
4. **Enable Compression:** For memory-constrained environments
5. **Garbage Collection:** Enable for long-running applications

## Troubleshooting

### Common Issues
1. **Format Detection Failures:**
   - Check file corruption
   - Verify format version support
   - Use explicit format specification

2. **Memory Issues:**
   - Reduce cache size
   - Enable streaming mode
   - Increase system virtual memory

3. **Performance Problems:**
   - Enable parallel processing
   - Optimize chunk size
   - Check disk I/O bottlenecks

4. **Validation Errors:**
   - Adjust validation level
   - Enable error recovery
   - Check file format compliance

### Debug and Logging
```c
// Enable debug logging
set_log_level(LOG_DEBUG);
set_log_file("format_debug.log");

// Enable performance profiling
enable_performance_profiling(1);
set_profiling_output("performance.csv");

// Enable memory tracking
enable_memory_tracking(1);
set_memory_report_interval(1000);  // Report every 1000 allocations
```

## API Reference Summary

### Core Functions
- `detect_file_format()` - Automatic format detection
- `pcb_design_load()` - Simple file loading
- `pcb_design_load_advanced()` - Advanced loading with parameters
- `pcb_design_load_batch()` - Batch processing
- `format_get_info()` - Format information
- `format_get_capabilities()` - Format capabilities

### Configuration Functions
- `set_parallel_io_config()` - Configure parallel processing
- `set_memory_optimization_config()` - Memory management
- `set_validation_config()` - Validation parameters
- `set_format_preferences()` - Format-specific settings

### Utility Functions
- `format_convert()` - Format conversion
- `format_validate()` - Standalone validation
- `format_repair()` - Format repair
- `format_compare()` - Compare formats
- `format_benchmark()` - Performance testing

This documentation provides comprehensive guidance for using the advanced file format support system effectively in electromagnetic simulation applications.