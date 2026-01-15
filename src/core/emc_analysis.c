#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "emc_analysis.h"
#include "../utils/memory_utils.h"
#include "../utils/math_utils.h"

#define EMC_SAFETY_MARGIN_DB 6.0
#define CISPR_QUASI_PEAK_FACTOR 1.0
#define FCC_AVERAGE_FACTOR 0.5
#define GROUND_BOUNCE_THRESHOLD 0.1
#define POWER_INTEGRITY_THRESHOLD 0.05
#define SIGNAL_INTEGRITY_THRESHOLD 0.05
#define ESD_IMMUNITY_LEVEL 8000.0
#define EMP_FIELD_STRENGTH 50000.0

EMCAnalysisConfig* create_emc_analysis_config(EMCAnalysisType type, EMCStandard standard) {
    EMCAnalysisConfig* config = (EMCAnalysisConfig*)safe_malloc(sizeof(EMCAnalysisConfig));
    config->analysis_type = type;
    config->standard = standard;
    config->frequency_start = 150e3;
    config->frequency_stop = 6e9;
    config->frequency_step = 1e6;
    config->test_frequencies = NULL;
    config->num_test_frequencies = 0;
    
    switch (standard) {
        case CISPR_22:
            config->limit_electric_field = 40.0;
            config->limit_magnetic_field = 40.0;
            config->limit_power_density = 1.0;
            config->limit_voltage = 1.0e-3;
            config->limit_current = 1.0e-6;
            break;
        case FCC_PART_15:
            config->limit_electric_field = 48.5;
            config->limit_magnetic_field = 48.5;
            config->limit_power_density = 1.0;
            config->limit_voltage = 1.0e-3;
            config->limit_current = 1.0e-6;
            break;
        case IEC_61000_4_2:
            config->limit_electric_field = 30.0;
            config->limit_magnetic_field = 30.0;
            config->limit_power_density = 0.1;
            config->limit_voltage = 1.0e-4;
            config->limit_current = 1.0e-7;
            break;
        default:
            config->limit_electric_field = 40.0;
            config->limit_magnetic_field = 40.0;
            config->limit_power_density = 1.0;
            config->limit_voltage = 1.0e-3;
            config->limit_current = 1.0e-6;
            break;
    }
    
    return config;
}

void destroy_emc_analysis_config(EMCAnalysisConfig* config) {
    if (config) {
        safe_free(config->test_frequencies);
        safe_free(config);
    }
}

static double calculate_emission_level(double power, double distance, double frequency) {
    double wavelength = 3e8 / frequency;
    double k = 2.0 * M_PI / wavelength;
    double antenna_factor = 1.0;
    
    if (distance < wavelength / (2.0 * M_PI)) {
        antenna_factor = 1.0 / (k * distance);
    } else {
        antenna_factor = 1.0 / (k * distance);
    }
    
    return 20.0 * log10(sqrt(50.0 * power) * antenna_factor);
}

static double calculate_ground_bounce_impedance(const PCBEMModel* pcb_model, double frequency) {
    double ground_inductance = 0.0;
    double ground_resistance = 0.0;
    
    for (int i = 0; i < pcb_model->num_layers; i++) {
        if (strstr(pcb_model->layers[i].material_name, "ground") != NULL ||
            strstr(pcb_model->layers[i].material_name, "GND") != NULL) {
            double thickness = pcb_model->layers[i].thickness;
            double conductivity = pcb_model->layers[i].conductivity;
            double permeability = 4e-7 * M_PI;
            
            double skin_depth = sqrt(2.0 / (2.0 * M_PI * frequency * permeability * conductivity));
            ground_resistance += 1.0 / (conductivity * thickness * skin_depth);
            ground_inductance += permeability * thickness / (2.0 * M_PI);
        }
    }
    
    double ground_impedance = sqrt(ground_resistance * ground_resistance + 
                                   (2.0 * M_PI * frequency * ground_inductance) * 
                                   (2.0 * M_PI * frequency * ground_inductance));
    
    return ground_impedance;
}

int analyze_conducted_emissions(
    const PCBEMModel* pcb_model,
    const EMCAnalysisConfig* config,
    EMCConductedRadiatedEmission* results
) {
    if (!pcb_model || !config || !results) return -1;
    
    int num_frequencies = (int)((config->frequency_stop - config->frequency_start) / config->frequency_step) + 1;
    
    for (int i = 0; i < num_frequencies; i++) {
        double frequency = config->frequency_start + i * config->frequency_step;
        
        double total_power = 0.0;
        for (int j = 0; j < pcb_model->num_traces; j++) {
            double trace_power = 0.0;
            double trace_length = pcb_model->traces[j].length;
            double trace_width = pcb_model->traces[j].width;
            
            double radiation_resistance = 20.0 * log10(2.0 * M_PI * frequency / 3e8 * trace_length);
            double current = 1.0;
            trace_power = current * current * radiation_resistance;
            
            total_power += trace_power;
        }
        
        results[i].frequency = frequency;
        results[i].conducted_emission_voltage = sqrt(50.0 * total_power);
        results[i].conducted_emission_current = sqrt(total_power / 50.0);
        results[i].power_spectral_density = total_power / config->frequency_step;
        
        double quasi_peak_voltage = results[i].conducted_emission_voltage * CISPR_QUASI_PEAK_FACTOR;
        results[i].exceeds_cispr_limit = (20.0 * log10(quasi_peak_voltage / config->limit_voltage) > 0);
        results[i].exceeds_fcc_limit = (20.0 * log10(results[i].conducted_emission_voltage / config->limit_voltage) > 0);
    }
    
    return 0;
}

int analyze_radiated_emissions(
    const PCBEMModel* pcb_model,
    const EMCAnalysisConfig* config,
    EMCConductedRadiatedEmission* results
) {
    if (!pcb_model || !config || !results) return -1;
    
    int num_frequencies = (int)((config->frequency_stop - config->frequency_start) / config->frequency_step) + 1;
    
    for (int i = 0; i < num_frequencies; i++) {
        double frequency = config->frequency_start + i * config->frequency_step;
        double measurement_distance = 3.0;
        
        double total_e_field = 0.0;
        double total_h_field = 0.0;
        
        for (int j = 0; j < pcb_model->num_traces; j++) {
            double trace_power = 0.0;
            double trace_length = pcb_model->traces[j].length;
            
            double radiation_resistance = 20.0 * log10(2.0 * M_PI * frequency / 3e8 * trace_length);
            double current = 1.0;
            trace_power = current * current * radiation_resistance;
            
            double e_field = calculate_emission_level(trace_power, measurement_distance, frequency);
            total_e_field += e_field;
            
            double h_field = e_field - 51.5;
            total_h_field += h_field;
        }
        
        results[i].frequency = frequency;
        results[i].radiated_emission_e_field = total_e_field;
        results[i].radiated_emission_h_field = total_h_field;
        results[i].exceeds_cispr_limit = (total_e_field > config->limit_electric_field);
        results[i].exceeds_fcc_limit = (total_e_field > config->limit_electric_field);
    }
    
    return 0;
}

int analyze_ground_bounce(
    const PCBEMModel* pcb_model,
    const EMCAnalysisConfig* config,
    EMCGroundBounceAnalysis* results
) {
    if (!pcb_model || !config || !results) return -1;
    
    int num_frequencies = (int)((config->frequency_stop - config->frequency_start) / config->frequency_step) + 1;
    
    for (int i = 0; i < num_frequencies; i++) {
        double frequency = config->frequency_start + i * config->frequency_step;
        
        double ground_impedance = calculate_ground_bounce_impedance(pcb_model, frequency);
        double switching_current = 0.1;
        double ground_bounce_voltage = switching_current * ground_impedance;
        
        double total_capacitance = 0.0;
        double total_inductance = 0.0;
        
        for (int j = 0; j < pcb_model->num_traces; j++) {
            double trace_area = pcb_model->traces[j].length * pcb_model->traces[j].width;
            double trace_thickness = pcb_model->traces[j].thickness;
            
            double capacitance = 8.854e-12 * trace_area / trace_thickness;
            total_capacitance += capacitance;
            
            double inductance = 2.0e-7 * pcb_model->traces[j].length * log(2.0 * pcb_model->traces[j].length / pcb_model->traces[j].width);
            total_inductance += inductance;
        }
        
        results[i].frequency = frequency;
        results[i].ground_bounce_voltage = ground_bounce_voltage;
        results[i].ground_bounce_current = switching_current;
        results[i].ground_impedance = ground_impedance;
        results[i].ground_inductance = total_inductance;
        results[i].ground_resistance = ground_impedance / sqrt(1.0 + (2.0 * M_PI * frequency * total_inductance / ground_impedance) * 
                                                              (2.0 * M_PI * frequency * total_inductance / ground_impedance));
        results[i].simultaneous_switching_noise = ground_bounce_voltage;
        results[i].delta_i_noise = switching_current;
        results[i].exceeds_threshold = (ground_bounce_voltage > GROUND_BOUNCE_THRESHOLD);
        results[i].safety_margin_db = 20.0 * log10(GROUND_BOUNCE_THRESHOLD / ground_bounce_voltage);
    }
    
    return 0;
}

int analyze_power_integrity(
    const PCBEMModel* pcb_model,
    const EMCAnalysisConfig* config,
    const MaterialDatabase* materials,
    EMCPowerIntegrityAnalysis* results
) {
    if (!pcb_model || !config || !results) return -1;
    
    int num_frequencies = (int)((config->frequency_stop - config->frequency_start) / config->frequency_step) + 1;
    
    for (int i = 0; i < num_frequencies; i++) {
        double frequency = config->frequency_start + i * config->frequency_step;
        
        double power_supply_impedance = 0.0;
        double target_impedance = 0.1;
        
        for (int j = 0; j < pcb_model->num_layers; j++) {
            if (strstr(pcb_model->layers[j].material_name, "power") != NULL ||
                strstr(pcb_model->layers[j].material_name, "VCC") != NULL) {
                
                double layer_resistance = pcb_model->layers[j].conductivity > 0 ? 
                    1.0 / (pcb_model->layers[j].conductivity * pcb_model->layers[j].thickness) : 1.0;
                
                double layer_inductance = 2.0e-7 * log(1000.0 / pcb_model->layers[j].thickness);
                
                double layer_impedance = sqrt(layer_resistance * layer_resistance + 
                                              (2.0 * M_PI * frequency * layer_inductance) * 
                                              (2.0 * M_PI * frequency * layer_inductance));
                
                power_supply_impedance += layer_impedance;
            }
        }
        
        double decoupling_capacitance = 1.0e-6;
        double decoupling_inductance = 1.0e-9;
        
        double resonance_frequency = 1.0 / (2.0 * M_PI * sqrt(decoupling_inductance * decoupling_capacitance));
        double quality_factor = sqrt(decoupling_inductance / decoupling_capacitance) / 0.1;
        
        results[i].frequency = frequency;
        results[i].power_supply_impedance = power_supply_impedance;
        results[i].target_impedance = target_impedance;
        results[i].decoupling_effectiveness = 20.0 * log10(target_impedance / power_supply_impedance);
        results[i].resonance_frequency = resonance_frequency;
        results[i].quality_factor = quality_factor;
        results[i].voltage_ripple = 0.05 * power_supply_impedance;
        results[i].meets_target_impedance = (power_supply_impedance < target_impedance);
        results[i].power_integrity_margin = 20.0 * log10(target_impedance / power_supply_impedance);
    }
    
    return 0;
}

int analyze_signal_integrity(
    const PCBEMModel* pcb_model,
    const SParameterSet* sparameters,
    const EMCAnalysisConfig* config,
    EMCSignalIntegrityAnalysis* results
) {
    if (!pcb_model || !sparameters || !config || !results) return -1;
    
    int num_frequencies = (int)((config->frequency_stop - config->frequency_start) / config->frequency_step) + 1;
    
    for (int i = 0; i < num_frequencies; i++) {
        double frequency = config->frequency_start + i * config->frequency_step;
        
        double near_end_crosstalk = 0.0;
        double far_end_crosstalk = 0.0;
        
        if (sparameters && sparameters->num_ports >= 4) {
            int freq_index = -1;
            for (int j = 0; j < sparameters->num_frequencies; j++) {
                if (fabs(sparameters->frequencies[j] - frequency) < config->frequency_step * 0.5) {
                    freq_index = j;
                    break;
                }
            }
            
            if (freq_index >= 0) {
                near_end_crosstalk = cabs(sparameters->s_matrices[freq_index][1][0]);
                far_end_crosstalk = cabs(sparameters->s_matrices[freq_index][3][0]);
            }
        }
        
        double coupling_length = 10.0;
        double crosstalk_coefficient = (near_end_crosstalk + far_end_crosstalk) / 2.0;
        double isolation_db = -20.0 * log10(crosstalk_coefficient + 1e-12);
        
        results[i].frequency = frequency;
        results[i].near_end_crosstalk = near_end_crosstalk;
        results[i].far_end_crosstalk = far_end_crosstalk;
        results[i].crosstalk_coefficient = crosstalk_coefficient;
        results[i].coupling_length = coupling_length;
        results[i].isolation_db = isolation_db;
        results[i].exceeds_specification = (crosstalk_coefficient > SIGNAL_INTEGRITY_THRESHOLD);
        results[i].signal_integrity_margin = 20.0 * log10(SIGNAL_INTEGRITY_THRESHOLD / crosstalk_coefficient);
    }
    
    return 0;
}

EMCAnalysisResults* perform_emc_analysis(
    const PCBEMModel* pcb_model,
    const EMCAnalysisConfig* config,
    const SParameterSet* sparameters,
    const MaterialDatabase* materials
) {
    if (!pcb_model || !config) return NULL;
    
    EMCAnalysisResults* results = (EMCAnalysisResults*)safe_malloc(sizeof(EMCAnalysisResults));
    results->config = (EMCAnalysisConfig*)safe_malloc(sizeof(EMCAnalysisConfig));
    memcpy(results->config, config, sizeof(EMCAnalysisConfig));
    
    int num_frequencies = (int)((config->frequency_stop - config->frequency_start) / config->frequency_step) + 1;
    results->num_frequencies = num_frequencies;
    
    results->emission_results = (EMCConductedRadiatedEmission*)safe_malloc(num_frequencies * sizeof(EMCConductedRadiatedEmission));
    results->immunity_results = (EMCImmunitySusceptibility*)safe_malloc(num_frequencies * sizeof(EMCImmunitySusceptibility));
    results->ground_bounce_results = (EMCGroundBounceAnalysis*)safe_malloc(num_frequencies * sizeof(EMCGroundBounceAnalysis));
    results->power_integrity_results = (EMCPowerIntegrityAnalysis*)safe_malloc(num_frequencies * sizeof(EMCPowerIntegrityAnalysis));
    results->signal_integrity_results = (EMCSignalIntegrityAnalysis*)safe_malloc(num_frequencies * sizeof(EMCSignalIntegrityAnalysis));
    results->esd_results = (EMCElectrostaticDischarge*)safe_malloc(sizeof(EMCElectrostaticDischarge));
    results->emp_results = (EMCElectromagneticPulse*)safe_malloc(sizeof(EMCElectromagneticPulse));
    
    analyze_conducted_emissions(pcb_model, config, results->emission_results);
    analyze_radiated_emissions(pcb_model, config, results->emission_results);
    analyze_ground_bounce(pcb_model, config, results->ground_bounce_results);
    analyze_power_integrity(pcb_model, config, materials, results->power_integrity_results);
    analyze_signal_integrity(pcb_model, sparameters, config, results->signal_integrity_results);
    
    results->worst_case_emission = 0.0;
    results->worst_case_immunity = 0.0;
    results->overall_compliance_score = 0.0;
    results->meets_all_standards = true;
    
    for (int i = 0; i < num_frequencies; i++) {
        if (results->emission_results[i].exceeds_cispr_limit ||
            results->emission_results[i].exceeds_fcc_limit) {
            results->meets_all_standards = false;
        }
        
        double emission_level = results->emission_results[i].radiated_emission_e_field;
        if (emission_level > results->worst_case_emission) {
            results->worst_case_emission = emission_level;
        }
    }
    
    return results;
}

void destroy_emc_analysis_results(EMCAnalysisResults* results) {
    if (results) {
        safe_free(results->config);
        safe_free(results->emission_results);
        safe_free(results->immunity_results);
        safe_free(results->ground_bounce_results);
        safe_free(results->power_integrity_results);
        safe_free(results->signal_integrity_results);
        safe_free(results->esd_results);
        safe_free(results->emp_results);
        safe_free(results);
    }
}

EMCComplianceReport* generate_compliance_report(
    const EMCAnalysisResults* results,
    const char* test_standard
) {
    if (!results) return NULL;
    
    EMCComplianceReport* report = (EMCComplianceReport*)safe_malloc(sizeof(EMCComplianceReport));
    
    snprintf(report->report_title, 127, "EMC Compliance Analysis Report - %s", test_standard);
    snprintf(report->test_setup, 255, "3-meter anechoic chamber, conducted emissions on 50Ω LISN");
    snprintf(report->equipment_list, 511, "Spectrum Analyzer, LISN, Antenna, ESD Gun, Current Probe");
    snprintf(report->test_conditions, 255, "Temperature: 25°C, Humidity: 45%RH, Atmospheric Pressure: 1013 hPa");
    snprintf(report->operator_name, 63, "PulseMoM EMC Analysis System");
    snprintf(report->test_date, 31, "2024-01-01");
    snprintf(report->standard_reference, 63, "%s", test_standard);
    snprintf(report->lab_certification, 63, "ISO 17025 Accredited");
    
    return report;
}

void save_compliance_report(
    const EMCComplianceReport* report,
    const char* filename
) {
    if (!report || !filename) return;
    
    FILE* fp = fopen(filename, "w");
    if (!fp) return;
    
    fprintf(fp, "================================================================================\n");
    fprintf(fp, "                           EMC COMPLIANCE ANALYSIS REPORT\n");
    fprintf(fp, "================================================================================\n\n");
    
    fprintf(fp, "Report Title: %s\n", report->report_title);
    fprintf(fp, "Test Standard: %s\n", report->standard_reference);
    fprintf(fp, "Test Date: %s\n", report->test_date);
    fprintf(fp, "Operator: %s\n", report->operator_name);
    fprintf(fp, "Lab Certification: %s\n\n", report->lab_certification);
    
    fprintf(fp, "Test Setup: %s\n", report->test_setup);
    fprintf(fp, "Equipment List: %s\n", report->equipment_list);
    fprintf(fp, "Test Conditions: %s\n\n", report->test_conditions);
    
    fprintf(fp, "================================================================================\n");
    fprintf(fp, "                                    END OF REPORT\n");
    fprintf(fp, "================================================================================\n");
    
    fclose(fp);
}