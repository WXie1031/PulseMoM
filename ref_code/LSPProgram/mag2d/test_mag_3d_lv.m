a1 = 0;
a2 = 2*pi;
r1 = 0;
r2 = 1.13e-3;
r0 = 0.5e-3;
r0 = 3e-3;
b0 = 0;
len=1000;


% Sxy(idx,1) = wsum(1:Nw);
% Sxy(idx,3) = wsum(2:Nw+1);
% Sxy(idx,2) = tsum(ig);
% Sxy(idx,4) = tsum(ig+1);
Sxytmp = [-1e-3 -1e-3 1e-3 1e-3];
Oxytmp = [0.5*1e-3 0];
Oxytmp = [3*1e-3 0];
% 
% Sxytmp = [4e-3 4e-3 2e-3 2e-3];
% Oxytmp = [sqrt(3)*1e-3+3e-3 1e-3+3e-3]*2;
% [Dxz,Dyz,Txx,Txy,Tyy] = int_anl_fila_mag(a1, a2, r1, r2, r0, b0, 1, 5,6);
% [Dxz,Dyz,Txy,Txx,Tyy]
[Dxz,Dyz,Txx,Txy,Tyy] = int_anl_mag_p3d(a1, a2, r1, r2, r0, b0, len, 5,6);
[Dxz,Dyz,Txy,Txx,Tyy]/len


% dS(ik) = dW.*dT;
[I31, I21, I23, I22, I33] = int_z_m_2d(Sxytmp, Oxytmp);
[I21, I31, I23, I22, I33]


