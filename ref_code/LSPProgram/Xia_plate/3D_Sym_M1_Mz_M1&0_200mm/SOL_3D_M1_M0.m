% SOL_3D_M1_M0 Main program for solving Jx Jy Mx My Mz, in Ferro-magnetic plates and B field 
%              using METHOD M1&M0  (segment-match for KVL, point-match for M)
% Jx Jy Mx My  applying double exponential distribution along z-direction
% Mz in the edge blocks applying M0 method, other area applying M1 method
% Jx Jy Mz applying intensive grid in the edge blocks
% the width of edge band should be treated specially is about 2.4mm
%
% updated on 31/03/2011
%
function [Jx,Jy,Mx,My,Mz,Bx,By,Bz]=SOL_3D_M1_M0

clear all;
% initilization & material info.
f=50; mur=200; delt=0.75e+7; mu0=4e-7*pi;
aa=-(1+1i)*sqrt(pi*f*mu0*mur*delt);
% ******************************************************

% source lines
Qs=[-0.05 -0.05 0.0; 
    0.05 -0.05 0.0;
    0.05 0.05 0.0;
    -0.05 0.05 0.0]; 
Is=1;
% ******************************************************

% object points for magnetic field
uo=linspace(-0.2,0.2,51);
no=length(uo);
Io=ones(no,1);
Object=[uo' uo' Io*0.1];
% ******************************************************

% position and dimensions of plate
XYZ=[0.0 0.0 0.05];                  % coordinates of middle points of plates 
w=0.2; l=0.2; d=0.002;               % plate size
kz=1;                                % define the match position ('0-1':'middle-surface')
ko=[-1 -0.5 0 0.5 1];                % position for observing current density (-1:1,bottom:top)
%zs=d/2*[-1 0 1];                      % intervals for numerical integration 
zs=d/2*[-1 1];                      % intervals for numerical integration 
% ******************************************************

% mesh generation 
% global Nx Ny NXx NXy NYx NYy NZx NZy;
c=0;
N=[8 8];
Nx=N(1); Ny=N(2);          % mesh evenly for plate
du=w/Nx;
dv=l/Ny;
u=-w/2+(0:Nx)*du;          % x坐标数组
v=-l/2+(0:Ny)*dv;          % y坐标数组

% increase the mesh density in the edge blocks
le=0.0025;                             % the depth for abnormal distribution of variables
te=[0 0.3 0.55 0.75 0.9 1];            % uneven partition for edge blocks from inside to side
te=le*te;
ue=w/2-le+te;
ve=l/2-le+te;
ue=[ue(1)-0.0045 ue(1)-0.0025 ue(1)-0.001 ue];      % add several buffer grid
ve=[ve(1)-0.0045 ve(1)-0.0025 ve(1)-0.001 ve];
tz=[0 0.2 0.4 0.6 0.8 1];                      % partition for edge blocks along z direction
ez=d*tz;
ez=XYZ(3)-d/2+ez;

u0=[-fliplr(ue) u(3:Nx-1) ue];           % the total partition
v0=[-fliplr(ve) v(3:Ny-1) ve];
Nx0=length(u0)-1;
Ny0=length(v0)-1;
u=u0;
v=v0;
Nx=Nx0;
Ny=Ny0;
N=[Nx Ny];
%du=u(2:Nx0+1)-u(1:Nx0);
c=c+1;
figure(c);
plot(u0,ones(1,Nx0+1),'r*');

Ne=2;                                   % the number of grids applying M0 method 
uz=u0(Ne+1:Nx0-Ne+1);                   % the grid applying M1 for Mz (middle area) 
vz=v0(Ne+1:Ny0-Ne+1);
NZx=length(uz)-1;
NZy=length(vz)-1;

%ux=0.5*(u(1:Nx)+u(2:Nx+1));
ux=0.5*(u0(1:Nx0)+u0(2:Nx0+1));        % mesh for Jx cells
vx=v0;
NXx=length(ux)-1;
NXy=length(vx)-1;

uy=u0;                                 % mesh for Jy cells
%vy=0.5*(v(1:Ny)+v(2:Ny+1));
vy=0.5*(v0(1:Ny0)+v0(2:Ny0+1));
NYx=length(uy)-1;
NYy=length(vy)-1;

Nxc=Nx/2; Nyc=Ny/2;
NXxc=fix(NXx/2); NXyc=NXy/2;
NYxc=NYx/2; NYyc=fix(NYy/2);
NZxc=NZx/2; NZyc=NZy/2;

NN=Nxc*Nyc;
NX=(NXxc+1)*NXyc;
NY=NYxc*(NYyc+1);
NZ1=NZxc*NZyc;
           
u=u+XYZ(1); v=v+XYZ(2);                % transform coordinate
ux=ux+XYZ(1); vx=vx+XYZ(2);
uy=uy+XYZ(1); vy=vy+XYZ(2);
uz=uz+XYZ(1); vz=vz+XYZ(2);
zo1=XYZ(3)-kz*d/2;                     % lower object
zo2=XYZ(3)+kz*d/2;                     % upper object
zo=[zo1 zo2];
zs=zs+XYZ(3);

[Oxy Oxy_X Oxy_Y Oxy_Z Sxy Sxy_X Sxy_Y Sxy_Z]=MESHING_3D_1(u,v,ux,vx,uy,vy,uz,vz);
[Oxyz_Z0 Sxyz_Z0]=MESHING_3D_M0_1(u0,v0,ez,Ne);
%[Oxy Oxy_X Oxy_Y Oxy_Z Sxy Sxy_X Sxy_Y Sxy_Z]=MESHING_3D(u,v,ux,vx,uy,vy,uz,vz);          % coordinate matrix generation
%[Oxyz_Z0 Sxyz_Z0]=MESHING_3D_M0(u0,v0,ez,Ne);
[NZ0 def]=size(Oxyz_Z0);

[c]=Geoplot(u0,v0,ux,vx,uy,vy,Oxy_Z,Oxyz_Z0,zo,ez,c);
% Oxy(xo,yo),Sxy(x1,y1,x2,y2), for Mx My 
% Oxy_X(xo1,xo2,yo),Sxy_X(x1,y1,x2,y2), for Jx
% Oxy_Y(xo,yo1,yo2),Sxy_Y(x1,y1,x2,y2), for Jy
% Oxy_Z(xo,yo),Sxy_Y(x1,y1,x2,y2), for Mz applying M1 method
% Oxyz_Z0(xo,yo,zo),Sxyz_Z0(x1,y1,z1,x2,y2,z2), for Mz applying M0 method
% ******************************************************

% coefficient matrix for KCL and KVL
%[Coe_jx,Coe_jy,Coe_ux,Coe_uy]=Coef(NXxc+1,NXyc,NYxc,NYyc+1,1);
[Coe_jx,Coe_jy,Coe_ux,Coe_uy]=Coef(NXx,NXy,NYx,NYy,1);
%[Coe_jx,Coe_jy,Coe_ux,Coe_uy]=Coef(4,4,4,4,1);

% coefficient for object branches
ke=2/(1i*f*delt*mu0);                 % ke =1/(j*(2*pi*f)*delt*mu0/(4*pi))
kz1=sinh(0.5*aa*d*(1+kz))/sinh(aa*d);         
kz2=sinh(0.5*aa*d*(1-kz))/sinh(aa*d);

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
Ez=speye(NZ1);
Ez1=kz1*Ez; Ez2=kz2*Ez;       
EZ1=[Ez1 Ez2;
     Ez2 Ez1];

clear dx dy Ex Ex1 Ex2 Ey Ey1 Ey2 Em Em1 Em2 Ez Ez1 Ez2;
% *************************************************************************

% main program
% interaction between each current cells  
[Lx Ly Qx_y Qx_z Qy_x Qy_z Bx_y By_x Bz_x Bz_y Pmxx Pmxy Pmxz Pmyx Pmyy Pmyz Pmzx Pmzy Pmzz]=COEF_3D_PLATE_M1(Oxy,Oxy_X,Oxy_Y,Oxy_Z,Sxy,Sxy_X,Sxy_Y,Sxy_Z,zo,zs,aa,Nx,Ny,NXx,NXy,NYx,NYy,NZx,NZy);
Lx=Lx+ke*EX;
Ly=Ly+ke*EY;
Pmxx=Pmxx+(4*pi-km)*EM;
Pmyy=Pmyy+(4*pi-km)*EM;
Pmzz=Pmzz-km*EZ1;

[Qx_z10 Qy_z10 Bz_x01 Bz_y01 Pmxz10 Pmyz10 Pmzz10 Pmzx01 Pmzy01 Pmzz01 Pmzz00]=COEF_3D_PLATE_M0(Oxy,Oxy_X,Oxy_Y,Oxy_Z,Oxyz_Z0,Sxy,Sxy_X,Sxy_Y,Sxy_Z,Sxyz_Z0,zo,zs,aa,Nx,Ny,NXx,NXy,NYx,NYy,NZx,NZy);
EZ0=speye(NZ0);
Pmzz00=Pmzz00+(4*pi-km)*EZ0;
clear EX EY EM EZ1 EZ0;

% force by external sources
[Sx,Sy,Bxs,Bys,Bzs1,Bzs0]=COEF_3D_SOUR(Oxy,Oxy_X,Oxy_Y,Oxy_Z,Oxyz_Z0,zo,Qs,Is);

% solve the equation matrix
Z1=sparse(2*NX,2*NY); Z2=sparse(2*NX,2*NN); 
Z4=sparse(2*NY,2*NX); Z5=sparse(2*NY,2*NN); 
Z7=sparse(2*NN,2*NX); Z8=sparse(2*NN,2*NN);
Z9=sparse(2*NN,2*NY); 
Z12=sparse(2*NN,2*NN); Z13=sparse(2*NN,2*NN); 
Z14=sparse(2*NN,1);
Z15=sparse(2*NZ1,2*NN);
Z16=sparse(2*NN,2*NZ1);
Z17=sparse(NZ0,2*NN);
Z18=sparse(2*NN,NZ0);

Z3=sparse(NX,NN); Z6=sparse(NY,NN); 
Z10=sparse(NN,NX); Z11=sparse(NN,NY);
Z3=[Coe_ux Z3;Z3 Coe_ux]; Z6=[Coe_uy Z6;Z6 Coe_uy];
ku=-2/(1i*f*mu0);
Z3=ku*Z3; Z6=ku*Z6;
Z10=[Coe_jx Z10;Z10 Coe_jx]; Z11=[Coe_jy Z11;Z11 Coe_jy];
clear Coe_jx Coe_jy Coe_ux Coe_uy;

T=[Sx; Sy; Bxs; Bys; Bzs1; Bzs0; Z14];
T=-T;
clear Sx Sy Bxs Bys Bzs1 Bzs0 Z14;

G=[Lx Z1 Z2 Qx_y Qx_z Qx_z10 Z3;   
   Z4 Ly Qy_x Z5 Qy_z Qy_z10 Z6;
   Z7 Bx_y Pmxx Pmxy Pmxz Pmxz10 Z8;
   By_x Z9 Pmyx Pmyy Pmyz Pmyz10 Z8;
   Bz_x Bz_y Pmzx Pmzy Pmzz Pmzz10 Z15;
   Bz_x01 Bz_y01 Pmzx01 Pmzy01 Pmzz01 Pmzz00 Z17;
   Z10 Z11 Z12 Z12 Z16 Z18 Z13];

clear Z1 Z2 Z3 Z4 Z5 Z6 Z7 Z8 Z9 Z10 Z11 Z12 Z13 Z15 Z16 Z17 Z18;
clear Lx Ly Qx_y Qx_z Qy_x Qy_z Bx_y By_x Bz_x Bz_y Pmxx Pmxy Pmxz Pmyy Pmyz Pmzx Pmzy Pmzz;
clear Qx_z10 Qy_z10 Pmxz10 Pmyz10 Pmzz10 Bz_x01 Bz_y01 Pmzx01 Pmzy01 Pmzz01 Pmzz00;

Q=G\T;

clear G T;

JX1=Q(1:NX);
JX2=Q(NX+1:2*NX);
JY1=Q(2*NX+1:2*NX+NY);
JY2=Q(2*NX+NY+1:2*(NX+NY));
MX1=Q(2*(NX+NY)+1:2*(NX+NY)+NN);
MX2=Q(2*(NX+NY)+NN+1:2*(NX+NY+NN));
MY1=Q(2*(NX+NY+NN)+1:2*(NX+NY)+3*NN);
MY2=Q(2*(NX+NY)+3*NN+1:2*(NX+NY)+4*NN);
MZ1=Q(2*(NX+NY)+4*NN+1:2*(NX+NY)+4*NN+NZ1);
MZ2=Q(2*(NX+NY)+4*NN+NZ1+1:2*(NX+NY)+4*NN+2*NZ1);
MZ0=Q(2*(NX+NY)+4*NN+2*NZ1+1:2*(NX+NY)+4*NN+2*NZ1+NZ0);
%ku=-(1i*f*mu0)/2;
%U=ku*U;

% the solutions matrix of Jx Jy Mx My Mz
for i=1:Nyc    
    Mx1(i,:)=MX1((i-1)*Nxc+1:i*Nxc);
    Mx2(i,:)=MX2((i-1)*Nxc+1:i*Nxc);
    My1(i,:)=MY1((i-1)*Nxc+1:i*Nxc);
    My2(i,:)=MY2((i-1)*Nxc+1:i*Nxc);    
end
Mx1=[Mx1 -fliplr(Mx1);
    flipud(Mx1) -rot90(Mx1,2)];
Mx2=[Mx2 -fliplr(Mx2);
    flipud(Mx2) -rot90(Mx2,2)];
for i=1:NXyc
    Jx1(i,:)=JX1((i-1)*NXxc+1:i*NXxc);           % on the bottom ('1')
    Jx2(i,:)=JX2((i-1)*NXxc+1:i*NXxc);           % on the top ('2')    
end
Jx1=[Jx1 JX1(end-NXyc+1:end) fliplr(Jx1);
    -flipud(Jx1) -flipud(JX1(end-NXyc+1:end)) -rot90(Jx1,2)];
Jx2=[Jx2 JX2(end-NXyc+1:end) fliplr(Jx2);
    -flipud(Jx2) -flipud(JX2(end-NXyc+1:end)) -rot90(Jx2,2)];
for i=1:NYyc
    Jy1(i,:)=JY1((i-1)*NYxc+1:i*NYxc);             
    Jy2(i,:)=JY2((i-1)*NYxc+1:i*NYxc);
end
Jy1=[Jy1 -fliplr(Jy1); 
    JY1(end-NYxc+1:end)' -fliplr(JY1(end-NYxc+1:end)');
    flipud(Jy1) -rot90(Jy1,2)];
Jy2=[Jy2 -fliplr(Jy2); 
    JY2(end-NYxc+1:end)' -fliplr(JY2(end-NYxc+1:end)');
    flipud(Jy2) -rot90(Jy2,2)];
for i=1:NZyc   
    Mz1(i,:)=MZ1((i-1)*NZxc+1:i*NZxc);
    Mz2(i,:)=MZ2((i-1)*NZxc+1:i*NZxc);
end
Mz1=[Mz1 fliplr(Mz1);
    flipud(Mz1) rot90(Mz1,2)];
Mz2=[Mz2 fliplr(Mz2);
    flipud(Mz2) rot90(Mz2,2)];

% the solutions on the objective lines
No=length(ko);
ko1=sinh(0.5*aa*d*(1-ko))/sinh(aa*d);          % ko (-1:1,bottom:top)
ko2=sinh(0.5*aa*d*(1+ko))/sinh(aa*d);
for i=1:No
    Jxo=ko1(i)*Jx1+ko2(i)*Jx2;                           % on the observation ('o') plane
    Jx(:,i)=Jxo(:,fix(NXx/2)+1);
    Jyo=ko1(i)*Jy1+ko2(i)*Jy2;
    Jy(:,i)=Jyo(fix(NYy/2)+1,:);
    Mxo=ko1(i)*Mx1+ko2(i)*Mx2;
    Mx(:,i)=Mxo(Ny/2,:);
    Myo=ko1(i)*My1+ko2(i)*My2;
    My(:,i)=Myo(:,Nx/2);
    Mzo=ko1(i)*Mz1+ko2(i)*Mz2;
    Mz(:,i)=Mzo(NZy/2,:);
end
Jx=full(Jx);
Jy=full(Jy);
Mx=full(Mx);
My=full(My);
Mz=full(Mz);
% *************************************************************************

% draw the solution lines
c=c+1;
figure(c);
for i=1:No
    y=1:NYx;
    Jxo=Jx(:,i);
    subplot(2,3,i);    
    plot(y,abs(Jxo(y)),'r + -',y,real(Jxo(y)),'g o -',y,imag(Jxo(y)),'b * -');
end
c=c+1;
figure(c);
for i=1:No
    x=1:NYx;
    Jyo=Jy(:,i);
    subplot(2,3,i); 
    plot(x,abs(Jyo(x)),'r + -',x,real(Jyo(x)),'g o -',x,imag(Jyo(x)),'b * -');
end
c=c+1;
figure(c);
for i=1:No
    x=1:Nx;
    Mxo=Mx(:,i);
    subplot(2,3,i);
    plot(x,abs(Mxo(x)),'r + -',x,real(Mxo(x)),'g o -',x,imag(Mxo(x)),'b * -');
end
c=c+1;
figure(c);
for i=1:No
    y=1:Ny;
    Myo=Mx(:,i);
    subplot(2,3,i);   
    plot(y,abs(Myo(y)),'r + -',y,real(Myo(y)),'g o -',y,imag(Myo(y)),'b * -');
end
c=c+1;
figure(c);
for i=1:No
    x=1:NZx;
    Mzo=Mz(:,i);
    subplot(2,3,i);
    plot(x,abs(Mzo(x)),'r + -',x,real(Mzo(x)),'g o -',x,imag(Mzo(x)),'b * -');
end
% *************************************************************************

% calculate the magnetic fields
% [Bx_y By_x Bz_x Bz_y Pmxx Pmxy Pmxz Pmyy Pmyz Pmzx Pmzy Pmzz]=BFCAL_M1(Object,Sxy,Sxy_X,Sxy_Y,Sxy_Z,zs,aa);
[Bx_y By_x Bz_x Bz_y Pmxx Pmxy Pmxz Pmyx Pmyy Pmyz Pmzx Pmzy Pmzz]=BFCAL_M1(Object,Sxy,Sxy_X,Sxy_Y,Sxy_Z,zs,aa,N,NZx,NZy);
[Pmxz0 Pmyz0 Pmzz0]=BFCAL_M0(Object,Sxyz_Z0);

[Bxs Bys Bzs]=BFSCAL(Object,Qs,Is);

Bx=Bx_y*[JY1;JY2]+Pmxx*[MX1;MX2]+Pmxy*[MY1;MY2]+Pmxz*[MZ1;MZ2]+Pmxz0*MZ0+Bxs;
By=By_x*[JX1;JX2]+Pmyx*[MX1;MX2]+Pmyy*[MY1;MY2]+Pmyz*[MZ1;MZ2]+Pmyz0*MZ0+Bys;
Bz=Bz_x*[JX1;JX2]+Bz_y*[JY1;JY2]+Pmzx*[MX1;MX2]+Pmzy*[MY1;MY2]+Pmzz*[MZ1;MZ2]+Pmzz0*MZ0+Bzs;

%Bx=Bx_y*[JY1;JY2]+Pmxx*[MX1;MX2]+Pmxy*[MY1;MY2]+Bxs;
%By=By_x*[JX1;JX2]+Pmyx*[MX1;MX2]+Pmyy*[MY1;MY2]+Bys;
%Bz=Bz_x*[JX1;JX2]+Bz_y*[JY1;JY2]+Pmzx*[MX1;MX2]+Pmzy*[MY1;MY2]+Bzs;
k0=1e-7;
Bx=k0*Bx;
By=k0*By;
Bz=k0*Bz;
% *************************************************************************

No=1:no;
c=c+1;
figure(c);
plot(No,abs(Bx(No)),'r + -',No,real(Bx(No)),'g o -',No,imag(Bx(No)),'b * -');
c=c+1;
figure(c);
plot(No,abs(By(No)),'r + -',No,real(By(No)),'g o -',No,imag(By(No)),'b * -');
c=c+1;
figure(c);
plot(No,abs(Bz(No)),'r + -',No,real(Bz(No)),'g o -',No,imag(Bz(No)),'b * -');


end

