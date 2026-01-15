

% [DxzTmp, DyzTmp, TxxTmp, TxyTmp, TyyTmp] = int_anl_mag_p3d( ...
% 0, 0.2618, 0.7798e-3, 1e-3, 0.8899e-3, 0.1309, 0.1, 5, 6);
% [DxzTmp, DyzTmp] ,[ TxxTmp, TxyTmp, TyyTmp]
% 
% 
% [DxzTmp, DyzTmp, TxxTmp, TxyTmp, TyyTmp] = int_anl_mag_p3d( ...
% 0.2618, 0.5236, 0.7798e-3, 1e-3, 0.8899e-3, 0.3927, 0.1, 5, 6);
% [DxzTmp, DyzTmp], [TxxTmp, TxyTmp, TyyTmp]



r0 = 0.0089;
b0 = 3.1416;
a1 = 4.6441;
a2 = 4.9173;
r1 = 0.0086;
r2 = 0.0093;
l = 1;
rp = r0;
beta = b0;
x0 = rp*cos(beta);
y0 = rp*sin(beta);
funtxx = @(x,y) (2*(x-x0).^2.*(2*l.^2+(x-x0).^2+(y-y0).^2)./...
    (((x-x0).^2+(y-y0).^2).^2.*sqrt(l.^2+(x-x0).^2+(y-y0).^2)));
polarfuntxx = @(theta,r) funtxx(r.*cos(theta),r.*sin(theta)).*r;
Txxtmp = int_quad_num(funtxx,a1,a2,r1,r2,'Sector',true)





a1 = 0;
a2 = 2*pi;
r1 = 0;
r2 = 1.13e-3;
r0 =  2;
b0 = 0.;
len=10;

Sxytmp = [1e-3 1e-3 -1e-3 -1e-3]/2;
Oxytmp = [r0*1e-3 0];

% 
% Sxytmp = [4e-3 4e-3 2e-3 2e-3];
% Oxytmp = [sqrt(3)*1e-3+3e-3 1e-3+3e-3]*2;
% [Dxz,Dyz,Txx,Txy,Tyy] = int_anl_fila_mag(a1, a2, r1, r2, r0, b0, len, 5,6);
% [Dxz,Dyz,Txy,Txx,Tyy]/len
[Dxz,Dyz,Txx,Txy,Tyy] = int_anl_mag_p3d(a1, a2, r1, r2, r0, b0, len, 6,4);
[Dxz,Dyz,Txy,Txx,Tyy]/len

[Dxz,Dyz,Txx,Txy,Tyy] = int_anl_mag_p3d_integral2(a1, a2, r1, r2, r0, b0, l);
[Dxz,Dyz,Txy,Txx,Tyy]/len

[I31, I21, I23, I22, I33] = int_z_m_2d(Sxytmp, Oxytmp);
[I21, I31, I23, I22, I33]





% r0 = 5.4666e-4;
% b0 = 3.1416;
% a1 = 3.6652;
% a2 = 3.9270;
% r1 = 7.7982e-4;
% r2 = 1e-3;
% l = 1;
% rp = r0;
% beta = b0;
% x0 = rp*cos(beta);
% y0 = rp*sin(beta);
% funtxx = @(x,y) (-2.*(x-x0).^2./((x-x0).^2+(y-y0).^2).^1.5+2*(x-x0).^2.*(2*l.^2+(x-x0).^2+(y-y0).^2)./...
%         ((x-x0).^2+(y-y0).^2).^2./sqrt(l.^2+(x-x0).^2+(y-y0).^2)+2./sqrt((x-x0).^2+(y-y0).^2)-...
%         2.*sqrt((x-x0).^2+(y-y0).^2+l^2)./((x-x0).^2+(y-y0).^2));
%     polarfuntxx = @(theta,r) funtxx(r.*cos(theta),r.*sin(theta)).*r;
%     Txxtmp = int_quad_num(polarfuntxx,a1,a2,r1,r2,'Sector',true)








% 
% 
% [I31, I21, I23, I22, I33, Lint] = int_z_m_2d(Sxytmp, Oxytmp);
% 
% [DxzTmp, DyzTmp, TxxTmp, TxyTmp, TyyTmp] = int_anl_mag_p3d( ...
%     ang1, ang2, r1, r2, r0f(idcir_mag), a0f(idcir_mag), lgrp, 5, 6);
%                 
%                 
% E = eye(Ns,Ns);
% km = mu0*mur/(mur-1);      % km=mu0*mur/(mur-1)/(mu0/2*pi)
% 
% G22 =  cof*I22./dS(ik)-(E*km)./dS(ik);
% G33 =  cof*I33./dS(ik)-(E*km)./dS(ik);
% G21 = -cof*I21./dS(ik);
% G31 =  cof*I31./dS(ik);
% G23 =  cof*I23./dS(ik);
% % G32=G23;
% 
% G11 =  diag(Rs)+1j*w0*cof*Lint./dS(ik);
% G12 =  1j*w0*cof*I21./dS(ik);
% G13 = -1j*w0*cof*I31./dS(ik);
% 
% Gtmp2 = G23/G22;
% Gtmp3 = G23/G33;
% GI = G11 + G12/(Gtmp3*G23-G22)*(G21-G31-Gtmp3*G31) ...
%     + G13/(Gtmp2*G23-G33)*(G31-Gtmp2*G21);


