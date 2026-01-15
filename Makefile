# Comprehensive PEEC-MoM Unified Framework Makefile
# Supports Windows, Linux, and macOS builds with multiple compilers

# Build configuration
BUILD_TYPE ?= RELEASE
PLATFORM ?= $(shell uname -s)
ARCH ?= $(shell uname -m)

# Compiler selection
CC ?= gcc
CXX ?= g++
FC ?= gfortran

# Base directories
SRC_DIR = src
INC_DIR = include
OBJ_DIR = obj
BIN_DIR = bin
LIB_DIR = lib
PLUGIN_DIR = plugins
EXAMPLES_DIR = examples
DOCS_DIR = docs

# Source files
CORE_SRCS = $(SRC_DIR)/core/electromagnetic_kernel_library.c
CAD_SRCS = $(SRC_DIR)/cad/cad_mesh_generation.c
PLUGIN_SRCS = $(SRC_DIR)/plugins/plugin_framework.c
PERF_SRCS = $(SRC_DIR)/performance/performance_monitor.c

# Plugin source files
MOM_PLUGIN_SRCS = $(SRC_DIR)/plugins/mom/mom_solver_plugin.cpp
PEEC_PLUGIN_SRCS = $(SRC_DIR)/plugins/peec/peec_solver_plugin.cpp
HYBRID_PLUGIN_SRCS = $(SRC_DIR)/plugins/hybrid/hybrid_coupling_plugin.cpp

# Example source files
EXAMPLE_SRCS = $(EXAMPLES_DIR)/comprehensive_integration_demo.c
PERF_EXAMPLE_SRCS = $(EXAMPLES_DIR)/performance_benchmark_suite.c
PLUGIN_EXAMPLE_SRCS = $(EXAMPLES_DIR)/plugin_architecture_integration.c

# Header files
HEADERS = $(wildcard $(SRC_DIR)/**/*.h) $(wildcard $(SRC_DIR)/**/*.hpp)

# Object files
CORE_OBJS = $(OBJ_DIR)/core/electromagnetic_kernel_library.o
CAD_OBJS = $(OBJ_DIR)/cad/cad_mesh_generation.o
PLUGIN_OBJS = $(OBJ_DIR)/plugins/plugin_framework.o
PERF_OBJS = $(OBJ_DIR)/performance/performance_monitor.o

# Plugin object files
MOM_PLUGIN_OBJS = $(OBJ_DIR)/plugins/mom/mom_solver_plugin.o
PEEC_PLUGIN_OBJS = $(OBJ_DIR)/plugins/peec/peec_solver_plugin.o
HYBRID_PLUGIN_OBJS = $(OBJ_DIR)/plugins/hybrid/hybrid_coupling_plugin.o

# Example object files
EXAMPLE_OBJS = $(OBJ_DIR)/examples/comprehensive_integration_demo.o
PERF_EXAMPLE_OBJS = $(OBJ_DIR)/examples/performance_benchmark_suite.o
PLUGIN_EXAMPLE_OBJS = $(OBJ_DIR)/examples/plugin_architecture_integration.o

# Libraries
CORE_LIB = $(LIB_DIR)/libelectromagnetic_kernel_library.a
CAD_LIB = $(LIB_DIR)/libcad_mesh.a
PLUGIN_LIB = $(LIB_DIR)/libplugin_framework.a
PERF_LIB = $(LIB_DIR)/libperformance_monitor.a

# Plugin libraries
MOM_PLUGIN_LIB = $(PLUGIN_DIR)/mom_solver_plugin.dll
PEEC_PLUGIN_LIB = $(PLUGIN_DIR)/peec_solver_plugin.dll
HYBRID_PLUGIN_LIB = $(PLUGIN_DIR)/hybrid_coupling_plugin.dll

# Executables
DEMO_EXE = $(BIN_DIR)/comprehensive_integration_demo
PERF_EXE = $(BIN_DIR)/performance_benchmark_suite
PLUGIN_EXE = $(BIN_DIR)/plugin_architecture_integration
PULSEEM_EXE = $(BIN_DIR)/pulseem$(EXE_EXT)

# Platform-specific settings
ifeq ($(PLATFORM),Windows)
    SHARED_EXT = dll
    STATIC_EXT = lib
    EXE_EXT = .exe
    PLATFORM_FLAGS = -DWIN32 -D_WINDOWS
    PLUGIN_FLAGS = -DPLUGIN_EXPORTS
    OPENMP_FLAG = -fopenmp
else ifeq ($(PLATFORM),Linux)
    SHARED_EXT = so
    STATIC_EXT = a
    EXE_EXT =
    PLATFORM_FLAGS = -DUNIX -DLINUX
    PLUGIN_FLAGS = -fPIC -DPLUGIN_EXPORTS
    OPENMP_FLAG = -fopenmp
else ifeq ($(PLATFORM),Darwin)
    SHARED_EXT = dylib
    STATIC_EXT = a
    EXE_EXT =
    PLATFORM_FLAGS = -DUNIX -DMACOS
    PLUGIN_FLAGS = -fPIC -DPLUGIN_EXPORTS
    OPENMP_FLAG = -Xclang -fopenmp
endif

# Build type flags
ifeq ($(BUILD_TYPE),DEBUG)
    OPT_FLAGS = -O0 -g -DDEBUG
else ifeq ($(BUILD_TYPE),RELEASE)
    OPT_FLAGS = -O3 -DNDEBUG
else ifeq ($(BUILD_TYPE),PROFILE)
    OPT_FLAGS = -O2 -g -pg -DPROFILE
endif

# Feature flags
FEATURE_FLAGS = -DENABLE_OPENMP -DENABLE_MPI -DENABLE_GPU -DENABLE_COMPRESSION

# Warning flags
WARN_FLAGS = -Wall -Wextra -Wpedantic -Wno-unused-parameter

# Include paths
INCLUDES = -I$(INC_DIR) -I$(SRC_DIR)/core -I$(SRC_DIR)/cad -I$(SRC_DIR)/plugins -I$(SRC_DIR)/performance

# Library paths
LIB_PATHS = -L$(LIB_DIR) -L$(PLUGIN_DIR)

# Libraries to link
LIBS = -lm -lpthread

# OpenMP support
ifdef ENABLE_OPENMP
    LIBS += -lgomp
    FEATURE_FLAGS += -DENABLE_OPENMP
endif

# MPI support
ifdef ENABLE_MPI
    LIBS += -lmpi
    FEATURE_FLAGS += -DENABLE_MPI
    INCLUDES += -I$(MPI_HOME)/include
    LIB_PATHS += -L$(MPI_HOME)/lib
endif

# GPU support
ifdef ENABLE_GPU
    FEATURE_FLAGS += -DENABLE_GPU
    NVCC = nvcc
    GPU_FLAGS = -arch=sm_70
endif

# Compression support
ifdef ENABLE_COMPRESSION
    LIBS += -lz
    FEATURE_FLAGS += -DENABLE_COMPRESSION
endif

# Combined flags
CFLAGS = $(PLATFORM_FLAGS) $(OPT_FLAGS) $(FEATURE_FLAGS) $(WARN_FLAGS) $(INCLUDES)
CXXFLAGS = $(CFLAGS) -std=c++11
LDFLAGS = $(LIB_PATHS) $(LIBS)

# Default target
all: directories $(CORE_LIB) $(CAD_LIB) $(PLUGIN_LIB) $(PERF_LIB) plugins examples pulseem

# Create directories
directories:
	@mkdir -p $(OBJ_DIR)/core
	@mkdir -p $(OBJ_DIR)/cad
	@mkdir -p $(OBJ_DIR)/plugins/mom
	@mkdir -p $(OBJ_DIR)/plugins/peec
	@mkdir -p $(OBJ_DIR)/plugins/hybrid
	@mkdir -p $(OBJ_DIR)/performance
	@mkdir -p $(OBJ_DIR)/examples
	@mkdir -p $(BIN_DIR)
	@mkdir -p $(LIB_DIR)
	@mkdir -p $(PLUGIN_DIR)

# Core library
$(CORE_LIB): $(CORE_OBJS)
	@echo "Building core library..."
	ar rcs $@ $^

$(OBJ_DIR)/core/electromagnetic_kernel_library.o: $(SRC_DIR)/core/electromagnetic_kernel_library.c $(SRC_DIR)/core/electromagnetic_kernel_library.h
	@echo "Compiling core module..."
	$(CC) $(CFLAGS) -c $< -o $@

# CAD mesh library
$(CAD_LIB): $(CAD_OBJS)
	@echo "Building CAD mesh library..."
	ar rcs $@ $^

$(OBJ_DIR)/cad/cad_mesh_generation.o: $(SRC_DIR)/cad/cad_mesh_generation.c $(SRC_DIR)/cad/cad_mesh_generation.h
	@echo "Compiling CAD mesh generation..."
	$(CC) $(CFLAGS) $(OPENMP_FLAG) -c $< -o $@

# Plugin framework library
$(PLUGIN_LIB): $(PLUGIN_OBJS)
	@echo "Building plugin framework library..."
	ar rcs $@ $^

$(OBJ_DIR)/plugins/plugin_framework.o: $(SRC_DIR)/plugins/plugin_framework.c $(SRC_DIR)/plugins/plugin_framework.h
	@echo "Compiling plugin framework..."
	$(CC) $(CFLAGS) -c $< -o $@

# Performance monitoring library
$(PERF_LIB): $(PERF_OBJS)
	@echo "Building performance monitoring library..."
	ar rcs $@ $^

$(OBJ_DIR)/performance/performance_monitor.o: $(SRC_DIR)/performance/performance_monitor.c $(SRC_DIR)/performance/performance_monitor.h
	@echo "Compiling performance monitor..."
	$(CC) $(CFLAGS) -c $< -o $@

# Plugin libraries
plugins: $(MOM_PLUGIN_LIB) $(PEEC_PLUGIN_LIB) $(HYBRID_PLUGIN_LIB)

$(MOM_PLUGIN_LIB): $(MOM_PLUGIN_OBJS)
	@echo "Building MoM solver plugin..."
ifeq ($(PLATFORM),Windows)
	$(CXX) -shared $(CXXFLAGS) $(PLUGIN_FLAGS) -o $@ $^ $(LDFLAGS)
else
	$(CXX) -shared $(CXXFLAGS) $(PLUGIN_FLAGS) -o $@ $^ $(LDFLAGS)
endif

$(OBJ_DIR)/plugins/mom/mom_solver_plugin.o: $(SRC_DIR)/plugins/mom/mom_solver_plugin.cpp $(SRC_DIR)/plugins/mom/mom_solver_module.h
	@echo "Compiling MoM solver plugin..."
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(PEEC_PLUGIN_LIB): $(PEEC_PLUGIN_OBJS)
	@echo "Building PEEC solver plugin..."
ifeq ($(PLATFORM),Windows)
	$(CXX) -shared $(CXXFLAGS) $(PLUGIN_FLAGS) -o $@ $^ $(LDFLAGS)
else
	$(CXX) -shared $(CXXFLAGS) $(PLUGIN_FLAGS) -o $@ $^ $(LDFLAGS)
endif

$(OBJ_DIR)/plugins/peec/peec_solver_plugin.o: $(SRC_DIR)/plugins/peec/peec_solver_plugin.cpp $(SRC_DIR)/plugins/peec/peec_solver_module.h
	@echo "Compiling PEEC solver plugin..."
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(HYBRID_PLUGIN_LIB): $(HYBRID_PLUGIN_OBJS)
	@echo "Building hybrid coupling plugin..."
ifeq ($(PLATFORM),Windows)
	$(CXX) -shared $(CXXFLAGS) $(PLUGIN_FLAGS) -o $@ $^ $(LDFLAGS)
else
	$(CXX) -shared $(CXXFLAGS) $(PLUGIN_FLAGS) -o $@ $^ $(LDFLAGS)
endif

$(OBJ_DIR)/plugins/hybrid/hybrid_coupling_plugin.o: $(SRC_DIR)/plugins/hybrid/hybrid_coupling_plugin.cpp $(SRC_DIR)/hybrid/hybrid_coupling_interface.h
	@echo "Compiling hybrid coupling plugin..."
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Example executables
examples: $(DEMO_EXE) $(PERF_EXE) $(PLUGIN_EXE)

# Unified PulseEM executable
pulseem: $(PULSEEM_EXE)

$(PULSEEM_EXE): $(OBJ_DIR)/apps/pulseem_cli.o $(CORE_LIB) $(PLUGIN_LIB)
	@echo "Building unified PulseEM executable..."
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJ_DIR)/apps/pulseem_cli.o: apps/pulseem_cli.c
	@echo "Compiling PulseEM CLI..."
	@mkdir -p $(OBJ_DIR)/apps
	$(CC) $(CFLAGS) -c $< -o $@

$(DEMO_EXE): $(EXAMPLE_OBJS) $(CORE_LIB) $(CAD_LIB) $(PLUGIN_LIB) $(PERF_LIB)
	@echo "Building comprehensive integration demo..."
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJ_DIR)/examples/comprehensive_integration_demo.o: $(EXAMPLES_DIR)/comprehensive_integration_demo.c
	@echo "Compiling comprehensive integration demo..."
	$(CC) $(CFLAGS) -c $< -o $@

$(PERF_EXE): $(PERF_EXAMPLE_OBJS) $(CORE_LIB) $(PLUGIN_LIB) $(PERF_LIB)
	@echo "Building performance benchmark suite..."
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJ_DIR)/examples/performance_benchmark_suite.o: $(EXAMPLES_DIR)/performance_benchmark_suite.c
	@echo "Compiling performance benchmark suite..."
	$(CC) $(CFLAGS) -c $< -o $@

$(PLUGIN_EXE): $(PLUGIN_EXAMPLE_OBJS) $(CORE_LIB) $(PLUGIN_LIB)
	@echo "Building plugin architecture integration demo..."
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJ_DIR)/examples/plugin_architecture_integration.o: $(EXAMPLES_DIR)/plugin_architecture_integration.c
	@echo "Compiling plugin architecture integration demo..."
	$(CC) $(CFLAGS) -c $< -o $@

# GPU-accelerated components
ifdef ENABLE_GPU
GPU_OBJS = $(OBJ_DIR)/gpu/gpu_acceleration.o $(OBJ_DIR)/gpu/cuda_kernels.o

$(OBJ_DIR)/gpu/%.o: $(SRC_DIR)/gpu/%.cu
	@echo "Compiling GPU kernel: $<"
	$(NVCC) $(GPU_FLAGS) -c $< -o $@
endif

# Installation targets
install: all
	@echo "Installing libraries and executables..."
	cp $(LIB_DIR)/* $(PREFIX)/lib/
	cp $(BIN_DIR)/* $(PREFIX)/bin/
	cp $(PLUGIN_DIR)/* $(PREFIX)/lib/plugins/
	cp $(SRC_DIR)/*.h $(PREFIX)/include/
	@echo "Installation complete!"
	@echo "Executables installed:"
	@echo "  - pulseem (unified electromagnetic simulator)"
	@echo "  - comprehensive_integration_demo"
	@echo "  - performance_benchmark_suite"
	@echo "  - plugin_architecture_integration"

# Uninstall target
uninstall:
	@echo "Uninstalling..."
	rm -f $(PREFIX)/lib/libpeec_mom_*.a
	rm -f $(PREFIX)/lib/libcad_mesh.a
	rm -f $(PREFIX)/lib/libplugin_framework.a
	rm -f $(PREFIX)/lib/libperformance_monitor.a
	rm -f $(PREFIX)/bin/pulseem
	rm -f $(PREFIX)/bin/comprehensive_integration_demo
	rm -f $(PREFIX)/bin/performance_benchmark_suite
	rm -f $(PREFIX)/bin/plugin_architecture_integration
	rm -rf $(PREFIX)/lib/plugins/

# Testing targets
test: all
	@echo "Running comprehensive tests..."
	$(BIN_DIR)/comprehensive_integration_demo
	$(BIN_DIR)/performance_benchmark_suite
	$(BIN_DIR)/plugin_architecture_integration

# Benchmark targets
benchmark: all
	@echo "Running performance benchmarks..."
	$(BIN_DIR)/performance_benchmark_suite --benchmark-all

# Documentation targets
docs:
	@echo "Generating documentation..."
	doxygen Doxyfile

# Clean targets
clean:
	@echo "Cleaning build artifacts..."
	rm -rf $(OBJ_DIR)
	rm -rf $(BIN_DIR)
	rm -rf $(LIB_DIR)
	rm -rf $(PLUGIN_DIR)

distclean: clean
	@echo "Cleaning all generated files..."
	rm -rf html/
	rm -rf latex/

# Help target
help:
	@echo "PEEC-MoM Unified Framework Build System"
	@echo ""
	@echo "Usage: make [target] [options]"
	@echo ""
	@echo "Targets:"
	@echo "  all          - Build all libraries and executables (default)"
	@echo "  pulseem      - Build unified PulseEM executable"
	@echo "  plugins      - Build solver plugins"
	@echo "  examples     - Build example programs"
	@echo "  test         - Run comprehensive tests"
	@echo "  benchmark    - Run performance benchmarks"
	@echo "  install      - Install libraries and executables"
	@echo "  uninstall    - Uninstall libraries and executables"
	@echo "  clean        - Remove build artifacts"
	@echo "  distclean    - Remove all generated files"
	@echo "  docs         - Generate documentation"
	@echo "  help         - Show this help message"
	@echo ""
	@echo "Options:"
	@echo "  BUILD_TYPE=DEBUG|RELEASE|PROFILE"
	@echo "  PLATFORM=Windows|Linux|Darwin"
	@echo "  ENABLE_OPENMP=1"
	@echo "  ENABLE_MPI=1"
	@echo "  ENABLE_GPU=1"
	@echo "  ENABLE_COMPRESSION=1"
	@echo "  PREFIX=/path/to/install"
	@echo ""
	@echo "Examples:"
	@echo "  make all BUILD_TYPE=RELEASE ENABLE_OPENMP=1"
	@echo "  make test ENABLE_GPU=1"
	@echo "  make install PREFIX=/usr/local"

# Dependency generation
depend:
	@echo "Generating dependencies..."
	$(CC) $(CFLAGS) -MM $(SRC_DIR)/*/*.c > .depend
	$(CXX) $(CXXFLAGS) -MM $(SRC_DIR)/*/*.cpp >> .depend

# Include dependencies
-include .depend

# Phony targets
.PHONY: all directories plugins examples test benchmark install uninstall clean distclean docs help depend

# Automatic dependency generation
%.d: %.c
	$(CC) $(CFLAGS) -MM -MT $(OBJ_DIR)/$*.o $< > $@

%.d: %.cpp
	$(CXX) $(CXXFLAGS) -MM -MT $(OBJ_DIR)/$*.o $< > $@