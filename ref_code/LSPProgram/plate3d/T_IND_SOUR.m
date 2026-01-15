% T_IND_SOUR   Return field matrix (+, -) from source current
%                        
% Ps=(xs,ys)            coordinates of source lines
% Z(:,:,p,q)            p,q=1 for "+", and =2 for "-"
% INDEX                 1='++', 2='-+' 3='+-' 4='--'
% X=[x1 x2;x1 x2] Y=[y1 y2];
% coef = k*conj(k)/dx^2/sigma
% Jan. 2016

function [Lxps Lxns Lyps Lyns]=T_IND_SOUR(Sour,Plat)

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
coex=conj(2*sinh(0.5*alpha*dz)./alpha*dy).\1e-7;      %k*conj(k)/dx*e-7

Lxps=[];
Lxns=[];
Ns=Sour.xnum;
if Ns~=0
    Ps=Sour.xcen;
    coex=repmat(coex,1,Ns);

% impedance +s
    INDEX=1;
    L=INTE_LS(X,Y,Z,Ps,alpha,T,A,INDEX,1);
    Lxps=coex.*L;

% impedance -s
    INDEX=2;
    L=INTE_LS(X,Y,Z,Ps,alpha,T,A,INDEX,1);
    Lxns=coex.*L;
end

% (2) y dir cells
[n m]=size(Plat.YCEL);
X=Plat.YCEL(1:n,1:2);
Y=Plat.YCEL(1:n,3:4);

% coef acciated with total current
dx=abs(X(1:n,2)-X(1:n,1));
coey=conj(2*sinh(0.5*alpha*dz)./alpha*dx).\1e-7;      %k*conj(k)/dx*e-7

Lyps=[];
Lyns=[];
Ns=Sour.ynum;
if Ns~=0
    Ps=Sour.ycen;
    coey=repmat(coey,1,Ns);

% impedance +s
    INDEX=1;
    L=INTE_LS(X,Y,Z,Ps,alpha,T,A,INDEX,2);
    Lyps=coey.*L;

% impedance -s
    INDEX=2;
    L=INTE_LS(X,Y,Z,Ps,alpha,T,A,INDEX,2);
    Lyns=coey.*L;
end
% coey*2*sinh(0.5*conj(alpha)*dz)/conj(alpha)
% coex*2*sinh(0.5*conj(alpha)*dz)/conj(alpha)
end