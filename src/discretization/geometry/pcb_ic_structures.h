/******************************************************************************
 * PCB/IC Structure Type Matrix Definition
 * 
 * Comprehensive enumeration and classification of all PCB and integrated
 * circuit structures for electromagnetic simulation
 ******************************************************************************/

#ifndef PCB_IC_STRUCTURES_H
#define PCB_IC_STRUCTURES_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * Primary Structure Categories
 ******************************************************************************/
typedef enum {
    STRUCT_CATEGORY_NONE = 0,
    
    /* Basic PCB Structures */
    STRUCT_CATEGORY_TRANSMISSION_LINE,
    STRUCT_CATEGORY_VIA,
    STRUCT_CATEGORY_PAD,
    STRUCT_CATEGORY_COMPONENT,
    STRUCT_CATEGORY_LAYER,
    
    /* Advanced PCB Structures */
    STRUCT_CATEGORY_ANTENNA,
    STRUCT_CATEGORY_FILTER,
    STRUCT_CATEGORY_COUPLED_STRUCTURE,
    STRUCT_CATEGORY_POWER_DISTRIBUTION,
    STRUCT_CATEGORY_GROUNDING,
    
    /* IC Package Structures */
    STRUCT_CATEGORY_PACKAGE,
    STRUCT_CATEGORY_BONDWIRE,
    STRUCT_CATEGORY_LEADFRAME,
    STRUCT_CATEGORY_MOLD_COMPOUND,
    STRUCT_CATEGORY_DIE_ATTACH,
    
    /* On-Chip Structures */
    STRUCT_CATEGORY_ON_CHIP_INTERCONNECT,
    STRUCT_CATEGORY_ON_CHIP_DEVICE,
    STRUCT_CATEGORY_ON_CHIP_PASSIVE,
    STRUCT_CATEGORY_ON_CHIP_POWER,
    STRUCT_CATEGORY_ON_CHIP_CLOCK,
    
    /* Special Structures */
    STRUCT_CATEGORY_METAMATERIAL,
    STRUCT_CATEGORY_PERIODIC_STRUCTURE,
    STRUCT_CATEGORY_ENCLOSURE,
    STRUCT_CATEGORY_CONNECTOR,
    STRUCT_CATEGORY_MECHANICAL,
    
    STRUCT_CATEGORY_COUNT
} StructureCategory;

/******************************************************************************
 * Transmission Line Types
 ******************************************************************************/
typedef enum {
    TRANSMISSION_LINE_NONE = 0,
    
    /* Single-ended Lines */
    TRANSMISSION_LINE_MICROSTRIP,
    TRANSMISSION_LINE_STRIPLINE,
    TRANSMISSION_LINE_COPLANAR_WAVEGUIDE,
    TRANSMISSION_LINE_SLOT_LINE,
    TRANSMISSION_LINE_COPLANAR_STRIPLINE,
    
    /* Differential Lines */
    TRANSMISSION_LINE_DIFFERENTIAL_MICROSTRIP,
    TRANSMISSION_LINE_DIFFERENTIAL_STRIPLINE,
    TRANSMISSION_LINE_DIFFERENTIAL_COPLANAR,
    TRANSMISSION_LINE_BROADSIDE_COUPLED,
    
    /* Specialized Lines */
    TRANSMISSION_LINE_SUSPENDED_SUBSTRATE,
    TRANSMISSION_LINE_INVERTED_MICROSTRIP,
    TRANSMISSION_LINE_FIN_LINE,
    TRANSMISSION_LINE_IMAGE_GUIDE,
    TRANSMISSION_LINE_DIELECTRIC_WAVEGUIDE,
    
    /* High-frequency Lines */
    TRANSMISSION_LINE_RECTANGULAR_WAVEGUIDE,
    TRANSMISSION_LINE_CIRCULAR_WAVEGUIDE,
    TRANSMISSION_LINE_RIDGE_WAVEGUIDE,
    TRANSMISSION_LINE_ELLIPTICAL_WAVEGUIDE,
    
    /* On-Chip Lines */
    TRANSMISSION_LINE_ON_CHIP_MICROSTRIP,
    TRANSMISSION_LINE_ON_CHIP_COPLANAR,
    TRANSMISSION_LINE_ON_CHIP_STRIPLINE,
    TRANSMISSION_LINE_ON_CHIP_DIFFERENTIAL,
    
    TRANSMISSION_LINE_COUNT
} TransmissionLineType;

/******************************************************************************
 * Via Types
 ******************************************************************************/
typedef enum {
    VIA_NONE = 0,
    
    /* Standard Vias */
    VIA_THROUGH_HOLE,
    VIA_BLIND,
    VIA_BURIED,
    VIA_MICROVIA,
    VIA_STACKED_MICROVIA,
    VIA_STAGGERED_MICROVIA,
    
    /* Via-in-Pad */
    VIA_IN_PAD,
    VIA_IN_PAD_FILLED,
    VIA_IN_PAD_CAPPED,
    
    /* Thermal Vias */
    VIA_THERMAL,
    VIA_THERMAL_FILLED,
    VIA_THERMAL_CAPPED,
    
    /* High-frequency Vias */
    VIA_COAXIAL,
    VIA_GROUND_SLEEVE,
    VIA_BACKDRILLED,
    VIA_TENTED,
    
    /* Package Vias */
    VIA_PACKAGE_THROUGH,
    VIA_PACKAGE_BLIND,
    VIA_PACKAGE_THERMAL,
    VIA_PACKAGE_SIGNAL,
    
    VIA_COUNT
} ViaType;

/******************************************************************************
 * Pad Types
 ******************************************************************************/
typedef enum {
    PAD_NONE = 0,
    
    /* SMD Pads */
    PAD_SMD_RECTANGULAR,
    PAD_SMD_ROUND,
    PAD_SMD_OVAL,
    PAD_SMD_POLYGON,
    
    /* Through-hole Pads */
    PAD_TH_ROUND,
    PAD_TH_RECTANGULAR,
    PAD_TH_OVAL,
    PAD_TH_SLOT,
    
    /* Special Pads */
    PAD_THERMAL_RELIEF,
    PAD_ANTIPAD,
    PAD_VIA_PAD,
    PAD_TEST_POINT,
    PAD_PRESS_FIT,
    
    /* High-frequency Pads */
    PAD_GCPW,
    PAD_CPW,
    PAD_LAUNCHER,
    PAD_PROBE,
    
    /* Package Pads */
    PAD_PACKAGE_BGA,
    PAD_PACKAGE_QFP,
    PAD_PACKAGE_QFN,
    PAD_PACKAGE_CSP,
    PAD_PACKAGE_LGA,
    
    PAD_COUNT
} PadType;

/******************************************************************************
 * Component Types
 ******************************************************************************/
typedef enum {
    COMPONENT_NONE = 0,
    
    /* Passive Components */
    COMPONENT_RESISTOR,
    COMPONENT_CAPACITOR,
    COMPONENT_INDUCTOR,
    COMPONENT_FERRITE_BEAD,
    COMPONENT_VARISTOR,
    
    /* Active Components */
    COMPONENT_DIODE,
    COMPONENT_TRANSISTOR,
    COMPONENT_IC,
    COMPONENT_OPTOCOUPLER,
    COMPONENT_LED,
    
    /* RF Components */
    COMPONENT_RF_AMPLIFIER,
    COMPONENT_RF_MIXER,
    COMPONENT_RF_FILTER,
    COMPONENT_RF_SWITCH,
    COMPONENT_RF_ATTENUATOR,
    
    /* Power Components */
    COMPONENT_POWER_REGULATOR,
    COMPONENT_POWER_MODULE,
    COMPONENT_TRANSFORMER,
    COMPONENT_RELAY,
    COMPONENT_FUSE,
    
    /* Connectors */
    COMPONENT_CONNECTOR,
    COMPONENT_HEADER,
    COMPONENT_SOCKET,
    COMPONENT_SWITCH,
    
    /* Mechanical */
    COMPONENT_HEATSINK,
    COMPONENT_SHIELD,
    COMPONENT_MOUNTING_HOLE,
    COMPONENT_STANDOFF,
    
    COMPONENT_COUNT
} ComponentType;

/******************************************************************************
 * Layer Types
 ******************************************************************************/
typedef enum {
    LAYER_NONE = 0,
    
    /* Signal Layers */
    LAYER_SIGNAL_TOP,
    LAYER_SIGNAL_BOTTOM,
    LAYER_SIGNAL_INTERNAL,
    LAYER_SIGNAL_POWER,
    LAYER_SIGNAL_GROUND,
    
    /* Dielectric Layers */
    LAYER_DIELECTRIC_CORE,
    LAYER_DIELECTRIC_PREPREG,
    LAYER_DIELECTRIC_SOLDER_MASK,
    LAYER_DIELECTRIC_SOLDER_PASTE,
    LAYER_DIELECTRIC_ADHESIVE,
    
    /* Surface Finish */
    LAYER_SURFACE_FINISH_HASL,
    LAYER_SURFACE_FINISH_ENIG,
    LAYER_SURFACE_FINISH_IMMERSION_SILVER,
    LAYER_SURFACE_FINISH_IMMERSION_TIN,
    LAYER_SURFACE_FINISH_OSP,
    
    /* Manufacturing */
    LAYER_SILKSCREEN_TOP,
    LAYER_SILKSCREEN_BOTTOM,
    LAYER_ASSEMBLY_TOP,
    LAYER_ASSEMBLY_BOTTOM,
    LAYER_COURTYARD_TOP,
    LAYER_COURTYARD_BOTTOM,
    
    /* Package */
    LAYER_PACKAGE_SUBSTRATE,
    LAYER_PACKAGE_DIELECTRIC,
    LAYER_PACKAGE_METAL,
    LAYER_PACKAGE_SOLDER_MASK,
    
    LAYER_COUNT
} LayerType;

/******************************************************************************
 * Antenna Types
 ******************************************************************************/
typedef enum {
    ANTENNA_NONE = 0,
    
    /* Wire Antennas */
    ANTENNA_DIPOLE,
    ANTENNA_MONOPOLE,
    ANTENNA_LOOP,
    ANTENNA_SPIRAL,
    ANTENNA_HELICAL,
    
    /* Patch Antennas */
    ANTENNA_RECTANGULAR_PATCH,
    ANTENNA_CIRCULAR_PATCH,
    ANTENNA_TRIANGULAR_PATCH,
    ANTENNA_ELLIPTICAL_PATCH,
    ANTENNA_ANNULAR_RING_PATCH,
    
    /* Array Antennas */
    ANTENNA_LINEAR_ARRAY,
    ANTENNA_PLANAR_ARRAY,
    ANTENNA_CONFORMAL_ARRAY,
    ANTENNA_PHASED_ARRAY,
    
    /* Slot Antennas */
    ANTENNA_SLOT,
    ANTENNA_WAVEGUIDE_SLOT_ARRAY,
    ANTENNA_CAVITY_BACKED_SLOT,
    
    /* Horn Antennas */
    ANTENNA_PYRAMIDAL_HORN,
    ANTENNA_CONICAL_HORN,
    ANTENNA_CORRUGATED_HORN,
    
    /* Reflector Antennas */
    ANTENNA_PARABOLIC_REFLECTOR,
    ANTENNA_CASSEGRAIN,
    ANTENNA_FLATE_PLATE_REFLECTOR,
    
    /* On-Chip Antennas */
    ANTENNA_ON_CHIP_DIPOLE,
    ANTENNA_ON_CHIP_PATCH,
    ANTENNA_ON_CHIP_SPIRAL,
    ANTENNA_ON_CHIP_LOOP,
    
    ANTENNA_COUNT
} AntennaType;

/******************************************************************************
 * Metamaterial Types
 ******************************************************************************/
typedef enum {
    METAMATERIAL_NONE = 0,
    
    /* Split Ring Resonators */
    METAMATERIAL_SRR,
    METAMATERIAL_CSRR,
    METAMATERIAL_DSRR,
    METAMATERIAL_MSRR,
    
    /* Wire Structures */
    METAMATERIAL_WIRE_GRID,
    METAMATERIAL_WIRE_MESH,
    METAMATERIAL_THIN_WIRE,
    METAMATERIAL_LOADED_WIRE,
    
    /* Patch Structures */
    METAMATERIAL_PATCH_ARRAY,
    METAMATERIAL_FRACTAL_PATCH,
    METAMATERIAL_LOADED_PATCH,
    METAMATERIAL_STACKED_PATCH,
    
    /* Transmission Line Based */
    METAMATERIAL_CRLH_TL,
    METAMATERIAL_COMPOSITE_RH_LH,
    METAMATERIAL_ZERO_ORDER_RESONATOR,
    
    /* Frequency Selective Surfaces */
    METAMATERIAL_FSS_DIPOLE,
    METAMATERIAL_FSS_LOOP,
    METAMATERIAL_FSS_PATCH,
    METAMATERIAL_FSS_JERUSALEM_CROSS,
    
    /* Metasurfaces */
    METAMATERIAL_HUYGENS_SURFACE,
    METAMATERIAL_REFLECTARRAY,
    METAMATERIAL_TRANSMITARRAY,
    
    METAMATERIAL_COUNT
} MetamaterialType;

/******************************************************************************
 * Enclosure Types
 ******************************************************************************/
typedef enum {
    ENCLOSURE_NONE = 0,
    
    /* Metallic Enclosures */
    ENCLOSURE_RECTANGULAR_CAVITY,
    ENCLOSURE_CIRCULAR_CAVITY,
    ENCLOSURE_COAXIAL_CAVITY,
    ENCLOSURE_WAVEGUIDE_CAVITY,
    
    /* Shielding Structures */
    ENCLOSURE_FARADAY_CAGE,
    ENCLOSURE_CONDUCTIVE_ENCLOSURE,
    ENCLOSURE_MESH_SHIELD,
    ENCLOSURE_FOIL_SHIELD,
    
    /* Package Enclosures */
    ENCLOSURE_IC_PACKAGE,
    ENCLOSURE_QFN_PACKAGE,
    ENCLOSURE_BGA_PACKAGE,
    ENCLOSURE_CSP_PACKAGE,
    
    /* RF Enclosures */
    ENCLOSURE_RF_SHIELD_CAN,
    ENCLOSURE_RF_GASKET,
    ENCLOSURE_RF_ABSORBER,
    
    ENCLOSURE_COUNT
} EnclosureType;

/******************************************************************************
 * Structure Complexity Levels
 ******************************************************************************/
typedef enum {
    COMPLEXITY_SIMPLE = 0,
    COMPLEXITY_MODERATE,
    COMPLEXITY_COMPLEX,
    COMPLEXITY_VERY_COMPLEX,
    COMPLEXITY_EXTREME
} StructureComplexity;

/******************************************************************************
 * Structure Properties
 ******************************************************************************/
typedef struct {
    /* Basic Properties */
    StructureCategory category;
    uint32_t type;              /* Specific type within category */
    StructureComplexity complexity;
    
    /* Physical Properties */
    double length;               /* Characteristic length */
    double width;                /* Characteristic width */
    double height;               /* Characteristic height */
    double thickness;            /* Metal thickness */
    
    /* Electrical Properties */
    double conductivity;         /* S/m */
    double permittivity;         /* Relative permittivity */
    double permeability;         /* Relative permeability */
    double loss_tangent;         /* Dielectric loss tangent */
    
    /* Frequency Properties */
    double min_frequency;        /* Minimum valid frequency (Hz) */
    double max_frequency;        /* Maximum valid frequency (Hz) */
    double characteristic_frequency; /* Design frequency (Hz) */
    
    /* Simulation Properties */
    bool requires_3d_simulation;
    bool supports_2d_approximation;
    bool requires_full_wave;
    bool supports_quasi_static;
    
    /* Mesh Properties */
    double min_mesh_size;
    double max_mesh_size;
    double suggested_mesh_density;
    
    /* Material Properties */
    char material_name[64];
    char substrate_name[64];
    
} StructureProperties;

/******************************************************************************
 * Structure Classification Matrix
 ******************************************************************************/
typedef struct {
    /* Structure Identification */
    StructureCategory category;
    uint32_t type;
    const char* name;
    const char* description;
    
    /* Classification Properties */
    StructureComplexity complexity;
    bool is_periodic;
    bool is_symmetric;
    bool is_planar;
    bool is_multilayer;
    
    /* Simulation Requirements */
    bool requires_electromagnetic;
    bool requires_thermal;
    bool requires_mechanical;
    bool requires_coupled_field;
    
    /* Supported Analysis Types */
    bool supports_dc;
    bool supports_ac;
    bool supports_transient;
    bool supports_harmonic;
    bool supports_eigenmode;
    
    /* Default Properties */
    StructureProperties default_properties;
    
} StructureClassification;

/******************************************************************************
 * Structure Type Matrix Functions
 ******************************************************************************/

/* Structure classification matrix */
extern const StructureClassification* get_structure_classification_matrix(void);
extern uint32_t get_structure_classification_count(void);

/* Structure type queries */
extern const char* get_structure_category_name(StructureCategory category);
extern const char* get_structure_type_name(StructureCategory category, uint32_t type);
extern StructureComplexity get_structure_complexity(StructureCategory category, uint32_t type);

/* Structure property queries */
extern const StructureProperties* get_structure_default_properties(StructureCategory category, uint32_t type);
extern bool is_structure_periodic(StructureCategory category, uint32_t type);
extern bool is_structure_symmetric(StructureCategory category, uint32_t type);

/* Simulation requirement queries */
extern bool requires_3d_simulation(StructureCategory category, uint32_t type);
extern bool supports_2d_approximation(StructureCategory category, uint32_t type);
extern bool requires_full_wave_analysis(StructureCategory category, uint32_t type);

/* Structure validation */
extern bool is_valid_structure_type(StructureCategory category, uint32_t type);
extern bool is_valid_structure_combination(StructureCategory category1, uint32_t type1,
                                          StructureCategory category2, uint32_t type2);

/* Structure complexity analysis */
extern StructureComplexity analyze_structure_complexity(const StructureProperties* properties);
extern double estimate_computation_complexity(StructureCategory category, uint32_t type, 
                                             double frequency, double accuracy);

/* Specialized structure functions */
extern const StructureClassification* get_antenna_structures(uint32_t* count);
extern const StructureClassification* get_metamaterial_structures(uint32_t* count);
extern const StructureClassification* get_transmission_line_structures(uint32_t* count);
extern const StructureClassification* get_package_structures(uint32_t* count);

#ifdef __cplusplus
}
#endif

#endif /* PCB_IC_STRUCTURES_H */