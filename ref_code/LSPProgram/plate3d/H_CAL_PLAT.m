% H_CAL_SOUR    Calcualte magetic field (H) using free-space Green's fun
%               Using current-density formulas, converting to cell current
%               B unit: mG
%
function BFDC=H_CAL_PLAT(Fied,Plat)
nf=Fied.num;
xf=Fied.line(1:nf,1);
yf=Fied.line(1:nf,2);
zf=Fied.line(1:nf,3);

Nx=Plat.num(1);                 % node cell # in x dir
Ny=Plat.num(2);                 % node cell # in y dir
nx=Nx-1;                        % x-cell cell # in x dir
ny=Ny-1;                        % y-cell cell # in y dir

nnx=nx*Ny;                      % total Ix cells
nny=ny*Nx;                      % total Iy cells
nn=Nx*Ny;                       % total p cells

z0=Plat.cent(3);

 % (1) Ix contribution
dz=repmat(zf,1,nnx)-z0;
xs1=Plat.XCEL(1:nnx,1);
xs2=Plat.XCEL(1:nnx,2);
ys1=Plat.XCEL(1:nnx,3);
ys2=Plat.XCEL(1:nnx,4);

 % S11
dx=repmat(xf,1,nnx)-repmat(xs1',nf,1);
dy=repmat(yf,1,nnx)-repmat(ys1',nf,1);
[Byx11 Bzx11]=F_H_Ix(dx,dy,dz);
 % $12
dx=repmat(xf,1,nnx)-repmat(xs1',nf,1);
dy=repmat(yf,1,nnx)-repmat(ys2',nf,1);
[Byx12 Bzx12]=F_H_Ix(dx,dy,dz);
 % $21
dx=repmat(xf,1,nnx)-repmat(xs2',nf,1);
dy=repmat(yf,1,nnx)-repmat(ys1',nf,1);
[Byx21 Bzx21]=F_H_Ix(dx,dy,dz);
 % $12
dx=repmat(xf,1,nnx)-repmat(xs2',nf,1);
dy=repmat(yf,1,nnx)-repmat(ys2',nf,1);
[Byx22 Bzx22]=F_H_Ix(dx,dy,dz);

BFDC.Byx=Byx11-Byx12-Byx21+Byx22;
BFDC.Bzx=Bzx11-Bzx12-Bzx21+Bzx22;

 % (2) Iy contribution
dz=repmat(zf,1,nny)-z0;
xs1=Plat.YCEL(1:nny,1);
xs2=Plat.YCEL(1:nny,2);
ys1=Plat.YCEL(1:nny,3);
ys2=Plat.YCEL(1:nny,4);

% S11
dx=repmat(xf,1,nny)-repmat(xs1',nf,1);
dy=repmat(yf,1,nny)-repmat(ys1',nf,1);
[Bxy11 Bzy11]=F_H_Iy(dx,dy,dz);
 % $12
dx=repmat(xf,1,nny)-repmat(xs1',nf,1);
dy=repmat(yf,1,nny)-repmat(ys2',nf,1);
[Bxy12 Bzy12]=F_H_Iy(dx,dy,dz);
 % $21
dx=repmat(xf,1,nny)-repmat(xs2',nf,1);
dy=repmat(yf,1,nny)-repmat(ys1',nf,1);
[Bxy21 Bzy21]=F_H_Iy(dx,dy,dz);
 % $12
dx=repmat(xf,1,nny)-repmat(xs2',nf,1);
dy=repmat(yf,1,nny)-repmat(ys2',nf,1);
[Bxy22 Bzy22]=F_H_Iy(dx,dy,dz);

BFDC.Bxy=Bxy11-Bxy12-Bxy21+Bxy22;
BFDC.Bzy=Bzy11-Bzy12-Bzy21+Bzy22;

% convert action of line density to cell current (Jx/Jy --> Ix/Iy) 
xw=repmat(Plat.XCEL(:,7)',6,1);                  % x cell withd
yw=repmat(Plat.YCEL(:,7)',6,1);                  % y cell withd
BFDC.Bxy=BFDC.Bxy./yw;
BFDC.Bzy=BFDC.Bzy./yw;
BFDC.Byx=BFDC.Byx./xw;
BFDC.Bzx=BFDC.Bzx./xw;
% Hx=Bxy*Jy/(4*pi);
% Hy=Byx*Jx/(4*pi);
% Hz=(Bzx*Jx+Bzy*Jy)/(4*pi);

end