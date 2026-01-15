% SOL_Sym_loop_M1
% Main program for solving Jx Jy Mx My Mz, in Ferro-magnetic plates and B field 
%              using METHOD M1&M0  (segment-match for KVL, point-match for M)
% Jx Jy Mx My  applying double exponential distribution along z-direction
% Mz in the edge zones applying M0 method, other area applying M1 method
% Mesh refinement can be applied for edge zones
%
% updated on 15/05/2012
%
function [Jx,Jy,Mx,My,Mz,Bx,By,Bz, Z,R,L]=SOL_Sym_loop_M1

clear all;
ts=clock();
% initilization & material info.
f=50; mur=170; delt=0.60e+7; mu0=4e-7*pi;
aa=-(1+1i)*sqrt(pi*f*mu0*mur*delt);
% ******************************************************

% observation lines for magnetic field
% 1. Compare with experiment
upo=linspace(-0.6,0.6,13);  % lines in middle plane
udo=linspace(-0.6,0.6,13);  % diagonal lines
vo=linspace(0,0.5,51);      % vertical line
npo=length(upo);            % points on a line
ndo=length(udo);
nvo=length(vo);
Ipo=ones(npo,1);
Ido=ones(ndo,1);
Ivo=ones(nvo,1);
Nup=7; Nud=4; Nv=1;         % No. of lines        
Object=[upo' Ipo*0  Ipo*0.280;
        upo' Ipo*0  Ipo*0.281;
        upo' Ipo*0  Ipo*0.282;
        upo' Ipo*0  Ipo*0.283;
        upo' Ipo*0  Ipo*0.284;
        upo' Ipo*0  Ipo*0.285;
        upo' Ipo*0  Ipo*0.286;
        udo' udo'   Ido*0.282;
        udo' udo'   Ido*0.283;
        udo' udo'   Ido*0.284;
        udo' udo'   Ido*0.285;
        Ivo*0 Ivo*0   vo'];

% 2. Compare with FEM_3D
% upo=linspace(-0.04,0.04,41);  % lines in middle plane
% udo=linspace(-0.04,0.04,41);  % diagonal lines
% vo=linspace(0,0.05,51);       % vertical line
% npo=length(upo);              % points on a line
% ndo=length(udo);
% nvo=length(vo);
% Ipo=ones(npo,1);
% Ido=ones(ndo,1);
% Ivo=ones(nvo,1);
% Nup=7; Nud=4; Nv=1;         % No. of lines        
% Object=[upo' Ipo*0  Ipo*0.025;
%         upo' Ipo*0  Ipo*0.03;
%         upo' Ipo*0  Ipo*0.035;
%         upo' Ipo*0  Ipo*0.04;
%         upo' Ipo*0  Ipo*0.045;
%         upo' Ipo*0  Ipo*0.05;
%         upo' Ipo*0  Ipo*0.055;
%         udo' udo'   Ido*0.03;
%         udo' udo'   Ido*0.04;
%         udo' udo'   Ido*0.05;
%         udo' udo'   Ido*0.06;
%         Ivo*0 Ivo*0   vo'];
 
% 3. Compare with FEM_2D
% upo=linspace(-0.5,0.5,41);  % lines in middle plane
% udo=linspace(-0.5,0.5,41);  % diagonal lines
% vo=linspace(0,0.5,51);      % vertical line
% npo=length(upo);            % points on a line
% ndo=length(udo);
% nvo=length(vo);
% Ipo=ones(npo,1);
% Ido=ones(ndo,1);
% Ivo=ones(nvo,1);
% Nup=7; Nud=4; Nv=1;         % No. of lines        
% Object=[upo' Ipo*0  Ipo*0.12;
%         upo' Ipo*0  Ipo*0.13;
%         upo' Ipo*0  Ipo*0.14;
%         upo' Ipo*0  Ipo*0.15;
%         upo' Ipo*0  Ipo*0.16;
%         upo' Ipo*0  Ipo*0.17;
%         upo' Ipo*0  Ipo*0.18;
%         udo' udo'   Ido*0.13;
%         udo' udo'   Ido*0.14;
%         udo' udo'   Ido*0.15;
%         udo' udo'   Ido*0.16;
%         Ivo*0 Ivo*0   vo'];
% ******************************************************

% Layout of source lines
% Qs=[x1 x2 y z;     
%     x y1 y2 z;
%     x y z1 z2]

% "SP" *************************
% 1. compare with experiment
% Qs=[-0.05  0.05 -1.1  0;
%     -0.05  0.05  1.1  0;
%     -0.05 -1.1   1.1  0;
%      0.05 -1.1   1.1  0];
% Is=[1 -1 -1 1]*100;           % [Ix.. Iy.. Iz..]
% Ns=[2 2 0];               % [Nsx Nsy Nsz]

% 2. compare with FEM_3D
% Qs=[-0.01  0.01 -0.01  0;
%     -0.01  0.01  0.01  0;
%     -0.01 -0.01  0.01  0;
%      0.01 -0.01  0.01  0];
% Is=[1 -1 -1 1];           % [Ix.. Iy.. Iz..]
% Ns=[2 2 0];               % [Nsx Nsy Nsz]

% 3. compare with FEM_2D
% Qs=[-0.1  0.1 -2  0;
%     -0.1  0.1  2  0;
%     -0.1 -2    2  0;
%      0.1 -2    2  0];
% Is=[1 -1 -1 1];           % [Ix.. Iy.. Iz..]
% Ns=[2 2 0];               % [Nsx Nsy Nsz]

% "SL" *************************
% compare with experiment
Qs=[-0.05  0.05 -0.5 -0.5;   % x
    -0.05  0.05  0.5 -0.5;
    -0.05 -0.5   0.5  0;     % y
     0.05 -0.5   0.5  0;
    -0.05 -0.5  -0.5  0;     % z
     0.05 -0.5  -0.5  0;
    -0.05  0.5  -0.5  0;
     0.05  0.5  -0.5  0    ];
Is=[1 -1 -1 1 -1 1 1 -1]*100;           % [Ix.. Iy.. Iz..]
Ns=[2 2 4];               % [Nsx Nsy Nsz]
% ******************************************************

% Calculating the magnetic field excited by external sources
[BSX,BSY,BSZ]=Cal_3D_Bs(Object,Qs,Is,Ns);
k0=1e-1;
BSX=k0*BSX;
BSY=k0*BSY;
BSZ=k0*BSZ;

% position and dimensions of plate
Pl=[0 0 0.1];
Ps=[1 4 0.002];           % plate size with FEM_2D
% Pl=[0 0 0.136];         % coordinates of middle points of plates [X0 Y0 Z0]
% Ps=[1.22 2.44 0.002];   % plate size [w l d], for experiment
N=[10 40];               % divide number along x, y, z [Nx Ny Nz]
Ne=0;                     % grids for Mz applying M0
p=0;                      % No. of figures
% ******************************************************

% mesh generation 
kz=1;                     % define the match position ('0-1':'middle-surface')
dd=Ps(3);  Z0=Pl(3);
zs=dd/2*[-1 1];           % intervals for numerical integration 
zs=zs+Z0;
zo1=Z0-kz*dd/2;           % lower object
zo2=Z0+kz*dd/2;           % upper object
zo=[zo1 zo2];

[Oxy Oxy_X Oxy_Y Oxy_Z Sxy Sxy_X Sxy_Y Sxy_Z u v u0 v0]=MESHING_3D_M1_0(Pl,Ps,N,Ne,0);
% Oxy(xo,yo),Sxy(x1,x2,y1,y2), for Mx My 
% Oxy_X(xo1,xo2,yo),Sxy_X(x1,x2,y1,y2), for Jx
% Oxy_Y(xo,yo1,yo2),Sxy_Y(x1,x2,y1,y2), for Jy
% Oxy_Z(xo,yo),Sxy_Z(x1,x2,y1,y2), for Mz applying M1 method
Nx=length(u)-1;
Ny=length(v)-1; 
N=[Nx Ny];

XNx=Nx-1; YNy=Ny-1;
Nxc=Nx/2; Nyc=Ny/2;
XNxc=Nxc-1; YNyc=Nyc-1;

NN=Nxc*Nyc;
NX=(XNxc+1)*Nyc;
NY=Nxc*(YNyc+1);

p=p+1;
figure(p);
plot(u,ones(1,Nx+1),'r*');
% [NZ0 def]=size(Oxyz_Z0);
% [p]=Geoplot(u,v,ux,vx,uy,vy,Oxy_Z,Oxyz_Z0,zo,ez,p);
% ******************************************************

% coefficient for transfer Jx Jy to loop I
[Coex,Coey]=Coef_Sym_loop(u,v,N);

% coefficient for object branches
ke=2/(1i*f*delt*mu0);                 % ke =1/(j*(2*pi*f)*delt*mu0/(4*pi))
kz1=sinh(0.5*aa*dd*(1+kz))/sinh(aa*dd);         
kz2=sinh(0.5*aa*dd*(1-kz))/sinh(aa*dd);

dx=Oxy_X(:,2)-Oxy_X(:,1);
dy=Oxy_Y(:,3)-Oxy_Y(:,2);
Ex=sparse(1:NX,1:NX,dx);
Ex1=kz1*Ex; Ex2=kz2*Ex;
EX=[Ex1 Ex2; 
    Ex2 Ex1];
Ey=sparse(1:NY,1:NY,dy);
Ey1=kz1*Ey; Ey2=kz2*Ey;
EY=[Ey1 Ey2;
    Ey2 Ey1];

km=4*pi*mur/(mur-1);                 % mur=200,km=mu0*mur/(mur-1)/(mu0/4*pi)      
Em=speye(NN);
Em1=kz1*Em; Em2=kz2*Em;       
EM=[Em1 Em2;
    Em2 Em1];
Ez=speye(NN);
Ez1=kz1*Ez; Ez2=kz2*Ez;       
EZ1=[Ez1 Ez2;
     Ez2 Ez1];
clear dx dy Ex Ex1 Ex2 Ey Ey1 Ey2 Em Em1 Em2 Ez Ez1 Ez2;
% *************************************************************************

% main program
% interaction between each current cells  
t0=clock();
[Lx Ly Qx_y Qx_z Qy_x Qy_z Bx_y By_x Bz_x Bz_y Pmxx Pmxy Pmxz Pmyx Pmyy Pmyz Pmzx Pmzy Pmzz]=COEF_3D_PLATE_M1(Oxy,Oxy_X,Oxy_Y,Oxy_Z,Sxy,Sxy_X,Sxy_Y,Sxy_Z,zo,zs,aa,N,Ne);
t1=clock();
t1=t1-t0;      % time for calculating coefficient matrixes
Lx=Lx+ke*EX;
Ly=Ly+ke*EY;
Pxx=Pmxx+(4*pi-km)*EM;
Pyy=Pmyy+(4*pi-km)*EM;
Pzz=Pmzz-km*EZ1;
Pxy=Pmxy; Pxz=Pmxz;
Pyx=Pmyx; Pyz=Pmyz;
Pzx=Pmzx; Pzy=Pmzy;
clear Pmxx Pmxy Pmxz Pmyx Pmyy Pmyz Pmzx Pmzy Pmzz;
% [Qx_z10 Qy_z10 Bz_x01 Bz_y01 Pmxz10 Pmyz10 Pmzz10 Pmzx01 Pmzy01 Pmzz01 Pmzz00]=COEF_3D_PLATE_M0(Oxy,Oxy_X,Oxy_Y,Oxy_Z,Oxyz_Z0,Sxy,Sxy_X,Sxy_Y,Sxy_Z,Sxyz_Z0,zo,zs,aa,Nx,Ny,XNx,NXy,NYx,YNy,Nx,Ny);
% EZ0=speye(NZ0);
% Pmzz00=Pmzz00+(4*pi-km)*EZ0;
clear EX EY EM EZ1 EZ0;

% force by external sources
t2=clock();
[Sx,Sy,Bxs,Bys,Bzs]=COEF_3D_SOUR(Oxy,Oxy_X,Oxy_Y,zo,Qs,Is,Ns);
t3=clock();
t2=t3-t2;

[L,Qx,Qy,Qz,Bx,By,Bz,S]=Trans_loop(Lx,Qx_y,Qx_z,Ly,Qy_x,Qy_z,Bx_y,By_x,Bz_x,Bz_y,Sx,Sy,Coex,Coey,N);

clear Lx Ly Qx_y Qx_z Qy_x Qy_z Bx_y By_x Bz_x Bz_y;

T=[S; Bxs; Bys; Bzs];
T=-T;
clear Sx Sy Bxs Bys Bzs;

 % ke =1/(j*(2*pi*f)*delt*mu0/(4*pi))
 
G=[L   Qx   Qy   Qz;      
   Bx  Pxx  Pxy  Pxz;
   By  Pyx  Pyy  Pyz;
   Bz  Pzx  Pzy  Pzz];

Pinv = inv([Pxx Pxy Pxz;
            Pyx Pyy Pyz;
            Pzx Pzy Pzz;]);

Bm = [Bx;By;Bz];
Qm = [Qx Qy Qz;];

Z = (L-Qm*Pinv*Bm);
R = real(Z);
L = imag(Z)/(2*pi*f);

t3=clock();
Q=G\T;
t4=clock();
t3=t4-t3;
td=clock();
tt=td-ts;
clear G T;

I1=Q(1:NN);
I2=Q(NN+1:2*NN);
MX1=Q(2*NN+1:3*NN); 
MX2=Q(3*NN+1:4*NN);
MY1=Q(4*NN+1:5*NN);
MY2=Q(5*NN+1:6*NN);
MZ1=Q(6*NN+1:7*NN);
MZ2=Q(7*NN+1:8*NN);

JX1=Coex*I1; JX2=Coex*I2;
JY1=Coey*I1; JY2=Coey*I2;

% the solutions matrix of Jx Jy Mx My Mz
[Jx1 Jx2 Jy1 Jy2 Mx1 Mx2 My1 My2 Mz1 Mz2]=Get_results(JX1,JX2,JY1,JY2,MX1,MX2,MY1,MY2,MZ1,MZ2,N,1);

% the solutions on the objective lines
ko=[-1 -0.5 0 0.5 1];     % position for observing current density (-1:1,bottom:top)
No=length(ko);
ko1=sinh(0.5*aa*dd*(1-ko))/sinh(aa*dd);          % ko (-1:1,bottom:top)
ko2=sinh(0.5*aa*dd*(1+ko))/sinh(aa*dd);
for i=1:No
    Jxo=ko1(i)*Jx1+ko2(i)*Jx2;                           % on the observation ('o') plane
    Jx(:,i)=Jxo(:,fix(XNx/2)+1);
    Jyo=ko1(i)*Jy1+ko2(i)*Jy2;
    Jy(:,i)=Jyo(fix(YNy/2)+1,:);
    Mxo=ko1(i)*Mx1+ko2(i)*Mx2;
    Mx(:,i)=Mxo(Ny/2,:);
    Myo=ko1(i)*My1+ko2(i)*My2;
    My(:,i)=Myo(:,Nx/2);
    Mzo=ko1(i)*Mz1+ko2(i)*Mz2;
    Mz(:,i)=Mzo(Ny/2,:);
end
Jx=full(Jx);
Jy=full(Jy);
Mx=full(Mx);
My=full(My);
Mz=full(Mz);
Jx_mag=abs(Jx); Jy_mag=abs(Jy);
Mx_mag=abs(Mx); My_mag=abs(My); Mz_mag=abs(Mz);
Jy1_mag=abs(Jy1); Jy2_mag=abs(Jy2);
Mx1_mag=abs(Mx1); Mx2_mag=abs(Mx2);
% *************************************************************************

% draw the solution lines
u0=u0'; v0=v0'; 
p=p+1;
figure(p);
for i=1:No   
    Jxo=Jx(:,i);
    subplot(2,3,i);    
    plot(v0,abs(Jxo),'r + -',v0,real(Jxo),'g o -',v0,imag(Jxo),'b * -');
end
p=p+1;
figure(p);
for i=1:No
    Jyo=Jy(:,i);
    subplot(2,3,i); 
    plot(u0,abs(Jyo),'r + -',u0,real(Jyo),'g o -',u0,imag(Jyo),'b * -');
end
p=p+1;
figure(p);
for i=1:No
    Mxo=Mx(:,i);
    subplot(2,3,i);
    plot(u0,abs(Mxo),'r + -',u0,real(Mxo),'g o -',u0,imag(Mxo),'b * -');
end
p=p+1;
figure(p);
for i=1:No
    Myo=Mx(:,i);
    subplot(2,3,i);   
    plot(u0,abs(Myo),'r + -',u0,real(Myo),'g o -',u0,imag(Myo),'b * -');
end
p=p+1;
figure(p);
for i=1:No
    Mzo=Mz(:,i);
    subplot(2,3,i);
    plot(u0,abs(Mzo),'r + -',u0,real(Mzo),'g o -',u0,imag(Mzo),'b * -');
end
% *************************************************************************

% Calculate the magnetic field
[Bx_y By_x Bz_x Bz_y Pmxx Pmxy Pmxz Pmyx Pmyy Pmyz Pmzx Pmzy Pmzz]=BFCAL_M1(Object,Sxy,Sxy_X,Sxy_Y,Sxy_Z,zs,aa,N);
% [Pmxz0 Pmyz0 Pmzz0]=BFCAL_M0(Object,Sxyz_Z0);

Bx=Bx_y*[JY1;JY2]+Pmxx*[MX1;MX2]+Pmxy*[MY1;MY2]+Pmxz*[MZ1;MZ2];
By=By_x*[JX1;JX2]+Pmyx*[MX1;MX2]+Pmyy*[MY1;MY2]+Pmyz*[MZ1;MZ2];
Bz=Bz_x*[JX1;JX2]+Bz_y*[JY1;JY2]+Pmzx*[MX1;MX2]+Pmzy*[MY1;MY2]+Pmzz*[MZ1;MZ2];

Bxo=k0*Bx+BSX;
Byo=k0*By+BSY;
Bzo=k0*Bz+BSZ;

Bxp=Bxo(1:Nup*npo); Bxd=Bxo(Nup*npo+1:Nup*npo+Nud*ndo); Bxv=Bxo(Nup*npo+Nud*ndo+1:end);
Byp=Byo(1:Nup*npo); Byd=Byo(Nup*npo+1:Nup*npo+Nud*ndo); Byv=Byo(Nup*npo+Nud*ndo+1:end);
Bzp=Bzo(1:Nup*npo); Bzd=Bzo(Nup*npo+1:Nup*npo+Nud*ndo); Bzv=Bzo(Nup*npo+Nud*ndo+1:end);
for i=1:Nup
    Bpx(:,i)=Bxp((i-1)*npo+1:i*npo,:);
    Bpy(:,i)=Byp((i-1)*npo+1:i*npo,:);
    Bpz(:,i)=Bzp((i-1)*npo+1:i*npo,:);
end
for i=1:Nud
    Bdx(:,i)=Bxd((i-1)*ndo+1:i*ndo,:);
    Bdy(:,i)=Byd((i-1)*ndo+1:i*ndo,:);
    Bdz(:,i)=Bzd((i-1)*ndo+1:i*ndo,:);
end
for i=1:Nv
    Bvx(:,i)=Bxv((i-1)*nvo+1:i*nvo,:);
    Bvy(:,i)=Byv((i-1)*nvo+1:i*nvo,:);
    Bvz(:,i)=Bzv((i-1)*nvo+1:i*nvo,:);
end
Bpxm=abs(Bpx); Bpym=abs(Bpy); Bpzm=abs(Bpz);
Bdxm=abs(Bdx); Bdym=abs(Bdy); Bdzm=abs(Bdz);
Bvxm=abs(Bvx); Bvym=abs(Bvy); Bvzm=abs(Bvz);
Bp_mag=(Bpxm.^2+Bpym.^2+Bpzm.^2).^(1/2);
Bd_mag=(Bdxm.^2+Bdym.^2+Bdzm.^2).^(1/2);
Bv_mag=(Bvxm.^2+Bvym.^2+Bvzm.^2).^(1/2);
% *************************************************************************

% Bp
upo=upo'; udo=udo'; vo=vo';
p=p+1;
figure(p);
for i=1:Nup
    subplot(1,Nup,i);    
    plot(upo,Bpxm(:,i),'r + -',upo,Bpym(:,i),'g o -',upo,Bpzm(:,i),'b * -');
end

% Bd
p=p+1;
figure(p);
for i=1:Nud
    subplot(1,Nud,i);    
    plot(udo,Bdxm(:,i),'r + -',udo,Bdym(:,i),'g o -',udo,Bdzm(:,i),'b * -');
end

% Bv
p=p+1;
figure(p);
for i=1:Nv
    subplot(1,Nv,i);    
    plot(vo,Bvxm(:,i),'r + -',vo,Bvym(:,i),'g o -',vo,Bvzm(:,i),'b * -');
end

end
