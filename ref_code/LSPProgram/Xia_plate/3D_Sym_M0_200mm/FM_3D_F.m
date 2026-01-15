% updated on 13/08/2010

% By_x (Jcx), Bz_x (Jcx)
% I8=int(f8,x',y',z'),   f8=-dz/(dx*dx+dy*dy+dz*dz)^(3/2)
% I9=int(f9,x',y',z'),   f9=dy/(dx*dx+dy*dy+dz*dz)^(3/2)
% Oxyz, Sxyz_X

function [I8,I9]=FM_3D_F(Oxyz,Sxyz)   
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

clear Is Io xo1 xo2 xs1 xs2 yo ys1 ys2 zo zs;

I8=0; I9=0;

% x11
[F8,F9]=Formula_3D_F(x11,y11,z);
I8=I8+F8;
I9=I9+F9;

[F8,F9]=Formula_3D_F(x11,y12,z);
I8=I8-F8;
I9=I9-F9;

% x12
[F8,F9]=Formula_3D_F(x12,y11,z);
I8=I8-F8;
I9=I9-F9;

[F8,F9]=Formula_3D_F(x12,y12,z);
I8=I8+F8;
I9=I9+F9;

clear x11 x12 y11 y12 z F8 F9;

end