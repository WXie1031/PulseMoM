% Pmxx  Pmzz
%
% I12=int(f12,x',z'),   f12=-dy/(dx*dx+dy*dy+dz*dz)^(3/2)
% OM(xo,yo,zo)              field lines (M)
% Xs(x1,x2), Zs(y1,y2)      source cells (S2)
% ys                        single value
% updated on 15/06/2011
%
function [I12]=FM_3D_12(OM,Xs,ys,Zs)   
[no mo]=size(OM);
[ns ms]=size(Xs);
Io=ones(no,1);
Is=ones(1,ns);

xo=OM(1:no,1)*Is;
yo=OM(1:no,2)*Is;
zo=OM(1:no,3)*Is;

xs1=Io*Xs(1:ns,1)';
xs2=Io*Xs(1:ns,2)';
zs1=Io*Zs(1:ns,1)';
zs2=Io*Zs(1:ns,2)';

x11=xo-xs1;
x12=xo-xs2;
z11=zo-zs1;
z12=zo-zs2;
dy=yo-ys;
clear Io Is xo yo zo xs1 xs2 ys1 ys2 zs;

I12=0;
[F12]=Formula_3D_12(x11,dy,z11);
I12=I12+F12;

[F12]=Formula_3D_12(x11,dy,z12);
I12=I12-F12;

[F12]=Formula_3D_12(x12,dy,z11);
I12=I12-F12;

[F12]=Formula_3D_12(x12,dy,z12);
I12=I12+F12;

clear x11 x12 y11 y12 dz F12;

end