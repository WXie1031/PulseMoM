/******************************************************************************
 * MoM Solver with Layered Medium Support
 * 
 * Extends MoM solver to support multilayer medium for:
 * - PCB simulation
 * - Antenna substrate simulation
 * - Metamaterial simulation
 * 
 * Implementation: Uses layered Green's function (Sommerfeld/DCIM)
 ******************************************************************************/

#ifndef MOM_LAYERED_MEDIUM_H
#define MOM_LAYERED_MEDIUM_H

#include "../../operators/greens/layered_greens_function.h"
#include "../../discretization/mesh/core_mesh.h"
#include "mom_solver.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * Layered Medium Configuration for MoM
 ******************************************************************************/

typedef struct {
    // Layered medium structure
    LayeredMedium* medium;                // Layered medium definition
    GreensFunctionParams* greens_params;   // Green's function parameters
    
    // Application mode
    enum {
        MOM_LAYERED_MODE_PCB,             // PCB simulation mode
        MOM_LAYERED_MODE_ANTENNA,         // Antenna substrate mode
        MOM_LAYERED_MODE_METAMATERIAL     // Metamaterial mode
    } application_mode;
    
    // Frequency-dependent materials
    bool use_frequency_dependent_materials; // Enable frequency-dependent materials
    
} mom_layered_medium_config_t;

/******************************************************************************
 * Layered Medium Functions
 ******************************************************************************/

/**
 * Set layered medium for MoM solver
 * 
 * @param solver MoM solver
 * @param medium Layered medium structure
 * @param greens_params Green's function parameters (NULL for defaults)
 * @return 0 on success, negative on error
 */
int mom_solver_set_layered_medium(
    mom_solver_t* solver,
    const LayeredMedium* medium,
    const FrequencyDomain* freq,
    const GreensFunctionParams* greens_params
);

/**
 * Configure MoM solver for PCB simulation
 * 
 * @param solver MoM solver
 * @param pcb_layers PCB layer information array
 * @param num_layers Number of layers
 * @return 0 on success, negative on error
 */
int mom_solver_configure_pcb(
    mom_solver_t* solver,
    const void* pcb_layers,  // PCBLayerInfo array
    int num_layers
);

/**
 * Configure MoM solver for antenna simulation
 * 
 * @param solver MoM solver
 * @param substrate_thickness Substrate thickness (m)
 * @param substrate_er Substrate relative permittivity
 * @param substrate_tan_delta Substrate loss tangent
 * @return 0 on success, negative on error
 */
int mom_solver_configure_antenna(
    mom_solver_t* solver,
    double substrate_thickness,
    double substrate_er,
    double substrate_tan_delta
);

/**
 * Configure MoM solver for metamaterial simulation
 * 
 * @param solver MoM solver
 * @param unit_cell_size Unit cell size (m)
 * @param effective_er Effective permittivity
 * @param effective_mu Effective permeability
 * @return 0 on success, negative on error
 */
int mom_solver_configure_metamaterial(
    mom_solver_t* solver,
    double unit_cell_size,
    double effective_er,
    double effective_mu
);

#ifdef __cplusplus
}
#endif

#endif // MOM_LAYERED_MEDIUM_H
