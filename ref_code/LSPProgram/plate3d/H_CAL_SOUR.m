% H_CAL_SOUR    Calcualte magetic field (B) using Biot-Savart law
%               unit: mG
%
function [BFDS Isx Isy]=H_CAL_SOUR(Fied,Sour)
m0=4*pi*1e-7; k0=m0/(4*pi); k1=k0*1e7;      % unit: mG
nf=Fied.num;
xf=Fied.line(1:nf,1);
yf=Fied.line(1:nf,2);
zf=Fied.line(1:nf,3);

nxs=Sour.xnum;
nys=Sour.ynum;
% (1) Hx Hy Hz from Ix
xs1=Sour.xcen(1:nxs,1);
xs2=Sour.xcen(1:nxs,4);
ys=Sour.xcen(1:nxs,2);
zs=Sour.xcen(1:nxs,3);
Isx=Sour.xcur; 

dx=repmat(xf,1,nxs)-repmat(xs1',nf,1);
dy=repmat(yf,1,nxs)-repmat(ys',nf,1);
dz=repmat(zf,1,nxs)-repmat(zs',nf,1);

[Fsy_X,Fsz_X]=Formula_3D_Bs_X(dx,dy,dz);
Hy_X=Fsy_X;
Hz_X=Fsz_X;

dx=repmat(xf,1,nxs)-repmat(xs2',nf,1);
[Fsy_X,Fsz_X]=Formula_3D_Bs_X(dx,dy,dz);
Hy_X=-Hy_X+Fsy_X;
Hz_X=-Hz_X+Fsz_X;
 
% (2) Hx Hy Hz from Iy
xs=Sour.ycen(1:nys,1);
ys1=Sour.ycen(1:nys,2);
ys2=Sour.ycen(1:nys,5);
zs=Sour.ycen(1:nys,3);
Isy=Sour.ycur;

dx=repmat(xf,1,nys)-repmat(xs',nf,1);
dy=repmat(yf,1,nys)-repmat(ys1',nf,1);
dz=repmat(zf,1,nys)-repmat(zs',nf,1);

[Fsx_Y,Fsz_Y]=Formula_3D_Bs_Y(dx,dy,dz);
Hx_Y=Fsx_Y;
Hz_Y=Fsz_Y;

dy=repmat(yf,1,nys)-repmat(ys2',nf,1);
[Fsx_Y,Fsz_Y]=Formula_3D_Bs_Y(dx,dy,dz);
Hx_Y=-Hx_Y+Fsx_Y;
Hz_Y=-Hz_Y+Fsz_Y;

BFDS.Bxy=Hx_Y*k1;
BFDS.Byx=Hy_X*k1;
BFDS.Bzx=Hz_X*k1;
BFDS.Bzy=Hz_Y*k1;

% Hx=Hx_Y*Isy/(4*pi);
% Hy=Hy_X*Isx/(4*pi);
% Hz=(Hz_X*Isx+Hz_Y*Isy)/(4*pi);
end
