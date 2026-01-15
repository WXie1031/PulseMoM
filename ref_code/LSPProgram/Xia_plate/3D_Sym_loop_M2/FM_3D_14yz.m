% Pmxy (My)
%
% I14=int(f14,y',z'),   f14=dy/(dx*dx+dy*dy+dz*dz)^(3/2)
% OM(xo,yo,zo)              field lines (M)
% Ys(y1,y2), Zs(z1,z2)      source cells (S1)
% xs                        single value
% updated on 15/06/2011
%
function [I14]=FM_3D_14yz(OM,xs,Ys,Zs)   
[no mo]=size(OM);
[ns ms]=size(Ys);
Io=ones(no,1);
Is=ones(1,ns);

xo=OM(1:no,1)*Is;
yo=OM(1:no,2)*Is;
zo=OM(1:no,3)*Is;

ys1=Io*Ys(1:ns,1)';
ys2=Io*Ys(1:ns,2)';
zs1=Io*Zs(1:ns,1)';
zs2=Io*Zs(1:ns,2)';

y11=yo-ys1;
y12=yo-ys2;
z11=zo-zs1;
z12=zo-zs2;
dx=xo-xs;
clear Io Is xo yo zo xs1 xs2 ys1 ys2 zs;

I14=0;
[F14]=Formula_3D_14(dx,y11,z11);
I14=I14+F14;

[F14]=Formula_3D_14(dx,y11,z12);
I14=I14-F14;

[F14]=Formula_3D_14(dx,y12,z11);
I14=I14-F14;

[F14]=Formula_3D_14(dx,y12,z12);
I14=I14+F14;

clear x11 x12 y11 y12 dz F14;

end