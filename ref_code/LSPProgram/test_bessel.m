

r_out = [4.3;  1.18;]*1e-3; 
r_in = [4 , 0]*1e-3;

sig = [8382636.95892901;40461101.6744624];
l0 = [1;1];

f0 = 10e3;

 [R, Lin] = resis_induct_cir_ac(r_out, r_in, sig, l0, f0);
 R*1e3
 Lin*1e6
 
 
 r_out = 0.0043; r_in = 0.0040; sig = 8.3826e+06; l0 = 1;
 
 resis_induct_cir_ac(r_out, r_in, sig, l0, f0)
 
 Zcir = Z_IN_CIR(r_out, r_in, sig, l0, f0);
 R = real(Zcir)
 Lin = imag(Zcir)./(2*pi*f0)*1e6
 
 [Rw Lw]=IMP_WIREINT_M_S(len(ik),dim1(ik),f0*1e-6,Sig_pul(ik))
 
 resis_induct_cir_ac(dim1(ik), dim2(ik), Sig_pul(ik), len(ik), f0)
 
  resis_induct_cir_ac(dim1(ik), 0, Sig_pul(ik), len(ik), f0)
  
 [Rmesh2D_pul, Lmesh2D_pul] = para_group_mesh2d( ...
    shape(ik), [0 0], dim1(ik), dim2(ik), R_pul(ik), Lin_pul(ik))


 

