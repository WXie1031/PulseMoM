#ifndef EMC_ANALYSIS_H
#define EMC_ANALYSIS_H

#include <complex.h>
#include <stdbool.h>
#include "pcb_structure.h"
#include "enhanced_sparameter_extraction.h"
#include "advanced_material_models.h"

typedef enum {
    EMC_EMISSION,
    EMC_IMMUNITY,
    EMC_COUPLING,
    EMC_GROUND_BOUNCE,
    EMC_POWER_INTEGRITY,
    EMC_SIGNAL_INTEGRITY,
    EMC_ELECTROSTATIC_DISCHARGE,
    EMC_ELECTROMAGNETIC_PULSE
} EMCAnalysisType;

typedef enum {
    CISPR_22,
    CISPR_32,
    FCC_PART_15,
    IEC_61000_4_2,
    IEC_61000_4_3,
    MIL_STD_461,
    RTCA_DO_160,
    AUTOMOTIVE_ISO_11452
} EMCStandard;

typedef struct {
    double frequency;
    double electric_field;
    double magnetic_field;
    double power_density;
    double voltage;
    double current;
    double impedance;
    bool exceeds_limit;
} EMCMeasurement;

typedef struct {
    EMCAnalysisType analysis_type;
    EMCStandard standard;
    double frequency_start;
    double frequency_stop;
    double frequency_step;
    double* test_frequencies;
    int num_test_frequencies;
    double limit_electric_field;
    double limit_magnetic_field;
    double limit_power_density;
    double limit_voltage;
    double limit_current;
} EMCAnalysisConfig;

typedef struct {
    char source_name[64];
    double source_power;
    double source_frequency;
    double source_bandwidth;
    double source_location[3];
    double source_orientation[3];
    bool is_differential;
    double differential_mode_rejection;
} EMCNoiseSource;

typedef struct {
    char victim_name[64];
    double victim_sensitivity;
    double victim_bandwidth;
    double victim_location[3];
    double victim_orientation[3];
    double victim_impedance;
    bool is_differential;
    double common_mode_rejection;
} EMCVictim;

typedef struct {
    EMCNoiseSource* sources;
    EMCVictim* victims;
    int num_sources;
    int num_victims;
    double coupling_matrix[100][100];
    double transfer_impedance[100][100];
    double mutual_inductance[100][100];
    double mutual_capacitance[100][100];
    double near_field_coupling[100][100];
    double far_field_radiation[100][100];
} EMCCouplingAnalysis;

typedef struct {
    double frequency;
    double conducted_emission_voltage;
    double conducted_emission_current;
    double radiated_emission_e_field;
    double radiated_emission_h_field;
    double power_spectral_density;
    double spurious_emission_level;
    double harmonic_distortion;
    bool exceeds_cispr_limit;
    bool exceeds_fcc_limit;
} EMCConductedRadiatedEmission;

typedef struct {
    double frequency;
    double immunity_voltage_level;
    double immunity_current_level;
    double immunity_field_level;
    double susceptibility_threshold;
    double desensitization_level;
    double bit_error_rate;
    double signal_to_noise_ratio;
    bool fails_immunity_test;
    bool meets_safety_margin;
} EMCImmunitySusceptibility;

typedef struct {
    double frequency;
    double ground_bounce_voltage;
    double ground_bounce_current;
    double ground_impedance;
    double ground_inductance;
    double ground_resistance;
    double simultaneous_switching_noise;
    double delta_i_noise;
    bool exceeds_threshold;
    double safety_margin_db;
} EMCGroundBounceAnalysis;

typedef struct {
    double frequency;
    double power_supply_noise;
    double power_supply_impedance;
    double target_impedance;
    double decoupling_effectiveness;
    double resonance_frequency;
    double quality_factor;
    double voltage_ripple;
    bool meets_target_impedance;
    double power_integrity_margin;
} EMCPowerIntegrityAnalysis;

typedef struct {
    double frequency;
    double crosstalk_voltage;
    double crosstalk_current;
    double near_end_crosstalk;
    double far_end_crosstalk;
    double crosstalk_coefficient;
    double coupling_length;
    double isolation_db;
    bool exceeds_specification;
    double signal_integrity_margin;
} EMCSignalIntegrityAnalysis;

typedef struct {
    double esd_voltage;
    double esd_current;
    double esd_energy;
    double rise_time;
    double fall_time;
    double repetition_rate;
    double coupling_factor;
    double immunity_level;
    bool passes_esd_test;
    double safety_margin;
} EMCElectrostaticDischarge;

typedef struct {
    double emp_field_strength;
    double emp_rise_time;
    double emp_pulse_width;
    double coupling_efficiency;
    double induced_voltage;
    double induced_current;
    double system_hardness;
    double shielding_effectiveness;
    bool survives_emp;
    double emp_margin;
} EMCElectromagneticPulse;

typedef struct {
    EMCAnalysisConfig* config;
    EMCConductedRadiatedEmission* emission_results;
    EMCImmunitySusceptibility* immunity_results;
    EMCGroundBounceAnalysis* ground_bounce_results;
    EMCPowerIntegrityAnalysis* power_integrity_results;
    EMCSignalIntegrityAnalysis* signal_integrity_results;
    EMCElectrostaticDischarge* esd_results;
    EMCElectromagneticPulse* emp_results;
    int num_frequencies;
    double worst_case_emission;
    double worst_case_immunity;
    double overall_compliance_score;
    bool meets_all_standards;
} EMCAnalysisResults;

typedef struct {
    char report_title[128];
    char test_setup[256];
    char equipment_list[512];
    char test_conditions[256];
    char operator_name[64];
    char test_date[32];
    char standard_reference[64];
    char lab_certification[64];
} EMCComplianceReport;

EMCAnalysisConfig* create_emc_analysis_config(EMCAnalysisType type, EMCStandard standard);
void destroy_emc_analysis_config(EMCAnalysisConfig* config);

EMCAnalysisResults* perform_emc_analysis(
    const PCBEMModel* pcb_model,
    const EMCAnalysisConfig* config,
    const SParameterSet* sparameters,
    const MaterialDatabase* materials
);

void destroy_emc_analysis_results(EMCAnalysisResults* results);

int analyze_conducted_emissions(
    const PCBEMModel* pcb_model,
    const EMCAnalysisConfig* config,
    EMCConductedRadiatedEmission* results
);

int analyze_radiated_emissions(
    const PCBEMModel* pcb_model,
    const EMCAnalysisConfig* config,
    EMCConductedRadiatedEmission* results
);

int analyze_immunity_susceptibility(
    const PCBEMModel* pcb_model,
    const EMCAnalysisConfig* config,
    EMCImmunitySusceptibility* results
);

int analyze_ground_bounce(
    const PCBEMModel* pcb_model,
    const EMCAnalysisConfig* config,
    EMCGroundBounceAnalysis* results
);

int analyze_power_integrity(
    const PCBEMModel* pcb_model,
    const EMCAnalysisConfig* config,
    const MaterialDatabase* materials,
    EMCPowerIntegrityAnalysis* results
);

int analyze_signal_integrity(
    const PCBEMModel* pcb_model,
    const SParameterSet* sparameters,
    const EMCAnalysisConfig* config,
    EMCSignalIntegrityAnalysis* results
);

int analyze_electrostatic_discharge(
    const PCBEMModel* pcb_model,
    const EMCAnalysisConfig* config,
    EMCElectrostaticDischarge* results
);

int analyze_electromagnetic_pulse(
    const PCBEMModel* pcb_model,
    const EMCAnalysisConfig* config,
    EMCElectromagneticPulse* results
);

EMCCouplingAnalysis* create_coupling_analysis(int max_sources, int max_victims);
void destroy_coupling_analysis(EMCCouplingAnalysis* analysis);

int calculate_coupling_parameters(
    EMCCouplingAnalysis* analysis,
    const PCBEMModel* pcb_model,
    const SParameterSet* sparameters
);

double calculate_crosstalk_coefficient(
    const EMCCouplingAnalysis* analysis,
    int source_index,
    int victim_index
);

double calculate_shielding_effectiveness(
    const PCBEMModel* pcb_model,
    double frequency,
    const char* shield_type
);

double calculate_ground_impedance(
    const PCBEMModel* pcb_model,
    double frequency,
    const char* ground_plane
);

EMCComplianceReport* generate_compliance_report(
    const EMCAnalysisResults* results,
    const char* test_standard
);

void save_compliance_report(
    const EMCComplianceReport* report,
    const char* filename
);

int validate_emc_compliance(
    const EMCAnalysisResults* results,
    EMCStandard standard
);

typedef struct {
    char mitigation_technique[64];
    char description[256];
    double effectiveness_db;
    double cost_estimate;
    double implementation_difficulty;
    char affected_frequencies[64];
    char design_rules[256];
} EMCMitigationStrategy;

EMCMitigationStrategy* generate_mitigation_strategies(
    const EMCAnalysisResults* results,
    int* num_strategies
);

void apply_mitigation_strategy(
    PCBEMModel* pcb_model,
    const EMCMitigationStrategy* strategy
);

#endif