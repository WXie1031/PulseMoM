% Bz_x (Jx)
%
% I9=int(f9,x',y',z'),   f9=dy/(dx*dx+dy*dy+dz*dz)^(3/2)
% Oxyz(xo,yo,zo),Sxyz(x1,y1,x2,y2,zs)
%
% updated on 15/02/2011
%
function [I9]=FM_3D_9(Oxyz,Sxyz)   

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

I9=0;

% x11
[F9]=Formula_3D_9(x11,y11,dz);
I9=I9+F9;

[F9]=Formula_3D_9(x11,y12,dz);
I9=I9-F9;

% x12
[F9]=Formula_3D_9(x12,y11,dz);
I9=I9-F9;

[F9]=Formula_3D_9(x12,y12,dz);
I9=I9+F9;

clear x11 x12 y11 y12 dz F9;

end