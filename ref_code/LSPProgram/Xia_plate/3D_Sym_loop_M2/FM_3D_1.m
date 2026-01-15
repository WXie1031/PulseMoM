% Lx (Jx)
%
% I1=int(f1,x,x',y'),   f1=1/(dx*dx+dy*dy+dz*dz)^(1/2)
% 
% OJ(x1,x2,yo,zo)           field lines
% Xs(x1,x2), Ys(y1,y2)      source cells
% zs                        single value
% updated on 15/06/2011
%
function [I1]=FM_3D_1(OJ,Xs,Ys,zs)   
[no mo]=size(OJ);
[ns ms]=size(Xs);
Io=ones(no,1);
Is=ones(1,ns);

xo1=OJ(1:no,1)*Is;
xo2=OJ(1:no,2)*Is;
yo=OJ(1:no,3)*Is;
zo=OJ(1:no,4)*Is;

xs1=Io*Xs(1:ns,1)';
xs2=Io*Xs(1:ns,2)';
ys1=Io*Ys(1:ns,1)';
ys2=Io*Ys(1:ns,2)';

x11=xo1-xs1;
x12=xo1-xs2;
x21=xo2-xs1;
x22=xo2-xs2;
y11=yo-ys1;
y12=yo-ys2;
dz=zo-zs;
clear Io Is xo1 xo2 yo zo xs1 xs2 ys1 ys2 zs;

I1=0;
% y11
[F1]=Formula_3D_1(x11,y11,dz);
I1=I1-F1;

[F1]=Formula_3D_1(x12,y11,dz);
I1=I1+F1;

[F1]=Formula_3D_1(x21,y11,dz);
I1=I1+F1;

[F1]=Formula_3D_1(x22,y11,dz);
I1=I1-F1;

% y12
[F1]=Formula_3D_1(x11,y12,dz);
I1=I1+F1;

[F1]=Formula_3D_1(x12,y12,dz);
I1=I1-F1;

[F1]=Formula_3D_1(x21,y12,dz);
I1=I1-F1;

[F1]=Formula_3D_1(x22,y12,dz);
I1=I1+F1;

clear x11 x12 x21 x22 y11 y12 dz F1; 
end