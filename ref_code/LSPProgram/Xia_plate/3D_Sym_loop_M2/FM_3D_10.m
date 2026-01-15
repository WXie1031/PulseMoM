% Bz_y (Jy)
%
% I10=int(f10,x',y'),   f10=-dx/(dx*dx+dy*dy+dz*dz)^(3/2)
% OM(xo,yo,zo)              field lines
% Xs(x1,x2), Ys(y1,y2)      source cells
% zs                        single value
% updated on 15/06/2011
%
function [I10]=FM_3D_10(OM,Xs,Ys,zs)   
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

I10=0;
% x11
[F10]=Formula_3D_10(x11,y11,dz);
I10=I10+F10;

[F10]=Formula_3D_10(x11,y12,dz);
I10=I10-F10;

% x12
[F10]=Formula_3D_10(x12,y11,dz);
I10=I10-F10;

[F10]=Formula_3D_10(x12,y12,dz);
I10=I10+F10;

clear x11 x12 y11 y12 dz F10;

end