% Qx_y (My)
%
% I2=int(f2,x,x',y',z'),   f2=dz/(dx*dx+dy*dy+dz*dz)^(3/2)
% I2'=int(f2,y,x,x',y',z'),   f2=dz/(dx*dx+dy*dy+dz*dz)^(3/2)
% Oxyz_X, Sxyz
%
% updated on 15/02/2011
%
function [I2]=FM_3D_2(Oxyz,Sxyz)   

% Obj are lines, Oxyz(x1,x2,yo,zo),Sxyz(x1,y1,x2,y2,zs)
% Obj are planes, Oxyz(x1,y1,x2,y2,zo),Sxyz(x1,y1,x2,y2,zs)
% OBJ=1,"line"; OBJ=2,"plane"

[no mo]=size(Oxyz);
[ns ms]=size(Sxyz);
Io=ones(no,1);
Is=ones(1,ns);

xo1=Oxyz(:,1)*Is;
xo2=Oxyz(:,2)*Is;
yo=Oxyz(:,3)*Is;
zo=Oxyz(:,4)*Is;

xs1=Io*Sxyz(:,1)';
xs2=Io*Sxyz(:,2)';
ys1=Io*Sxyz(:,3)';
ys2=Io*Sxyz(:,4)';
zs=Io*Sxyz(:,5)';

x11=xo1-xs1;
x12=xo1-xs2;
x21=xo2-xs1;
x22=xo2-xs2;
y11=yo-ys1;
y12=yo-ys2;
dz=zo-zs;

clear Io Is xo1 xo2 yo zo xs1 xs2 ys1 ys2 zs;

I2=0; 

% y11
[F2]=Formula_3D_2(x11,y11,dz);
I2=I2-F2;

[F2]=Formula_3D_2(x12,y11,dz);
I2=I2+F2;

[F2]=Formula_3D_2(x21,y11,dz);
I2=I2+F2;

[F2]=Formula_3D_2(x22,y11,dz);
I2=I2-F2;

% y12
[F2]=Formula_3D_2(x11,y12,dz);
I2=I2+F2;

[F2]=Formula_3D_2(x12,y12,dz);
I2=I2-F2;

[F2]=Formula_3D_2(x21,y12,dz);
I2=I2-F2;

[F2]=Formula_3D_2(x22,y12,dz);
I2=I2+F2;

clear x11 x12 x21 x22 y11 y12 dz F2; 
end