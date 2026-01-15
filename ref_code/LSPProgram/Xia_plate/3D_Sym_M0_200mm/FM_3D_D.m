% updated on 13/08/2010

% Qy_x (Mx), Qy_z (Mz)
% I5=int(f5,y,x',y',z'),   f5=-dz/(dx*dx+dy*dy+dz*dz)^(3/2)
% I5'=int(f5,x,y,x',y',z'),   f5=-dz/(dx*dx+dy*dy+dz*dz)^(3/2)
% I6=int(f6,y,x',y',z'),   f6=dx/(dx*dx+dy*dy+dz*dz)^(3/2)
% I6'=int(f6,x,y,x',y',z'),   f6=dx/(dx*dx+dy*dy+dz*dz)^(3/2)

% Oxyz_Y, Sxyz

function [I5,I6]=FM_3D_D(Oxyz,Sxyz,OBJ)  

% Obj are lines, Oxyz(xo,y1,y2,zo),Sxyz(x1,y1,x2,y2,zs)
% Obj are planes, Oxyz(x1,y1,x2,y2,zo),Sxyz(x1,y1,x2,y2,zs)
% OBJ=1,"line"; OBJ=2,"plane"

[no mo]=size(Oxyz);
[ns ms]=size(Sxyz);

Is=ones(1,ns);
Io=ones(no,1);
xo=Oxyz(:,1)*Is;
yo1=Oxyz(:,2)*Is;
yo2=Oxyz(:,3)*Is;
zo=Oxyz(:,4)*Is;
xs1=Io*Sxyz(:,1)';
xs2=Io*Sxyz(:,3)';
ys1=Io*Sxyz(:,2)';
ys2=Io*Sxyz(:,4)';
zs=Io*Sxyz(:,5)';

x11=xo-xs1;
x12=xo-xs2;
y11=yo1-ys1;
y12=yo1-ys2;
y21=yo2-ys1;
y22=yo2-ys2;
z=zo-zs;

clear Is Io xo xs1 xs2 yo1 yo2 ys1 ys2 zo zs;

I5=0; I6=0;

% x11
[F5,F6]=Formula_3D_D(x11,y11,z);
I5=I5-F5;
I6=I6-F6;

[F5,F6]=Formula_3D_D(x11,y12,z);
I5=I5+F5;
I6=I6+F6;

[F5,F6]=Formula_3D_D(x11,y21,z);
I5=I5+F5;
I6=I6+F6;

[F5,F6]=Formula_3D_D(x11,y22,z);
I5=I5-F5;
I6=I6-F6;

% x12
[F5,F6]=Formula_3D_D(x12,y11,z);
I5=I5+F5;
I6=I6+F6;

[F5,F6]=Formula_3D_D(x12,y12,z);
I5=I5-F5;
I6=I6-F6;

[F5,F6]=Formula_3D_D(x12,y21,z);
I5=I5-F5;
I6=I6-F6;

[F5,F6]=Formula_3D_D(x12,y22,z);
I5=I5+F5;
I6=I6+F6;

clear x11 x12 y11 y12 y21 y22 z F5 F6;
end