% Pmzz (Mz)
%
% I13=int(f13,x',y',z'),   f13=3*dz*dz/(dx*dx+dy*dy+dz*dz)^(5/2)-1/(dx*dx+dy*dy+dz*dz)^(3/2)
% Oxyz(xo,yo,zo),Sxyz(x1,y1,z1,x2,y2,z2)
%
% updated on 15/02/2011
%
function [I13]=FM_3D_13_0(Oxyz,Sxyz)   

[no mo]=size(Oxyz);
[ns ms]=size(Sxyz);
Io=ones(no,1);
Is=ones(1,ns);

xo=Oxyz(:,1)*Is;
yo=Oxyz(:,2)*Is;
zo=Oxyz(:,3)*Is;

xs1=Io*Sxyz(:,1)';
xs2=Io*Sxyz(:,2)';
ys1=Io*Sxyz(:,3)';
ys2=Io*Sxyz(:,4)';
zs1=Io*Sxyz(:,5)';
zs2=Io*Sxyz(:,6)';

x11=xo-xs1;
x12=xo-xs2;
y11=yo-ys1;
y12=yo-ys2;
z11=zo-zs1;
z12=zo-zs2;

clear Io Is xo yo zo xs1 xs2 ys1 ys2 zs1 zs2;

I13=0;

% x11
[F13]=Formula_3D_13_0(x11,y11,z11);
I13=I13-F13;

[F13]=Formula_3D_13_0(x11,y11,z12);
I13=I13+F13; 

[F13]=Formula_3D_13_0(x11,y12,z11);
I13=I13+F13; 

[F13]=Formula_3D_13_0(x11,y12,z12);
I13=I13-F13; 

% x12
[F13]=Formula_3D_13_0(x12,y11,z11);
I13=I13+F13; 

[F13]=Formula_3D_13_0(x12,y11,z12);
I13=I13-F13;

[F13]=Formula_3D_13_0(x12,y12,z11);
I13=I13-F13;

[F13]=Formula_3D_13_0(x12,y12,z12);
I13=I13+F13;

clear x11 x12 y11 y12 z11 z12 F13;

end