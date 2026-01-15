% Qy_x (Mx)
%
% I5=int(f5,y,x',y',z'),   f5=-dz/(dx*dx+dy*dy+dz*dz)^(3/2)
% I5'=int(f5,x,y,x',y',z'),   f5=-dz/(dx*dx+dy*dy+dz*dz)^(3/2)
% Oxyz_Y, Sxyz
%
% updated on 15/02/2011
%
function [I5]=FM_3D_5(Oxyz,Sxyz)  

% Obj are lines, Oxyz(xo,y1,y2,zo),Sxyz(x1,y1,x2,y2,zs)
% Obj are planes, Oxyz(x1,y1,x2,y2,zo),Sxyz(x1,y1,x2,y2,zs)
% OBJ=1,"line"; OBJ=2,"plane"

[no mo]=size(Oxyz);
[ns ms]=size(Sxyz);
Io=ones(no,1);
Is=ones(1,ns);

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
dz=zo-zs;

clear Io Is xo yo1 yo2 zo xs1 xs2 ys1 ys2 zs;

I5=0; 

% x11
[F5]=Formula_3D_5(x11,y11,dz);
I5=I5-F5;

[F5]=Formula_3D_5(x11,y12,dz);
I5=I5+F5;

[F5]=Formula_3D_5(x11,y21,dz);
I5=I5+F5;

[F5]=Formula_3D_5(x11,y22,dz);
I5=I5-F5;

% x12
[F5]=Formula_3D_5(x12,y11,dz);
I5=I5+F5;

[F5]=Formula_3D_5(x12,y12,dz);
I5=I5-F5;

[F5]=Formula_3D_5(x12,y21,dz);
I5=I5-F5;

[F5]=Formula_3D_5(x12,y22,dz);
I5=I5+F5;

clear x11 x12 y11 y12 y21 y22 dz F5;
end