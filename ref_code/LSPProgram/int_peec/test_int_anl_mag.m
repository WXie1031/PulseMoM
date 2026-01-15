a1 = 0;
a2 = pi/2;
r1 = 0;
r2 = 1.13e-3;
r0 = 10e-3;
b0 = pi/6;

Sxytmp = [1e-3 1e-3 -1e-3 -1e-3];
Oxytmp = [r0*cos(b0) r0*sin(b)];
% 
% Sxytmp = [4e-3 4e-3 2e-3 2e-3];
% Oxytmp = [sqrt(3)*1e-3+3e-3 1e-3+3e-3]*2;

[Dxz,Dyz,Txx,Txy,Tyy] = int_anl_fila_mag(a1, a2, r1, r2, r0, b0, 1, 5,6);
[Dxz,Dyz,Txy,Txx,Tyy]

% [I31, I21, I23, I22, I33] = int_z_m_2d(Sxytmp, Oxytmp);
% [I21, I31, I23, I22, I33]

[Dxz2,Dyz2,Txx2,Txy2,Tyy2] = int_anl_fila_mag_old(a1, a2, r1, r2, r0, b0, 1, 5,6);
[Dxz2,Dyz2,Txy2,Txx2,Tyy2]

xs1 = Sxytmp(:,1);
xs2 = Sxytmp(:,3);
ys1 = Sxytmp(:,2);
ys2 = Sxytmp(:,4);
xf = Oxytmp(:,1);
yf = Oxytmp(:,2);
[Dx,Dy] = int_bar_fila_mag_p3d_D(xs1,xs2,ys1,ys2,xf,yf,1);
[Dx,Dy]
[Txx,Txy,Tyy]  = int_bar_fila_mag_p3d_T(xs1,xs2,ys1,ys2,xf,yf,1);
[Txy,Txx,Tyy] 
