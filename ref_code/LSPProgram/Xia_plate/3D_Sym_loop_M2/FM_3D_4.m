% Ly (Jy)
%
% I4=int(f4,y,x',y')    f4=1/(dx*dx+dy*dy+dz*dz)^(1/2)
% OJ(xo,y1,y2,zo)           field lines
% Xs(x1,x2), Ys(y1,y2)      source cells
% zs                        single value
% updated on 15/06/2011
%
function [I4]=FM_3D_4(OJ,Xs,Ys,zs)   
[no mo]=size(OJ);
[ns ms]=size(Xs);
Io=ones(no,1);
Is=ones(1,ns);

xo=OJ(1:no,1)*Is;
yo1=OJ(1:no,2)*Is;
yo2=OJ(1:no,3)*Is;
zo=OJ(1:no,4)*Is;

xs1=Io*Xs(1:ns,1)';
xs2=Io*Xs(1:ns,2)';
ys1=Io*Ys(1:ns,1)';
ys2=Io*Ys(1:ns,2)';

x11=xo-xs1;
x12=xo-xs2;
y11=yo1-ys1;
y12=yo1-ys2;
y21=yo2-ys1;
y22=yo2-ys2;
dz=zo-zs;
clear Io Is xo yo1 yo2 zo xs1 xs2 ys1 ys2 zs;

I4=0;
% x11
[F4]=Formula_3D_4(x11,y11,dz);
I4=I4-F4;

[F4]=Formula_3D_4(x11,y12,dz);
I4=I4+F4;

[F4]=Formula_3D_4(x11,y21,dz);
I4=I4+F4;

[F4]=Formula_3D_4(x11,y22,dz);
I4=I4-F4;

% x12
[F4]=Formula_3D_4(x12,y11,dz);
I4=I4+F4;

[F4]=Formula_3D_4(x12,y12,dz);
I4=I4-F4;

[F4]=Formula_3D_4(x12,y21,dz);
I4=I4-F4;

[F4]=Formula_3D_4(x12,y22,dz);
I4=I4+F4;

clear x11 x12 y11 y12 y21 y22 dz F4;
end