% SOL_3D_M0 Main program for solving Jx Jy Mx My Mz, in Ferro-magnetic plates and B field using 
%           METHOD 0 (segment-match for KVL, point-match for M)
% Grid generation evenly or edge refinement on x-y plane
% Nx,Ny,Nz,the number of grid along x,y,z direction respectively 
% Jx Jy Mx My Mz, the three-dimensional matrix of eddy current density and magnetization
%
% updated on 19/12/2011
%
function [Jx,Jy,Mx,My,Mz,Bx,By,Bz,Z]=SOL_3D_M0

%clear all;
% initilization & material info.
f=50; mur=200; delt=0.75e+7;      
mu0=4e-7*pi;
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
XYZ=[0.0 0.0 0.05];                   % coordinates of middle points of plates 
w=0.5; l=0.5; d=0.002;                % plate size
kz=0.5;                               % define the match position ('0-1':'bottom-top' of cell)
% ******************************************************

% mesh generation for plate
Nx=10; Ny=10;
du=w/Nx;
dv=l/Ny;
u=-w/2+(0:Nx)*du;                                 % x坐标数组
v=-l/2+(0:Ny)*dv;                                 % y坐标数组
%z=d/2*[-1 -0.5 0 0.5 1];                          % divide along z-direction
z=d/2*[-1 1]; 
Nz=length(z)-1;

% refinement
% ******************************************************
le=0.0025;                             % the depth for abnormal distribution of variables
te=[0 0.3 0.55 0.75 0.9 1];            % uneven partition for edge blocks from inside to side
te=le*te;
ue=w/2-le+te;
ve=l/2-le+te;
ue=[ue(1)-0.0045 ue(1)-0.0025 ue(1)-0.001 ue];      % add several buffer grid
ve=[ve(1)-0.0045 ve(1)-0.0025 ve(1)-0.001 ve];

u0=[-fliplr(ue) u(3:Nx-1) ue];           % the total partition
v0=[-fliplr(ve) v(3:Ny-1) ve];
Nx0=length(u0)-1;
Ny0=length(v0)-1;
u=u0;
v=v0;
Nx=Nx0;
Ny=Ny0;
% ******************************************************

ux=0.5*(u(1:Nx)+u(2:Nx+1));            % mesh for Jx cells
vx=v;
NXx=Nx-1;
NXy=Ny;

uy=u;                                  % mesh for Jy cells
vy=0.5*(v(1:Ny)+v(2:Ny+1));
NYx=Nx;
NYy=Ny-1;

uz=u;
vz=v;

X0=XYZ(1); Y0=XYZ(2); Z0=XYZ(3);                  % transform coordinate
u=u+X0; v=v+Y0; z=z+Z0;
ux=ux+X0; vx=vx+Y0;
uy=uy+X0; vy=vy+Y0;
uz=uz+X0; vz=vz+Y0;

NZx=Nx; NZy=Ny;
Nxc=Nx/2; Nyc=Ny/2;
NXxc=fix(NXx/2); NXyc=NXy/2;
NYxc=NYx/2; NYyc=fix(NYy/2);
NZxc=NZx/2; NZyc=NZy/2;

Nxy=Nxc*Nyc;
NXxy=NXxc*NXyc;
NYxy=NYxc*NYyc;
NN=Nxy*Nz;
NX=(NXxc+1)*NXyc*Nz;
NY=NYxc*(NYyc+1)*Nz;

[Oxyz Oxyz_X Oxyz_Y Oxyz_Z Sxyz Sxyz_X Sxyz_Y Sxyz_Z]=MESHING_3D_M0(u,v,ux,vx,uy,vy,uz,vz,z);
% ******************************************************

[Coe_jx,Coe_jy,Coe_ux,Coe_uy]=Coef(NXx,NXy,NYx,NYy,Nz);

% coefficient for object branches
ke=2/(1i*f*delt*mu0);            % ke =1/(j*(2*pi*f)*delt*mu0/(4*pi))
dx=Oxyz_X(:,2)-Oxyz_X(:,1);
dy=Oxyz_Y(:,3)-Oxyz_Y(:,2);
Ex=sparse(1:NX,1:NX,dx);
Ex=ke*Ex;
Ey=sparse(1:NY,1:NY,dy);
Ey=ke*Ey;

km=4*pi*mur/(mur-1);             % mur=200,km=mu0*mur/(mur-1)/(mu0/4*pi)
Em=speye(NN);
Em=Em*(4*pi-km);                 % integral correction
% ******************************************************

% coefficient matrix for Jx Jy Mx My Mz
[Lx Qx_y Qx_z Ly Qy_x Qy_z Bx_y By_x Bz_x Bz_y Pmxx Pmxy Pmxz Pmyx Pmyy Pmyz Pmzx Pmzy Pmzz]=COEF_3D_PLATE_M0(Oxyz,Oxyz_X,Oxyz_Y,Oxyz_Z,Sxyz,Sxyz_X,Sxyz_Y,Sxyz_Z,Nx,Ny,NXx,NXy,NYx,NYy,NZx,NZy,Nz);
Lx=Ex+Lx;
Ly=Ey+Ly;
Pmxx=Pmxx+Em;
Pmyy=Pmyy+Em;
Pmzz=Pmzz+Em;
clear dx dy Ex Ey Em;

% forced by source lines
[Sx,Sy,Bxs,Bys,Bzs]=COEF_3D_SOUR(Oxyz,Oxyz_X,Oxyz_Y,Qs,Is); 

Z1=sparse(NX, NY); Z2=sparse(NX, NN); 
Z3=sparse(NY, NX); Z4=sparse(NY, NN);
Z5=sparse(NN, NX); Z6=sparse(NN, NY);
Z7=sparse(NN,NN);
%Z8=sparse(NN,NN); Z9=sparse(NN,NN); 
Z10=sparse(NN,1);

G=[Lx Z1 Z2 Qx_y Qx_z Coe_ux;
   Z3 Ly Qy_x Z4 Qy_z Coe_uy;
   Z5 Bx_y Pmxx Pmxy Pmxz Z7;
   By_x Z6 Pmyx Pmyy Pmyz Z7;
   Bz_x Bz_y Pmzx Pmzy Pmzz Z7;
   Coe_jx Coe_jy Z7 Z7 Z7 Z7];

Pinv = inv([Pmxx Pmxy Pmxz;
            Pmyx Pmyy Pmyz;
            Pmzx Pmzy Pmzz;]);
Bmx = [Z5;By_x;Bz_x];
Bmy = [Bx_y;Z6;Bz_y];
Qmx = [Z2 Qx_y Qx_z];
Qmy = [Qy_x Z4 Qy_z];

Z = [Lx-Qmx*Pinv*Bmx      -Qmx*Pinv*Bmy;
       -Qmy*Pinv*Bmx    Ly-Qmy*Pinv*Bmy;];

clear Z1 Z2 Z3 Z4 Z5 Z6 Z7 Z8 Z9;
clear Coe_ux Coe_uy Coe_jx Coe_jy;
clear Lx Qx_y Qx_z Ly Qy_x Qy_z Bx_y By_x Bz_x Bz_y Pmxx Pmxy Pmxz Pmyx Pmyy Pmyz Pmzx Pmzy Pmzz;

T=[Sx; Sy; Bxs; Bys; Bzs; Z10];
T=-T;
clear Sx Sy Bxs Bys Bzs Z10;

Q=G\T;

clear G T;

JX=Q(1:NX);
JY=Q(NX+1:NX+NY);
MX=Q(NX+NY+1:NX+NY+NN);
MY=Q(NX+NY+NN+1:NX+NY+2*NN);
MZ=Q(NX+NY+2*NN+1:NX+NY+3*NN);
JX=full(JX); JY=full(JY);
MX=full(MX); MY=full(MY); MZ=full(MZ);

%U=Q(NX+NY+3*NN+1:NX+NY+3*NN+Np*Nz);
%ku=-(1i*f*mu0)/2;
%U=ku*U;

for j=1:Nz
    for i=1:NXyc
        Jx(i,:)=JX((j-1)*NXxy+(i-1)*NXxc+1:(j-1)*NXxy+i*NXxc);             % 3D matrix of Jx
        Jx_m(i,:)=JX(NXxy*Nz+(j-1)*NXyc+i);
    end
    Jx3(:,:,j)=[Jx Jx_m fliplr(Jx);
                -flipud(Jx) -flipud(Jx_m) -rot90(Jx,2)];
    for i=1:NYyc
        Jy(i,:)=JY((j-1)*NYxy+(i-1)*NYxc+1:(j-1)*NYxy+i*NYxc);            % 3D matrix of Jy
    end
    Jy_m=JY(NYxy*Nz+(j-1)*NYxc+1:NYxy*Nz+j*NYxc); 
    Jy3(:,:,j)=[Jy -fliplr(Jy); 
                Jy_m' -fliplr(Jy_m');
                flipud(Jy) -rot90(Jy,2)];
    for i=1:Nyc
        SN=(j-1)*Nxy+(i-1)*Nxc+1;
        EN=(j-1)*Nxy+i*Nxc;
        Mx(i,:)=MX(SN:EN);                       % 3D matrix of Mx
        My(i,:)=MY(SN:EN);                       % 3D matrix of My
        Mz(i,:)=MZ(SN:EN);                       % 3D matrix of Mz
    end    
    Mx3(:,:,j)=[Mx -fliplr(Mx);
                flipud(Mx) -rot90(Mx,2)];
    My3(:,:,j)=[My fliplr(My);
                -flipud(My) -rot90(My,2)];
    Mz3(:,:,j)=[Mz fliplr(Mz);
                flipud(Mz) rot90(Mz,2)];       
end

Jcx=Jx3(:,Nx/2,1);              % the middle line (x=0) on the bottom layer
Jcy=Jy3(Ny/2,:,1);              % the middle line (y=0) on the bottom layer
Mx1=Mx3(Ny/2,:,1);              % the middle line (y=0) on the bottom layer
Mx2=Mx3(Ny/2,:,Nz);             % the middle line (y=0) on the top layer
My=My3(:,Nx/2,1);
Mz=Mz3(:,Nx/2,1);

y=1:Ny;
figure(1);
plot(y,abs(Jcx(y)),'r + -',y,real(Jcx(y)),'g o -',y,imag(Jcx(y)),'b * -');

x=1:Nx;
figure(2);
plot(x,abs(Jcy(x)),'r + -',x,real(Jcy(x)),'g o -',x,imag(Jcy(x)),'b * -');
figure(3);
plot(x,abs(Mx1(x)),'r + -',x,real(Mx1(x)),'g o -',x,imag(Mx1(x)),'b * -');
figure(4);
plot(x,abs(Mx2(x)),'r + -',x,real(Mx2(x)),'g o -',x,imag(Mx2(x)),'b * -');

% calculate the magnetic fields
[Bx_y By_x Bz_x Bz_y Pmxx Pmxy Pmxz Pmyx Pmyy Pmyz Pmzx Pmzy Pmzz]=BFCAL_M0(Object,Sxyz,Sxyz_X,Sxyz_Y,Nx,Ny,NXx,NXy,NYx,NYy,NZx,NZy,Nz);

clear Oxyz Oxyz_X Oxyz_Y Sxyz Sxyz_X Sxyz_Y;

[Bxs Bys Bzs]=BFSCAL(Object,Qs,Is);

Bx=Bx_y*JY+Pmxx*MX+Pmxy*MY+Pmxz*MZ+Bxs;
By=By_x*JX+Pmyx*MX+Pmyy*MY+Pmyz*MZ+Bys;
Bz=Bz_x*JX+Bz_y*JY+Pmzx*MX+Pmzy*MY+Pmzz*MZ+Bzs;
k0=1e-7;
Bx=k0*Bx;
By=k0*By;
Bz=k0*Bz;

y=1:Ny;
figure(1);
plot(y,abs(Jcx(y)),'r + -',y,real(Jcx(y)),'g o -',y,imag(Jcx(y)),'b * -');

x=1:Nx;
figure(2);
plot(x,abs(Jcy(x)),'r + -',x,real(Jcy(x)),'g o -',x,imag(Jcy(x)),'b * -');
figure(3);
plot(x,abs(Mx1(x)),'r + -',x,real(Mx1(x)),'g o -',x,imag(Mx1(x)),'b * -');
figure(4);
plot(x,abs(Mx2(x)),'r + -',x,real(Mx2(x)),'g o -',x,imag(Mx2(x)),'b * -');

No=1:no;
figure(5);
plot(No,abs(Bx(No)),'r + -',No,real(Bx(No)),'g o -',No,imag(Bx(No)),'b * -');

figure(6);
plot(No,abs(Bz(No)),'r + -',No,real(Bz(No)),'g o -',No,imag(Bz(No)),'b * -');

end

