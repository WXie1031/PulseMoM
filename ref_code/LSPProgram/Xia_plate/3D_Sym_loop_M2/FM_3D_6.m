% Qy_z (Mz)
%
% I6=int(f6,y,y',z'),   f6=1/(dx*dx+dy*dy+dz*dz)^(1/2)
% 
% OJ(y1,y2,yo,zo)           field lines (Jy)
% Ys(y1,y2), Zs(z1,z2)      source cells (S1)
% xs                        single value
% updated on 15/06/2011
%
function [I6]=FM_3D_6(OJ,xs,Ys,Zs)  
[no mo]=size(OJ);
[ns ms]=size(Ys);
Io=ones(no,1);
Is=ones(1,ns);

xo=OJ(:,1)*Is;
yo1=OJ(:,2)*Is;
yo2=OJ(:,3)*Is;
zo=OJ(:,4)*Is;

ys1=Io*Ys(:,1)';
ys2=Io*Ys(:,2)';
zs1=Io*Zs(:,1)';
zs2=Io*Zs(:,2)';

y11=yo1-ys1;
y12=yo1-ys2;
y21=yo2-ys1;
y22=yo2-ys2;
z11=zo-zs1;
z12=zo-zs2;
dx=xo-xs;
clear Io Is xo yo1 yo2 zo xs1 xs2 ys1 ys2 zs;

I6=0;
% z11
[F6]=Formula_3D_6(dx,y11,z11);
I6=I6-F6;

[F6]=Formula_3D_6(dx,y12,z11);
I6=I6+F6;

[F6]=Formula_3D_6(dx,y21,z11);
I6=I6+F6;

[F6]=Formula_3D_6(dx,y22,z11);
I6=I6-F6;

% z12
[F6]=Formula_3D_6(dx,y11,z12);
I6=I6+F6;

[F6]=Formula_3D_6(dx,y12,z12);
I6=I6-F6;

[F6]=Formula_3D_6(dx,y21,z12);
I6=I6-F6;

[F6]=Formula_3D_6(dx,y22,z12);
I6=I6+F6;

clear z11 z12 y11 y12 y21 y22 dx F6;
end