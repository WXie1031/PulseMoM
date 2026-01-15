% Bx_y (Jy)
%
% I7=int(f7,x',y',z'),   f7=dz/(dx*dx+dy*dy+dz*dz)^(3/2)
% Oxyz(xo,yo,zo),Sxyz(x1,y1,x2,y2,zs)
%
% updated on 15/02/2011
%
function [I7]=FM_3D_7(Oxyz,Sxyz)   

[no mo]=size(Oxyz);
[ns ms]=size(Sxyz);
Io=ones(no,1);
Is=ones(1,ns);

xo=Oxyz(:,1)*Is;
yo=Oxyz(:,2)*Is;
zo=Oxyz(:,3)*Is;

xs1=Io*Sxyz(:,1)';
xs2=Io*Sxyz(:,3)';
ys1=Io*Sxyz(:,2)';
ys2=Io*Sxyz(:,4)';
zs=Io*Sxyz(:,5)';

x11=xo-xs1;
x12=xo-xs2;
y11=yo-ys1;
y12=yo-ys2;
dz=zo-zs;

clear Io Is xo yo zo xs1 xs2 ys1 ys2 zs;

I7=0; 

% x11
[F7]=Formula_3D_7(x11,y11,dz);
I7=I7+F7; 

[F7]=Formula_3D_7(x11,y12,dz);
I7=I7-F7;

% x12
[F7]=Formula_3D_7(x12,y11,dz);
I7=I7-F7; 

[F7]=Formula_3D_7(x12,y12,dz);
I7=I7+F7;

clear x11 x12 y11 y12 dz F7;

end