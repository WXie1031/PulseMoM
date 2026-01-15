% By_x (Jx)
%
% I8=int(f8,x',y'),         f8=-dz/(dx*dx+dy*dy+dz*dz)^(3/2)
% OM(xo,yo,zo)              field points
% Xs(x1,x2), Ys(y1,y2)      source cells
% zs                        single value
% updated on 15/06/2011
%
function [I8]=FM_3D_8(OM,Xs,Ys,zs)   
[no mo]=size(OM);
[ns ms]=size(Xs);
Io=ones(no,1);
Is=ones(1,ns);

xo=OM(1:no,1)*Is;
yo=OM(1:no,2)*Is;
zo=OM(1:no,3)*Is;

xs1=Io*Xs(1:ns,1)';
xs2=Io*Xs(1:ns,2)';
ys1=Io*Ys(1:ns,1)';
ys2=Io*Ys(1:ns,2)';

x11=xo-xs1;
x12=xo-xs2;
y11=yo-ys1;
y12=yo-ys2;
dz=zo-zs;
clear Io Is xo yo zo xs1 xs2 ys1 ys2 zs;

I8=0;
% x11
[F8]=Formula_3D_8(x11,y11,dz);
I8=I8+F8;

[F8]=Formula_3D_8(x11,y12,dz);
I8=I8-F8; 

% x12
[F8]=Formula_3D_8(x12,y11,dz);
I8=I8-F8;

[F8]=Formula_3D_8(x12,y12,dz);
I8=I8+F8; 

clear x11 x12 y11 y12 dz F8;

end