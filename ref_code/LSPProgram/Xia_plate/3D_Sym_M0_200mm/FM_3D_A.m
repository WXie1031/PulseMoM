% updated on 13/08/2010

% Lx (Jcx)
% I1=int(f1,x,x',y',z'),   f1=1/(dx*dx+dy*dy+dz*dz)^(1/2)
% I1'=int(f1,y,x,x',y',z'),   f1=1/(dx*dx+dy*dy+dz*dz)^(1/2)
% Oxyz_X, Sxyz_X

function [I1]=FM_3D_A(Oxyz,Sxyz,OBJ)   

% Obj are lines, Oxyz(x1,x2,yo,zo),Sxyz(x1,y1,x2,y2,zs)
% Obj are planes, Oxyz(x1,y1,x2,y2,zo),Sxyz(x1,y1,x2,y2,zs)
% OBJ=1,"line"; OBJ=2,"plane"

[no mo]=size(Oxyz);
[ns ms]=size(Sxyz);

Is=ones(1,ns);
Io=ones(no,1);
xo1=Oxyz(:,1)*Is;
xo2=Oxyz(:,2)*Is;
yo=Oxyz(:,3)*Is;
zo=Oxyz(:,4)*Is;
xs1=Io*Sxyz(:,1)';
xs2=Io*Sxyz(:,3)';
ys1=Io*Sxyz(:,2)';
ys2=Io*Sxyz(:,4)';
zs=Io*Sxyz(:,5)';

x11=xo1-xs1;
x12=xo1-xs2;
x21=xo2-xs1;
x22=xo2-xs2;
y11=yo-ys1;
y12=yo-ys2;
z=zo-zs;

clear Is Io xo1 xo2 xs1 xs2 yo ys1 ys2 zo zs;

I1=0;

% y11
[F1]=Formula_3D_A(x11,y11,z);
I1=I1-F1;

[F1]=Formula_3D_A(x12,y11,z);
I1=I1+F1;

[F1]=Formula_3D_A(x21,y11,z);
I1=I1+F1;

[F1]=Formula_3D_A(x22,y11,z);
I1=I1-F1;

% y12
[F1]=Formula_3D_A(x11,y12,z);
I1=I1+F1;

[F1]=Formula_3D_A(x12,y12,z);
I1=I1-F1;

[F1]=Formula_3D_A(x21,y12,z);
I1=I1-F1;

[F1]=Formula_3D_A(x22,y12,z);
I1=I1+F1;

clear x11 x12 x21 x22 y11 y12 y21 y22 z F1; 
end