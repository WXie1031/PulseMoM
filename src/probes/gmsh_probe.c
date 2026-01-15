#include <stdio.h>
#include "gmshc.h"
int main() {
    int ierr = 0;
    gmshInitialize(0, NULL, 0, 0, &ierr);
    if (ierr) { printf("Gmsh init failed\n"); return 1; }
    gmshFinalize(&ierr);
    printf("Gmsh probe OK\n");
    return 0;
}
