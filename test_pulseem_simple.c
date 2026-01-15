/*********************************************************************
 * Simple test for PulseEM argument parsing
 *********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Simplified version of the mode parsing function
typedef enum {
    MODE_MOM, MODE_PEEC, MODE_HYBRID, MODE_HELP, MODE_VERSION
} PulseEMMode;

PulseEMMode parse_mode(const char* mode_str) {
    if (strcmp(mode_str, "mom") == 0) return MODE_MOM;
    if (strcmp(mode_str, "peec") == 0) return MODE_PEEC;
    if (strcmp(mode_str, "hybrid") == 0) return MODE_HYBRID;
    if (strcmp(mode_str, "help") == 0) return MODE_HELP;
    if (strcmp(mode_str, "version") == 0) return MODE_VERSION;
    return MODE_HELP;
}

const char* mode_to_string(PulseEMMode mode) {
    switch (mode) {
        case MODE_MOM: return "MoM";
        case MODE_PEEC: return "PEEC";
        case MODE_HYBRID: return "Hybrid";
        case MODE_HELP: return "Help";
        case MODE_VERSION: return "Version";
        default: return "Unknown";
    }
}

int main() {
    printf("=== PulseEM Mode Parsing Test ===\n");
    
    // Test valid modes
    const char* test_modes[] = {"mom", "peec", "hybrid", "help", "version", "invalid"};
    int num_tests = sizeof(test_modes) / sizeof(test_modes[0]);
    
    for (int i = 0; i < num_tests; i++) {
        PulseEMMode mode = parse_mode(test_modes[i]);
        printf("Input: %-10s -> Mode: %s\n", test_modes[i], mode_to_string(mode));
    }
    
    printf("\n=== Test completed successfully ===\n");
    return 0;
}