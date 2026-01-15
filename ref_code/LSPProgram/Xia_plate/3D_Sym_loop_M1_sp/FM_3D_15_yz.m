% Pmyz (Mz)
%
% I15=int(f15,x',y',z'),   f15=3*dy*dz/(dx*dx+dy*dy+dz*dz)^(5/2)
% Oxyz(xo,yo,zo),Sxyz(x1,y1,z1,x2,y2,z2)
%
% updated on 16/02/2011
%
function [I15]=FM_3D_15_yz(Oxyz,Sxyz)  

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

I15=0; 

% x11
[F15]=Formula_3D_15_yz(x11,y11,z11);
I15=I15-F15;

[F15]=Formula_3D_15_yz(x11,y11,z12);
I15=I15+F15;

[F15]=Formula_3D_15_yz(x11,y12,z11);
I15=I15+F15;

[F15]=Formula_3D_15_yz(x11,y12,z12);
I15=I15-F15;

% x12
[F15]=Formula_3D_15_yz(x12,y11,z11);
I15=I15+F15;

[F15]=Formula_3D_15_yz(x12,y11,z12);
I15=I15-F15;

[F15]=Formula_3D_15_yz(x12,y12,z11);
I15=I15-F15;

[F15]=Formula_3D_15_yz(x12,y12,z12);
I15=I15+F15;

clear x11 x12 y11 y12 z11 z12 F15;

end