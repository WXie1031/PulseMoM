% T_RES_CELL(X,Y)   Return self resistance matrix (++=--, +-=-+)
%                   with distributied current o 
% Rxp Rxn Ryp Ryn   R++=R--=Rp  Rn=R-+=R+-
% X=[x1 x2] Y=[y1 y2] Z=[z1 z2]
% dsx=2*sinh(0.5alpha*d)/alpha*dy
% dsy=2*sinh(0.5alpha*d)/alpha*dx
% Rxp=dz./(sigma*dsx2).*(sinh(a*dz)*dx/a);
% Rxn=dz./(sigma*dsx2).*(sin(b*dz)*dx/b);
% Jan. 2016

function [Rxp Rxn Ryp Ryn]=T_RES_CELL(Plat)
alpha=Plat.alph;
a=real(alpha);
b=imag(alpha);
sigma=Plat.sigm;

% (1) X curr cells
[n m]=size(Plat.XCEL);

dx=abs(Plat.XCEL(1:n,2)-Plat.XCEL(1:n,1));
dy=abs(Plat.XCEL(1:n,4)-Plat.XCEL(1:n,3));
dz=abs(Plat.Z(2)-Plat.Z(1));

dsx=2*sinh(0.5*alpha*dz)/alpha*dy; 
dsx2=(cosh(a*dz)-cos(b*dz))*dy.*dy/(a*a);  % dsx2=dsx.*conj(dsx)

Rxp=dx./(sigma*dsx2).*(sinh(a*dz)*dy/a);
Rxn=dx./(sigma*dsx2).*(sin(b*dz)*dy/b);

% (2) Y curr cells
[n m]=size(Plat.YCEL);

dx=abs(Plat.YCEL(1:n,2)-Plat.YCEL(1:n,1));
dy=abs(Plat.YCEL(1:n,4)-Plat.YCEL(1:n,3));
dz=abs(Plat.Z(2)-Plat.Z(1));

dsy=2*sinh(0.5*alpha*dz)/alpha*dx;
dsy2=(cosh(a*dz)-cos(b*dz))*dx.*dx/(a*a); % dsy2=dsy.*conj(dsy)

Ryp=dy./(sigma*dsy2).*(sinh(a*dz)*dx/a);
Ryn=dy./(sigma*dsy2).*(sin(b*dz)*dx/b);
end
