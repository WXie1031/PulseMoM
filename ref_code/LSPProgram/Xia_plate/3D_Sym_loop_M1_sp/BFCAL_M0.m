% calculate the affect on the objective line from Mz in the edge blocks 
%
% updated on 31/03/2011
%
function [Pmxz0 Pmyz0 Pmzz0]=BFCAL_M0(Oxyz,Sxyz_Z0)

% direct integration for solving Pmxz Pmyz Pmzz
[I16]=FM_3D_16_xz(Oxyz,Sxyz_Z0);
Pmxz0=I16;

[I15]=FM_3D_15_yz(Oxyz,Sxyz_Z0);
Pmyz0=I15;

[I13]=FM_3D_13_0(Oxyz,Sxyz_Z0);
Pmzz0=I13;

clear I16 I15 I13;

end