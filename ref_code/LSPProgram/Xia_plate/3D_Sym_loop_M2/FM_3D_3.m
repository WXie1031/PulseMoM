% Qx_z (Mz)
%
% I3=int(f3,x,x',z'),   f2=1/(dx*dx+dy*dy+dz*dz)^(1/2)
% 
% OJ(x1,x2,yo,zo)           field lines (Jx)
% Xs(x1,x2), Zs(y1,y2)      source cells (S2)
% ys                        single value
% updated on 15/06/2011
%
function [I3]=FM_3D_3(OJ,Xs,ys,Zs)   
[no mo]=size(OJ);
[ns ms]=size(Xs);
Io=ones(no,1);
Is=ones(1,ns);

xo1=OJ(:,1)*Is;
xo2=OJ(:,2)*Is;
yo=OJ(:,3)*Is;
zo=OJ(:,4)*Is;

xs1=Io*Xs(:,1)';
xs2=Io*Xs(:,2)';
zs1=Io*Zs(:,1)';
zs2=Io*Zs(:,2)';

x11=xo1-xs1;
x12=xo1-xs2;
x21=xo2-xs1;
x22=xo2-xs2;
z11=zo-zs1;
z12=zo-zs2;
dy=yo-ys;
clear Io Is xo1 xo2 yo zo xs1 xs2 zs1 zs2 ys;

I3=0;
% z11
[F3]=Formula_3D_3(x11,dy,z11);
I3=I3-F3;

[F3]=Formula_3D_3(x12,dy,z11);
I3=I3+F3;

[F3]=Formula_3D_3(x21,dy,z11);
I3=I3+F3;

[F3]=Formula_3D_3(x22,dy,z11);
I3=I3-F3;

% z12
[F3]=Formula_3D_3(x11,dy,z12);
I3=I3+F3;

[F3]=Formula_3D_3(x12,dy,z12);
I3=I3-F3;

[F3]=Formula_3D_3(x21,dy,z12);
I3=I3-F3;

[F3]=Formula_3D_3(x22,dy,z12);
I3=I3+F3;

clear x11 x12 x21 x22 z11 z12 dy F3; 
end