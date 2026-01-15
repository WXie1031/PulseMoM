% updated on 13/08/2010

% Bx_y (Jcy), Bz_y (Jcy)
% I7=int(f7,x',y',z'),   f7=dz/(dx*dx+dy*dy+dz*dz)^(3/2)
% I10=int(f10,x',y',z'),   f10=-dx/(dx*dx+dy*dy+dz*dz)^(3/2)
% Oxyz, Sxyz_Y

function [I7,I10]=FM_3D_E(Oxyz,Sxyz)   
% Oxyz(xo,yo,zo),Sxyz(x1,y1,x2,y2,zs)

[no mo]=size(Oxyz);
[ns ms]=size(Sxyz);

Is=ones(1,ns);
Io=ones(no,1);
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
z=zo-zs;

clear Is Io xo xs1 xs2 yo ys1 ys2 zo zs;

I7=0; I10=0;

% x11
[F7,F10]=Formula_3D_E(x11,y11,z);
I7=I7+F7;
I10=I10+F10;

[F7,F10]=Formula_3D_E(x11,y12,z);
I7=I7-F7;
I10=I10-F10;

% x12
[F7,F10]=Formula_3D_E(x12,y11,z);
I7=I7-F7;
I10=I10-F10;

[F7,F10]=Formula_3D_E(x12,y12,z);
I7=I7+F7;
I10=I10+F10;

clear x11 x12 y11 y12 z F7 F10;

end