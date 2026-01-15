/********************************************************************************
 *  PulseEM - Unified Mesh Platform Makefile
 *
 *  Copyright (C) 2025 PulseEM Technologies
 *
 *  Build configuration for unified mesh generation platform
 ********************************************************************************/

# Compiler settings
CC = gcc
CXX = g++
CFLAGS = -Wall -Wextra -O3 -fPIC -std=c11
CXXFLAGS = -Wall -Wextra -O3 -fPIC -std=c++11

# OpenMP support
OPENMP_FLAG = -fopenmp

# Platform-specific settings
ifeq ($(OS),Windows_NT)
    PLATFORM = windows
    LIB_EXT = .dll
    OBJ_EXT = .obj
    RM = del /Q
else
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Linux)
        PLATFORM = linux
        LIB_EXT = .so
        OBJ_EXT = .o
        RM = rm -f
    endif
    ifeq ($(UNAME_S),Darwin)
        PLATFORM = macos
        LIB_EXT = .dylib
        OBJ_EXT = .o
        RM = rm -f
    endif
endif

# Directory structure
SRC_DIR = .
OBJ_DIR = ../../build/mesh
LIB_DIR = ../../lib
INC_DIR = ../../include

# Source files
SOURCES = \
    mesh_engine.c \
    mesh_algorithms.c \
    mesh_subsystem.c

# Object files  
OBJECTS = $(patsubst %.c,$(OBJ_DIR)/%.$(OBJ_EXT),$(notdir $(SOURCES)))

# Target library name
TARGET = $(LIB_DIR)/libpulsemesh$(LIB_EXT)

# Include directories
INCLUDES = -I. -I../core -I../utils -I../cad

# Library dependencies
LIBS = -lm -lpthread

# OpenMP support (optional)
ifdef USE_OPENMP
    CFLAGS += $(OPENMP_FLAG)
    LIBS += -lgomp
endif

# GPU support (optional)
ifdef USE_GPU
    CFLAGS += -DUSE_GPU
    LIBS += -lcuda -lcudart
endif

# Debug build
ifdef DEBUG
    CFLAGS += -g -DDEBUG
    CXXFLAGS += -g -DDEBUG
else
    CFLAGS += -DNDEBUG
    CXXFLAGS += -DNDEBUG
endif

# Default target
all: directories $(TARGET)

# Create directories
directories:
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(LIB_DIR)
	@mkdir -p $(INC_DIR)

# Build target library
$(TARGET): $(OBJECTS)
	@echo "Building unified mesh library..."
	$(CC) -shared $(CFLAGS) -o $@ $^ $(LIBS)
	@echo "Mesh library built successfully: $@"

# Compile source files
$(OBJ_DIR)/%.$(OBJ_EXT): $(SRC_DIR)/%.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Install headers
install-headers:
	@cp *.h $(INC_DIR)/
	@echo "Mesh headers installed to $(INC_DIR)"

# Install library
install-lib: $(TARGET)
	@echo "Mesh library installed to $(LIB_DIR)"

# Install everything
install: install-headers install-lib
	@echo "Unified mesh platform installed successfully"

# Clean build files
clean:
	$(RM) $(OBJ_DIR)/*
	$(RM) $(TARGET)
	@echo "Mesh platform cleaned"

# Deep clean (including dependencies)
distclean: clean
	$(RM) $(LIB_DIR)/libpulsemesh*
	@echo "Mesh platform deep cleaned"

# Build with all optimizations
release: clean
	$(MAKE) all USE_OPENMP=1

# Build with debug symbols
debug: clean
	$(MAKE) all DEBUG=1

# Build with GPU support
gpu: clean
	$(MAKE) all USE_GPU=1 USE_OPENMP=1

# Run tests
test: $(TARGET)
	@echo "Running mesh platform tests..."
	cd ../tests && $(MAKE) test

# Generate documentation
docs:
	@echo "Generating mesh platform documentation..."
	doxygen mesh_doxygen.conf

# Show build configuration
info:
	@echo "Platform: $(PLATFORM)"
	@echo "Compiler: $(CC)"
	@echo "OpenMP: $(if $(USE_OPENMP),Enabled,Disabled)"
	@echo "GPU: $(if $(USE_GPU),Enabled,Disabled)"
	@echo "Debug: $(if $(DEBUG),Enabled,Disabled)"
	@echo "Target: $(TARGET)"

.PHONY: all directories clean distclean install install-headers install-lib release debug gpu test docs info