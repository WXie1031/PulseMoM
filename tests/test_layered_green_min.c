#include <stdio.h>
#include <math.h>
#include "../src/core/electromagnetic_kernels.h"

int main(){
    double f0 = 1e9;
    double k0 = 2.0 * M_PI * f0 / C0;
    layered_media_t layers[2];
    layers[0].permittivity = complex_one;
    layers[0].permeability = complex_one;
    layers[0].impedance = complex_real(ETA0);
    layers[0].thickness = 0.005;
    layers[0].conductivity = 0.0;
    layers[1].permittivity = complex_one;
    layers[1].permeability = complex_one;
    layers[1].impedance = complex_real(ETA0);
    layers[1].thickness = 0.005;
    layers[1].conductivity = 5.0e7;
    double rho = 0.02;
    double z_same = 0.003, zp_same = 0.002;
    double z_cross = 0.007, zp_cross = 0.002;
    complex_t Gfs = green_function_free_space(rho, k0);
    double base_mag = sqrt(Gfs.re*Gfs.re + Gfs.im*Gfs.im);
    complex_t G_same = green_function_layered_media(rho, z_same, zp_same, k0, 2, layers);
    complex_t G_cross = green_function_layered_media(rho, z_cross, zp_cross, k0, 2, layers);
    double mag_same = sqrt(G_same.re*G_same.re + G_same.im*G_same.im);
    double mag_cross = sqrt(G_cross.re*G_cross.re + G_cross.im*G_cross.im);
    double r_same = base_mag>1e-15 ? mag_same/base_mag : 0.0;
    double r_cross = base_mag>1e-15 ? mag_cross/base_mag : 0.0;
    if(!(r_same>=0.0 && r_same<1.6)){
        printf("FAIL same-layer ratio %.3f\n", r_same);
        return 1;
    }
    if(!(r_cross>=0.0 && r_cross<1.3)){
        printf("FAIL cross-layer ratio %.3f\n", r_cross);
        return 1;
    }
    layers[1].conductivity = 1.0e8;
    complex_t G_cross2 = green_function_layered_media(rho, z_cross, zp_cross, k0, 2, layers);
    double mag_cross2 = sqrt(G_cross2.re*G_cross2.re + G_cross2.im*G_cross2.im);
    if(!(mag_cross2 <= mag_cross + 1e-12)){
        printf("FAIL conductivity trend %.6e -> %.6e\n", mag_cross, mag_cross2);
        return 1;
    }
    printf("PASS layered green min test\n");
    return 0;
}
