/*********************************************************************
 * Plugin Framework Implementation - Commercial-Grade PEEC-MoM Architecture
 * 
 * This module implements the core plugin framework functionality with
 * dynamic loading, validation, and management capabilities.
 * 
 * Author: PulseMoM Development Team
 * Copyright (c) 2024
 *********************************************************************/

#include "plugin_framework.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#define PLUGIN_EXTENSION ".dll"
#else
#define PLUGIN_EXTENSION ".so"
#endif

#define MAX_PLUGINS 100
#define PLUGIN_VALIDATION_TIMEOUT 30  // seconds
#define PLUGIN_DIRECTORY_DEFAULT "./plugins"

typedef PluginInterface* (*create_plugin_func)();
typedef void (*destroy_plugin_func)(PluginInterface*);

typedef struct {
    void* handle;
    create_plugin_func create;
    destroy_plugin_func destroy;
    time_t load_time;
    int load_count;
    char path[1024];
} PluginLibrary;

// Global plugin manager instance
static PluginManager* g_plugin_manager = NULL;
static int g_last_error = 0;

// Error codes
#define PLUGIN_ERROR_NONE 0
#define PLUGIN_ERROR_MEMORY -1
#define PLUGIN_ERROR_LOAD_FAILED -2
#define PLUGIN_ERROR_INVALID_PLUGIN -3
#define PLUGIN_ERROR_INCOMPATIBLE -4
#define PLUGIN_ERROR_ALREADY_LOADED -5
#define PLUGIN_ERROR_NOT_FOUND -6
#define PLUGIN_ERROR_VALIDATION_FAILED -7
#define PLUGIN_ERROR_TIMEOUT -8

// Forward declarations for internal functions
static int plugin_validate_interface(const PluginInterface* plugin);
static int plugin_check_dependencies(const PluginInterface* plugin);
static void plugin_update_capabilities(PluginInterface* plugin);
static int plugin_load_library(PluginManager* manager, const char* plugin_path, PluginLibrary* lib);
static int plugin_unload_library(PluginLibrary* lib);

// Plugin manager implementation
PluginManager* plugin_manager_create(Framework* framework) {
    PluginManager* manager = (PluginManager*)calloc(1, sizeof(PluginManager));
    if (!manager) {
        g_last_error = PLUGIN_ERROR_MEMORY;
        return NULL;
    }
    
    manager->plugins = (PluginInterface**)calloc(MAX_PLUGINS, sizeof(PluginInterface*));
    if (!manager->plugins) {
        free(manager);
        g_last_error = PLUGIN_ERROR_MEMORY;
        return NULL;
    }
    
    manager->num_plugins = 0;
    manager->max_plugins = MAX_PLUGINS;
    manager->framework = framework;
    strcpy(manager->plugin_directory, PLUGIN_DIRECTORY_DEFAULT);
    manager->auto_load_plugins = 1;
    
    // Set up function pointers
    manager->load_plugin = plugin_manager_load_plugin;
    manager->unload_plugin = plugin_manager_unload_plugin;
    manager->get_plugin = plugin_manager_get_plugin;
    manager->register_plugin = plugin_manager_register_plugin;
    manager->unregister_plugin = plugin_manager_unregister_plugin;
    manager->list_plugins = plugin_manager_list_plugins;
    manager->validate_plugin = plugin_manager_validate_plugin;
    
    g_plugin_manager = manager;
    g_last_error = PLUGIN_ERROR_NONE;
    
    return manager;
}

void plugin_manager_destroy(PluginManager* manager) {
    if (!manager) return;
    
    // Unload all plugins
    for (int i = 0; i < manager->num_plugins; i++) {
        if (manager->plugins[i]) {
            plugin_manager_unload_plugin(manager, manager->plugins[i]->info.name);
        }
    }
    
    free(manager->plugins);
    free(manager);
    
    if (g_plugin_manager == manager) {
        g_plugin_manager = NULL;
    }
}

int plugin_manager_load_plugin(PluginManager* manager, const char* plugin_path) {
    if (!manager || !plugin_path) {
        g_last_error = PLUGIN_ERROR_INVALID_PLUGIN;
        return -1;
    }
    
    // Check if plugin is already loaded
    for (int i = 0; i < manager->num_plugins; i++) {
        if (manager->plugins[i] && strcmp(manager->plugins[i]->path, plugin_path) == 0) {
            g_last_error = PLUGIN_ERROR_ALREADY_LOADED;
            return -1;
        }
    }
    
    // Check if we have space for new plugin
    if (manager->num_plugins >= manager->max_plugins) {
        g_last_error = PLUGIN_ERROR_MEMORY;
        return -1;
    }
    
    // Validate plugin before loading
    if (plugin_manager_validate_plugin(manager, plugin_path) != 0) {
        return -1;
    }
    
    // Load plugin library
    PluginLibrary lib;
    if (plugin_load_library(manager, plugin_path, &lib) != 0) {
        return -1;
    }
    
    // Create plugin interface
    PluginInterface* plugin = lib.create();
    if (!plugin) {
        plugin_unload_library(&lib);
        g_last_error = PLUGIN_ERROR_INVALID_PLUGIN;
        return -1;
    }
    
    // Validate plugin interface
    if (plugin_validate_interface(plugin) != 0) {
        lib.destroy(plugin);
        plugin_unload_library(&lib);
        g_last_error = PLUGIN_ERROR_INVALID_PLUGIN;
        return -1;
    }
    
    // Check API compatibility
    if (plugin->info.api_version != PLUGIN_API_VERSION) {
        lib.destroy(plugin);
        plugin_unload_library(&lib);
        g_last_error = PLUGIN_ERROR_INCOMPATIBLE;
        return -1;
    }
    
    // Initialize plugin
    if (plugin->initialize && plugin->initialize(plugin, manager->framework) != 0) {
        lib.destroy(plugin);
        plugin_unload_library(&lib);
        g_last_error = PLUGIN_ERROR_LOAD_FAILED;
        return -1;
    }
    
    // Store plugin library handle in plugin
    plugin->handle = lib.handle;
    strcpy(plugin->path, plugin_path);
    plugin->status = PLUGIN_STATUS_INITIALIZED;
    
    // Add to manager
    manager->plugins[manager->num_plugins++] = plugin;
    
    g_last_error = PLUGIN_ERROR_NONE;
    return 0;
}

int plugin_manager_unload_plugin(PluginManager* manager, const char* plugin_name) {
    if (!manager || !plugin_name) {
        g_last_error = PLUGIN_ERROR_INVALID_PLUGIN;
        return -1;
    }
    
    PluginInterface* plugin = plugin_manager_get_plugin(manager, plugin_name);
    if (!plugin) {
        g_last_error = PLUGIN_ERROR_NOT_FOUND;
        return -1;
    }
    
    // Find plugin index
    int plugin_index = -1;
    for (int i = 0; i < manager->num_plugins; i++) {
        if (manager->plugins[i] == plugin) {
            plugin_index = i;
            break;
        }
    }
    
    if (plugin_index == -1) {
        g_last_error = PLUGIN_ERROR_NOT_FOUND;
        return -1;
    }
    
    // Cleanup and shutdown plugin
    if (plugin->cleanup) {
        plugin->cleanup(plugin);
    }
    if (plugin->shutdown) {
        plugin->shutdown(plugin);
    }
    
    // Get library handle before destroying plugin
    void* handle = plugin->handle;
    
    // Remove from manager
    for (int i = plugin_index; i < manager->num_plugins - 1; i++) {
        manager->plugins[i] = manager->plugins[i + 1];
    }
    manager->num_plugins--;
    
    // Unload library
    if (handle) {
        dlclose(handle);
    }
    
    g_last_error = PLUGIN_ERROR_NONE;
    return 0;
}

PluginInterface* plugin_manager_get_plugin(PluginManager* manager, const char* plugin_name) {
    if (!manager || !plugin_name) {
        return NULL;
    }
    
    for (int i = 0; i < manager->num_plugins; i++) {
        if (manager->plugins[i] && strcmp(manager->plugins[i]->info.name, plugin_name) == 0) {
            return manager->plugins[i];
        }
    }
    
    return NULL;
}

int plugin_manager_register_plugin(PluginManager* manager, PluginInterface* plugin) {
    if (!manager || !plugin) {
        g_last_error = PLUGIN_ERROR_INVALID_PLUGIN;
        return -1;
    }
    
    if (manager->num_plugins >= manager->max_plugins) {
        g_last_error = PLUGIN_ERROR_MEMORY;
        return -1;
    }
    
    // Validate plugin
    if (plugin_validate_interface(plugin) != 0) {
        g_last_error = PLUGIN_ERROR_INVALID_PLUGIN;
        return -1;
    }
    
    manager->plugins[manager->num_plugins++] = plugin;
    g_last_error = PLUGIN_ERROR_NONE;
    return 0;
}

int plugin_manager_unregister_plugin(PluginManager* manager, const char* plugin_name) {
    if (!manager || !plugin_name) {
        g_last_error = PLUGIN_ERROR_INVALID_PLUGIN;
        return -1;
    }
    
    PluginInterface* plugin = plugin_manager_get_plugin(manager, plugin_name);
    if (!plugin) {
        g_last_error = PLUGIN_ERROR_NOT_FOUND;
        return -1;
    }
    
    // Find and remove plugin
    for (int i = 0; i < manager->num_plugins; i++) {
        if (manager->plugins[i] == plugin) {
            for (int j = i; j < manager->num_plugins - 1; j++) {
                manager->plugins[j] = manager->plugins[j + 1];
            }
            manager->num_plugins--;
            break;
        }
    }
    
    g_last_error = PLUGIN_ERROR_NONE;
    return 0;
}

void plugin_manager_list_plugins(PluginManager* manager) {
    if (!manager) return;
    
    printf("Loaded Plugins (%d):\n", manager->num_plugins);
    printf("========================================\n");
    
    for (int i = 0; i < manager->num_plugins; i++) {
        PluginInterface* plugin = manager->plugins[i];
        if (plugin) {
            printf("Plugin %d:\n", i + 1);
            printf("  Name:        %s\n", plugin->info.name);
            printf("  Version:     %s\n", plugin->info.version);
            printf("  Type:        %s\n", plugin_get_type_string(plugin->info.type));
            printf("  Status:      %s\n", plugin_get_status_string(plugin->status));
            printf("  API Version: 0x%06x\n", plugin->info.api_version);
            printf("  Path:        %s\n", plugin->path);
            
            if (plugin->info.type == PLUGIN_TYPE_SOLVER && plugin->get_capabilities) {
                SolverCapabilities* caps = plugin->get_capabilities(plugin);
                if (caps) {
                    printf("  Capabilities:\n");
                    printf("    Max Frequency Points: %d\n", caps->max_frequency_points);
                    printf("    Max Mesh Elements: %d\n", caps->max_mesh_elements);
                    printf("    Max Unknowns: %d\n", caps->max_unknowns);
                    printf("    Memory Requirement: %.1f GB\n", caps->memory_requirement_gb);
                }
            }
            printf("\n");
        }
    }
}

int plugin_manager_validate_plugin(PluginManager* manager, const char* plugin_path) {
    if (!manager || !plugin_path) {
        g_last_error = PLUGIN_ERROR_INVALID_PLUGIN;
        return -1;
    }
    
    // Check if file exists and is readable
    struct stat st;
    if (stat(plugin_path, &st) != 0) {
        g_last_error = PLUGIN_ERROR_NOT_FOUND;
        return -1;
    }
    
    // Check if file is a regular file
    if (!S_ISREG(st.st_mode)) {
        g_last_error = PLUGIN_ERROR_INVALID_PLUGIN;
        return -1;
    }
    
    // Check file extension
    const char* ext = strrchr(plugin_path, '.');
    if (!ext || strcmp(ext, PLUGIN_EXTENSION) != 0) {
        g_last_error = PLUGIN_ERROR_INVALID_PLUGIN;
        return -1;
    }
    
    // Try to load library temporarily for validation
    void* handle = dlopen(plugin_path, RTLD_NOW | RTLD_LOCAL);
    if (!handle) {
        g_last_error = PLUGIN_ERROR_LOAD_FAILED;
        return -1;
    }
    
    // Check for required symbols
    create_plugin_func create_func = (create_plugin_func)dlsym(handle, "create_plugin_interface");
    destroy_plugin_func destroy_func = (destroy_plugin_func)dlsym(handle, "destroy_plugin_interface");
    
    if (!create_func || !destroy_func) {
        dlclose(handle);
        g_last_error = PLUGIN_ERROR_INVALID_PLUGIN;
        return -1;
    }
    
    dlclose(handle);
    g_last_error = PLUGIN_ERROR_NONE;
    return 0;
}

int plugin_manager_load_directory(PluginManager* manager, const char* directory) {
    if (!manager || !directory) {
        g_last_error = PLUGIN_ERROR_INVALID_PLUGIN;
        return -1;
    }
    
    DIR* dir = opendir(directory);
    if (!dir) {
        g_last_error = PLUGIN_ERROR_NOT_FOUND;
        return -1;
    }
    
    struct dirent* entry;
    int loaded_count = 0;
    
    while ((entry = readdir(dir)) != NULL) {
        // Check if file has correct extension
        const char* ext = strrchr(entry->d_name, '.');
        if (ext && strcmp(ext, PLUGIN_EXTENSION) == 0) {
            char full_path[1024];
            snprintf(full_path, sizeof(full_path), "%s/%s", directory, entry->d_name);
            
            if (plugin_manager_load_plugin(manager, full_path) == 0) {
                loaded_count++;
            }
        }
    }
    
    closedir(dir);
    g_last_error = PLUGIN_ERROR_NONE;
    return loaded_count;
}

// Internal utility functions
static int plugin_validate_interface(const PluginInterface* plugin) {
    if (!plugin) return -1;
    
    // Check required function pointers
    if (!plugin->initialize || !plugin->configure || !plugin->run || 
        !plugin->cleanup || !plugin->shutdown) {
        return -1;
    }
    
    // Check plugin info
    if (strlen(plugin->info.name) == 0 || strlen(plugin->info.version) == 0) {
        return -1;
    }
    
    return 0;
}

static int plugin_check_dependencies(const PluginInterface* plugin) {
    // Check plugin dependencies
    // This would check for required system libraries, etc.
    return 0;
}

static void plugin_update_capabilities(PluginInterface* plugin) {
    if (plugin->get_capabilities) {
        SolverCapabilities* caps = plugin->get_capabilities(plugin);
        if (caps) {
            plugin->info.solver_caps = caps;
        }
    }
}

static int plugin_load_library(PluginManager* manager, const char* plugin_path, PluginLibrary* lib) {
    memset(lib, 0, sizeof(PluginLibrary));
    
    // Load dynamic library
    lib->handle = dlopen(plugin_path, RTLD_NOW | RTLD_LOCAL);
    if (!lib->handle) {
        return -1;
    }
    
    // Get function pointers
    lib->create = (create_plugin_func)dlsym(lib->handle, "create_plugin_interface");
    lib->destroy = (destroy_plugin_func)dlsym(lib->handle, "destroy_plugin_interface");
    
    if (!lib->create || !lib->destroy) {
        dlclose(lib->handle);
        lib->handle = NULL;
        return -1;
    }
    
    strcpy(lib->path, plugin_path);
    lib->load_time = time(NULL);
    lib->load_count = 0;
    
    return 0;
}

static int plugin_unload_library(PluginLibrary* lib) {
    if (!lib || !lib->handle) return -1;
    
    dlclose(lib->handle);
    memset(lib, 0, sizeof(PluginLibrary));
    return 0;
}

// Utility functions
const char* plugin_get_type_string(PluginType type) {
    switch (type) {
        case PLUGIN_TYPE_SOLVER: return "Solver";
        case PLUGIN_TYPE_PREPROCESSOR: return "Preprocessor";
        case PLUGIN_TYPE_POSTPROCESSOR: return "Postprocessor";
        case PLUGIN_TYPE_VISUALIZATION: return "Visualization";
        case PLUGIN_TYPE_IO: return "I/O";
        case PLUGIN_TYPE_MATERIAL: return "Material";
        case PLUGIN_TYPE_MESH_GENERATION: return "Mesh Generation";
        case PLUGIN_TYPE_OPTIMIZATION: return "Optimization";
        default: return "Unknown";
    }
}

const char* plugin_get_status_string(PluginStatus status) {
    switch (status) {
        case PLUGIN_STATUS_UNLOADED: return "Unloaded";
        case PLUGIN_STATUS_LOADED: return "Loaded";
        case PLUGIN_STATUS_INITIALIZED: return "Initialized";
        case PLUGIN_STATUS_RUNNING: return "Running";
        case PLUGIN_STATUS_ERROR: return "Error";
        case PLUGIN_STATUS_INCOMPATIBLE: return "Incompatible";
        default: return "Unknown";
    }
}

int plugin_check_compatibility(const PluginInfo* info) {
    if (!info) return -1;
    
    // Check API version compatibility
    if (info->api_version != PLUGIN_API_VERSION) {
        return -1;
    }
    
    // Check plugin version format
    if (strlen(info->version) == 0) {
        return -1;
    }
    
    return 0;
}

const char* plugin_get_error_string(int error_code) {
    switch (error_code) {
        case PLUGIN_ERROR_NONE: return "No error";
        case PLUGIN_ERROR_MEMORY: return "Memory allocation failed";
        case PLUGIN_ERROR_LOAD_FAILED: return "Plugin load failed";
        case PLUGIN_ERROR_INVALID_PLUGIN: return "Invalid plugin";
        case PLUGIN_ERROR_INCOMPATIBLE: return "Incompatible plugin";
        case PLUGIN_ERROR_ALREADY_LOADED: return "Plugin already loaded";
        case PLUGIN_ERROR_NOT_FOUND: return "Plugin not found";
        case PLUGIN_ERROR_VALIDATION_FAILED: return "Plugin validation failed";
        case PLUGIN_ERROR_TIMEOUT: return "Plugin operation timed out";
        default: return "Unknown error";
    }
}

int plugin_get_last_error(void) {
    return g_last_error;
}