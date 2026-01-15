% T_IND_CELL(Plat) Return inductance matrix (++, -+, +-, --)                       
%                       CROSS PRODUCT
% Z(:,:,p,q)            p,q=1 for "+", and =2 for "-"
% INDEX                 1='++', 2='-+' 3='+-' 4='--'
% X=[x1 x2;x1 x2] Y=[y1 y2];
% coef = k*conj(k)/dx^2/sigma
% Jan. 2016

function [Lxp Lxn Lyp Lyn]=T_IND_CELL(Plat)

% gauss legendre integration method for y bew. [-0.5d 0.5d]
NG=Plat.NG;
[T A] = Gauss(NG);

% getting plate parameters
alpha=Plat.alph;
a=real(alpha);
b=imag(alpha);
Z=Plat.Z;
dz=abs(Z(2)-Z(1));

% (1) x dir cells
[n m]=size(Plat.XCEL);
X=Plat.XCEL(1:n,1:2);
Y=Plat.XCEL(1:n,3:4);

% coef acciated with total current
dy=abs(Y(1:n,2)-Y(1:n,1));
coex=((cosh(a*dz)-cos(b*dz))/(a*a)*(dy*dy')).\1e-7;    %k*conj(k)/dx^2*1e-7

% internal impedance ++ 
INDEX=1;
L=INTE_LC(X,Y,Z,alpha,T,A,INDEX);
Lxp=coex.*L;

% internal impedance -+
INDEX=2;
L=INTE_LC(X,Y,Z,alpha,T,A,INDEX);
Lxn=coex.*L;

% (2) y dir cells
[n m]=size(Plat.YCEL);
X=Plat.YCEL(1:n,1:2);
Y=Plat.YCEL(1:n,3:4);

% coef acciated with total current
dx=abs(X(1:n,2)-X(1:n,1));
coey=((cosh(a*dz)-cos(b*dz))/(a*a)*(dx*dx')).\1e-7;     %k*conj(k)/dx^2*1e-7

% internal impedance ++ 
INDEX=1;
L=INTE_LC(X,Y,Z,alpha,T,A,INDEX);
Lyp=coey.*L;

% internal impedance -+
INDEX=2;
L=INTE_LC(X,Y,Z,alpha,T,A,INDEX);
Lyn=coey.*L;
end