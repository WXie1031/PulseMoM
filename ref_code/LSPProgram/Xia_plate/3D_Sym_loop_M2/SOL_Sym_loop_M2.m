% SOL_Sym_loop_M2 
% Main program for solving Jx Jy Mx My Mz in Ferro-magnetic plates and B field around the plates
%                  using METHOD M2  (segment-match for KVL, point-match for M)
% using the symmetry of the structrue, only 1/4 of the unknowns need to be solved 
% Jx Jy     applying double exponential distribution along z-direction
%           (S5 S6)
% Mx My Mz  are on the surface Mx (S3 S4 S5 S6)
%                              My (S1 S2 S5 S6)
%                              Mz (S1 S2 S3 S4)
% updated on 21/12/2011
%
function [Jx,Jy,Mx,My,Mz,Bx,By,Bz,Z]=SOL_Sym_loop_M2

clear all;
ts=clock();
% initilization & material info.
f=1e3; mur=35; delt=0.75e+7; mu0=4e-7*pi;
aa=-(1+1i)*sqrt(pi*f*mu0*mur*delt);
% ******************************************************

% source lines
Qs=[-0.05 -0.05 0.0; 
     0.05 -0.05 0.0;
     0.05  0.05 0.0;
    -0.05  0.05 0.0]; 
% Qs=[-0.01 -0.01 0.0; 
%     0.01 -0.01 0.0;
%     0.01 0.01 0.0;
%     -0.01 0.01 0.0];
Is=1;
% ******************************************************

% observation lines for magnetic field
uo=linspace(-0.2,0.2,51);
no=length(uo);            % points on a line
Io=ones(no,1);
Nb=3;                     % No. of lines        
Object=[uo' Io*0 Io*0.15;
        uo' uo' Io*0.1;
        uo' uo' Io*0.075];
% ******************************************************

% position and dimensions of plate
Pl=[0 0 0.05];           % coordinates of middle points of plates [X0 Y0 Z0]
Ps=[0.2 0.2 0.002];      % plate size [w l d]
N=[8 8 4];               % divide number along x, y, z [Nx Ny Nz]
ko=[-1 -0.5 0 0.5 1];    % position for observing current density (-1:1,bottom:top)
p=0;                     % No. of figures
% ******************************************************

[OMx OMy OMz OJx OJy S1 S2 S3 SX SY u,v,w]=MESHING_3D_M2(Pl,Ps,N,0);      
[NCX N0]=size(OJx);
[NCY N0]=size(OJy);
[NMX N0]=size(OMx);
[NMY N0]=size(OMy);
[NMZ N0]=size(OMz);

Nx=length(u)-1;   Ny=length(v)-1;   Nz=length(w)-1;
N=[Nx Ny Nz];
p=p+1;
figure(p);
plot(u,ones(1,Nx+1),'r*');
du=u(2:Nx+1)-u(1:Nx);

Nxc=Nx/2; Nyc=Ny/2;
NXxc=Nxc-1;
NYyc=Nyc-1;

NX=NXxc*Nyc;
NY=Nxc*NYyc;
Nxy=Nxc*Nyc;
Nyz=Nyc*Nz;
Nxz=Nxc*Nz;
% ******************************************************

[Coex,Coey]=Coef_Sym_loop(u,v,N);

% coefficient for object branches and points
ke=2/(1i*f*delt*mu0);                % ke =1/(j*(2*pi*f)*delt*mu0/(4*pi))
dx=OJx(:,2)-OJx(:,1);
dy=OJy(:,3)-OJy(:,2);
Ex=sparse(1:NCX,1:NCX,dx);
Ey=sparse(1:NCY,1:NCY,dy);

km=4*pi*mur/(mur-1);                 % mur=200,km=mu0*mur/(mur-1)/(mu0/4*pi)      
Emx=speye(NMX);
Emy=speye(NMY);
Emz=speye(NMZ);

clear dx dy;
% *************************************************************************

% main program
% interaction between each current cells  
t0=clock();
[Lx Qx_y Qx_z Ly Qy_x Qy_z Bx_y By_x Bz_x Bz_y Pmxx Pmxy Pmxz Pmyx Pmyy Pmyz Pmzx Pmzy Pmzz]=COEF_3D_PLATE_M2(Pl,Ps,OMx,OMy,OMz,OJx,OJy,S1,S2,S3,SX,SY,aa,N);
t1=clock();
t1=t1-t0;      % time for calculating coefficient matrixes

Lx=mur*Lx+ke*Ex;  
Ly=mur*Ly+ke*Ey;  
Pxx=Pmxx-km*Emx;  
Pyy=Pmyy-km*Emy;  
Pzz=Pmzz-km*Emz;  
Bx_y=mur*Bx_y;  
By_x=mur*By_x;  
Bz_x=mur*Bz_x;  
Bz_y=mur*Bz_y; 

clear Ex Ey Emx Emy Emz;

% force by external sources
t2=clock();
[Sx,Sy,Bxs,Bys,Bzs]=COEF_3D_SOUR_M2(OMx,OMy,OMz,OJx,OJy,Qs,Is);
t3=clock();
t2=t3-t2;     % time for calculating effect from sources

% solve the equation matrix
ZQxy=sparse(NCX,Nyz); ZQxz=sparse(NCX,Nyz);
ZQyx=sparse(NCY,Nxz); ZQyz=sparse(NCY,Nxz);
ZPxy=sparse(NMX,2*Nxy);  ZPxz=sparse(NMX,Nxz);
ZPyx=sparse(NMY,2*Nxy);  ZPyz=sparse(NMY,Nyz);
ZPzx=sparse(NMZ,Nxz);    ZPzy=sparse(NMZ,Nyz);
 
Qxy=[ZQxy Qx_y]; Qxz=[ZQxz Qx_z];  
Qyx=[ZQyx Qy_x]; Qyz=[Qy_z ZQyz];  
Pxy=[Pmxy ZPxy]; Pxz=[Pmxz ZPxz];  
Pyx=[Pmyx ZPyx]; Pyz=[ZPyz Pmyz];  
Pzx=[ZPzx Pmzx]; Pzy=[ZPzy Pmzy];  

% Qxy=sparse(Qxy); Qxz=sparse(Qxz); Qyx=sparse(Qyx); Qyz=sparse(Qyz);
% Pxy=sparse(Pxy); Pxz=sparse(Pxz); Pyx=sparse(Pyx); Pyz=sparse(Pyz);
% Pzx=sparse(Pzx); Pzy=sparse(Pzy);

Qxy=full(Qxy); Qxz=full(Qxz); Qyx=full(Qyx); Qyz=full(Qyz);
Pxy=full(Pxy); Pxz=full(Pxz); Pyx=full(Pyx); Pyz=full(Pyz);
Pzx=full(Pzx); Pzy=full(Pzy);

clear ZQxy ZQxz ZQyx ZQyz ZPxy ZPxz ZPyx ZPyz ZPzx ZPzy;

[L,Qx,Qy,Qz,Bx,By,Bz,S]=Trans_loop(Lx,Qxy,Qxz,Ly,Qyx,Qyz,Bx_y,By_x,Bz_x,Bz_y,Sx,Sy,Coex,Coey,N);

T=[S; Bxs; Bys; Bzs];
T=-T;
clear Sx Sy Bxs Bys Bsz Z14;

G=[L   Qx   Qy   Qz;      
   Bx  Pxx  Pxy  Pxz;
   By  Pyx  Pyy  Pyz;
   Bz  Pzx  Pzy  Pzz];

Pinv = inv([Pxx Pxy Pxz;
            Pyx Pyy Pyz;
            Pzx Pzy Pzz;]);

Bm = [Bx;By;Bz];
Qm = [Qx Qy Qz;];

Z = L-Qm*Pinv*Bm;

G=sparse(G);

clear Lx Ly Qx_y Qx_z Qy_x Qy_z Bx_y By_x Bz_x Bz_y Pmxx Pmxy Pmxz Pmyx Pmyy Pmyz Pmzx Pmzy Pmzz;

t4=clock();
Q=G\T;
t5=clock();
t3=t5-t4;      % time for total solution

clear G T;
td=clock();
tt=td-ts;

I1=Q(1:Nxy);
I2=Q(Nxy+1:2*Nxy);
MX=Q(2*Nxy+1:2*Nxy+NMX);
MY=Q(2*Nxy+NMX+1:2*Nxy+NMX+NMY);
MZ=Q(2*Nxy+NMX+NMY+1:2*Nxy+NMX+NMY+NMZ);

JX5=Coex*I1;
JX6=Coex*I2;
JY5=Coey*I1;
JY6=Coey*I2;

JX=[JX5; JX6];
JY=[JY5; JY6];
MX3=MX(1:Nxz);
MX5=MX(Nxz+1:Nxz+Nxy);
MX6=MX(Nxz+Nxy+1:Nxz+2*Nxy);

MY1=MY(1:Nyz);
MY5=MY(Nyz+1:Nyz+Nxy);
MY6=MY(Nyz+Nxy+1:Nyz+2*Nxy);

MZ1=MZ(1:Nyz);
MZ3=MZ(Nyz+1:Nyz+Nxz);

% the solutions matrix of Jx Jy Mx My Mz
for i=1:Nyc    
    Jx5(i,:)=JX5((i-1)*NXxc+1:i*NXxc);           % on the bottom ('1')
    Jx6(i,:)=JX6((i-1)*NXxc+1:i*NXxc);           % on the top ('2')    
    Mx5(i,:)=MX5((i-1)*Nxc+1:i*Nxc);
    Mx6(i,:)=MX6((i-1)*Nxc+1:i*Nxc);
    My5(i,:)=MY5((i-1)*Nxc+1:i*Nxc);
    My6(i,:)=MY6((i-1)*Nxc+1:i*Nxc);    
end

Jx5=[Jx5 JX5(Nxy-Nyc+1:Nxy) fliplr(Jx5);
     -flipud(Jx5) -flipud(JX5(Nxy-Nyc+1:Nxy)) -rot90(Jx5,2)];
Jx6=[Jx6 JX6(Nxy-Nyc+1:Nxy) fliplr(Jx6);
     -flipud(Jx6) -flipud(JX6(Nxy-Nyc+1:Nxy)) -rot90(Jx6,2)];

Mx5=[Mx5 -fliplr(Mx5);
     flipud(Mx5) -rot90(Mx5,2)];
Mx6=[Mx6 -fliplr(Mx6);
     flipud(Mx6) -rot90(Mx6,2)];
My5=[My5 fliplr(My5);
     -flipud(My5) -rot90(My5,2)];
My6=[My6 fliplr(My6);
     -flipud(My6) -rot90(My6,2)];

for i=1:NYyc
    Jy5(i,:)=JY5((i-1)*Nxc+1:i*Nxc);             
    Jy6(i,:)=JY6((i-1)*Nxc+1:i*Nxc);
end
Jy5=[Jy5 -fliplr(Jy5); 
     JY5(Nxy-Nxc+1:Nxy).' -fliplr(JY5(Nxy-Nxc+1:Nxy).');
     flipud(Jy5) -rot90(Jy5,2)];
Jy6=[Jy6 -fliplr(Jy6); 
     JY6(Nxy-Nxc+1:Nxy).' -fliplr(JY6(Nxy-Nxc+1:Nxy).');
     flipud(Jy6) -rot90(Jy6,2)];

for i=1:Nz
    Mx3(i,:)=MX3((i-1)*Nxc+1:i*Nxc);    
    Mz3(i,:)=MZ3((i-1)*Nxc+1:i*Nxc);    
    My1(i,:)=MY1((i-1)*Nyc+1:i*Nyc);
    Mz1(i,:)=MZ1((i-1)*Nyc+1:i*Nyc);    
end
Mx3=[Mx3 -fliplr(Mx3)];
Mx4=Mx3;
My1=[My1 -fliplr(My1)];
My2=My1;
Mz3=[Mz3 fliplr(Mz3)];
Mz4=Mz3;
Mz1=[Mz1 fliplr(Mz1)];
Mz2=Mz1;

Jx5=full(Jx5); Jx6=full(Jx6); Jy5=full(Jy5); Jy6=full(Jy6);
Mx3=full(Mx3); Mx4=full(Mx4); Mx5=full(Mx5); Mx6=full(Mx6);
My1=full(My1); My2=full(My2); My5=full(My5); My6=full(My6);
Mz1=full(Mz1); Mz2=full(Mz2); Mz3=full(Mz3); Mz4=full(Mz4);

% the solutions on the objective lines
No=length(ko);
d=Ps(3);
ko1=sinh(0.5*aa*d*(1-ko))/sinh(aa*d);          % ko (-1:1,bottom:top)
ko2=sinh(0.5*aa*d*(1+ko))/sinh(aa*d);
for i=1:No
    Jxo=ko1(i)*Jx5+ko2(i)*Jx6;                           % on the observation ('o') plane
    Jx(:,i)=Jxo(:,Nx/2);
    Jyo=ko1(i)*Jy5+ko2(i)*Jy6;
    Jy(:,i)=Jyo(Ny/2,:);    
end
Mx3o=Mx3(Nz/2,:); Mx5o=Mx5(Ny/2,:); Mx6o=Mx6(Ny/2,:);
My1o=My1(Nz/2,:); My5o=My5(:,Nx/2); My6o=My6(:,Nx/2);
Mz1o=Mz1(Nz/2,:); Mz3o=Mz3(Nz/2,:); 
% *************************************************************************

% draw the solution lines
% Jx
p=p+1;
figure(p);
for i=1:No
    y=1:Ny;
    Jxo=Jx(:,i);
    subplot(2,3,i);    
    plot(y,abs(Jxo(y)),'r + -',y,real(Jxo(y)),'g o -',y,imag(Jxo(y)),'b * -');
end

% Jy
p=p+1;
figure(p);
for i=1:No
    x=1:Nx;
    Jyo=Jy(:,i);
    subplot(2,3,i); 
    plot(x,abs(Jyo(x)),'r + -',x,real(Jyo(x)),'g o -',x,imag(Jyo(x)),'b * -');
end

% Mx
p=p+1;
figure(p);
x=1:Nx;
subplot(1,3,1);
plot(x,abs(Mx3o(x)),'r + -',x,real(Mx3o(x)),'g o -',x,imag(Mx3o(x)),'b * -');
subplot(1,3,2);
plot(x,abs(Mx5o(x)),'r + -',x,real(Mx5o(x)),'g o -',x,imag(Mx5o(x)),'b * -');
subplot(1,3,3);
plot(x,abs(Mx6o(x)),'r + -',x,real(Mx6o(x)),'g o -',x,imag(Mx6o(x)),'b * -');

% My
p=p+1;
figure(p);
y=1:Ny;
subplot(1,3,1);   
plot(y,abs(My1o(y)),'r + -',y,real(My1o(y)),'g o -',y,imag(My1o(y)),'b * -');
subplot(1,3,2);   
plot(y,abs(My5o(y)),'r + -',y,real(My5o(y)),'g o -',y,imag(My5o(y)),'b * -');
subplot(1,3,3);   
plot(y,abs(My6o(y)),'r + -',y,real(My6o(y)),'g o -',y,imag(My6o(y)),'b * -');
    
p=p+1;
figure(p);
y=1:Ny;
subplot(1,2,1);
plot(y,abs(Mz1o(y)),'r + -',y,real(Mz1o(y)),'g o -',y,imag(Mz1o(y)),'b * -');
x=1:Nx;
subplot(1,2,2);
plot(x,abs(Mz3o(x)),'r + -',x,real(Mz3o(x)),'g o -',x,imag(Mz3o(x)),'b * -');
% *************************************************************************

% calculate the magnetic fields
[Bx_y By_x Bz_x Bz_y Pmxx Pmxy Pmxz Pmyx Pmyy Pmyz Pmzx Pmzy Pmzz]=BFCAL_M2(Pl,Ps,Object,S1,S2,S3,SX,SY,aa,N);

[Bxs Bys Bzs]=BFSCAL(Object,Qs,Is);

Bxo=mur*Bx_y*JY+Pmxx*MX+Pmxy*MY1+Pmxz*MZ1+Bxs;
Byo=mur*By_x*JX+Pmyx*MX3+Pmyy*MY+Pmyz*MZ3+Bys;
Bzo=mur*Bz_x*JX+mur*Bz_y*JY+Pmzx*[MX5;MX6]+Pmzy*[MY5;MY6]+Pmzz*MZ+Bzs;
k0=1e-7;
Bxo=k0*Bxo;
Byo=k0*Byo;
Bzo=k0*Bzo;

for i=1:Nb
    BX(:,i)=Bxo((i-1)*no+1:i*no,:);
    BY(:,i)=Byo((i-1)*no+1:i*no,:);
    BZ(:,i)=Bzo((i-1)*no+1:i*no,:);
end

% draw B field
No=1:no;

% BX
p=p+1;
figure(p);
for i=1:Nb
    subplot(1,Nb,i);
    Bxp=BX(:,i);
    plot(No,abs(Bxp(No)),'r + -',No,real(Bxp(No)),'g o -',No,imag(Bxp(No)),'b * -');
end

% BY
p=p+1;
figure(p);
for i=1:Nb
    subplot(1,Nb,i);
    Byp=BY(:,i);
    plot(No,abs(Byp(No)),'r + -',No,real(Byp(No)),'g o -',No,imag(Byp(No)),'b * -');
end

% BZ
p=p+1;
figure(p);
for i=1:Nb
    subplot(1,Nb,i);
    Bzp=BZ(:,i);
    plot(No,abs(Bzp(No)),'r + -',No,real(Bzp(No)),'g o -',No,imag(Bzp(No)),'b * -');
end

end

