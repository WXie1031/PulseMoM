%COEF_2D_CELL_M0	 NUMERICAL INTEGRATION of 2D cell coefficients (2 fold)
%            cell-to-cell (source-to-obervation)
%            subscript "1" = observation, "2" = source (curr. dir. = U/u)
%
%            METHOD 0 (point-match)
%
%            constant J, Mx and My
%
% X1(2)      = lower and upper limits of a obj. cell (x dir.)
% X2(:,2)    = lower and upper limits of sour. cells (x dir.)
%
% G11 G12 G13   = coef.
% G21 G22 G23   = coef.
% G31 G32 G33   = coef.
% T1  T2  T3    = coef.
% Sxy=[x1a y1a x1b y1b; ...]  coorindates of source cells
% Oxy=[x1 y1; x2 y2; ...]  coorindates of observation points

% edited on 8 July 2009

function [I31, I21, I23, I22, I33, I11] = int_z_m_2d(Sxy, Oxy, r0)

% set r0 to be 0 at all time
if nargin<3
    r0=0;
end

[Ns , ~]=size(Sxy);
[No , ~]=size(Oxy);

I31=0; I21=0; I23=0; I22=0; I33=0; I11=0;

Ox = zeros(No,Ns);
Oy = zeros(No,Ns);
for i=1:Ns
    Ox(:,i)=Oxy(:,1);
    Oy(:,i)=Oxy(:,2);
end

Sxa = zeros(No,Ns);
Sxb = zeros(No,Ns);
Sya = zeros(No,Ns);
Syb = zeros(No,Ns);
for i=1:No
    Sxa(i,:) = Sxy(:,1);
    Sxb(i,:) = Sxy(:,3);
    Sya(i,:) = Sxy(:,2);
    Syb(i,:) = Sxy(:,4);
end

dx = Ox-Sxa;
dy = Oy-Sya;
[Ia,Ib,Ic,Idx,Idy,Ie] = int_z_m_2d_sub(dx,dy, r0);
I31 = I31+Ia;
I21 = I21+Ib; 
I23 = I23+Ic;
I22 = I22+Idx;
I33 = I33+Idy;
I11 = I11-Ie;

dx = Ox-Sxb;
dy = Oy-Syb;
[Ia,Ib,Ic,Idx,Idy,Ie] = int_z_m_2d_sub(dx,dy, r0);
I31 = I31+Ia;
I21 = I21+Ib;
I23 = I23+Ic;
I22 = I22+Idx;
I33 = I33+Idy;
I11 = I11-Ie;

dx = Ox-Sxa;
dy = Oy-Syb;
[Ia,Ib,Ic,Idx,Idy,Ie] = int_z_m_2d_sub(dx,dy, r0);
I31 = I31-Ia;
I21 = I21-Ib;
I23 = I23-Ic;
I22 = I22-Idx;
I33 = I33-Idy;
I11 = I11+Ie;

dx = Ox-Sxb;
dy = Oy-Sya;
[Ia,Ib,Ic,Idx,Idy,Ie] = int_z_m_2d_sub(dx,dy, r0);
I31 = I31-Ia;
I21 = I21-Ib;
I23 = I23-Ic;
I22 = I22-Idx;
I33 = I33-Idy;
I11 = I11+Ie;


end
