% Pmzz (Mz)
%
% I13=int(f13,x',y'),   f13=3*dz*dz/(dx*dx+dy*dy+dz*dz)^(5/2)-1/(dx*dx+dy*dy+dz*dz)^(3/2)
% Oxyz(xo,yo,zo),Sxyz(x1,y1,x2,y2,zs)
%
% updated on 11/03/2011
%
function [I13]=FM_3D_13(Oxyz,Sxyz)   

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

I13=0;

[F13]=Formula_3D_13(x11,y11,dz);
I13=I13+F13; 

[F13]=Formula_3D_13(x11,y12,dz);
I13=I13-F13; 

[F13]=Formula_3D_13(x12,y11,dz);
I13=I13-F13;

[F13]=Formula_3D_13(x12,y12,dz);
I13=I13+F13;

clear x11 x12 y11 y12 dz F13;

end