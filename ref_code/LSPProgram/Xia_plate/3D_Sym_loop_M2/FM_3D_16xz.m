% Pmyz (Mz)
%
% I16=int(f16,x',z'),   f16=dz/(dx*dx+dy*dy+dz*dz)^(3/2)
% OM(xo,yo,zo)              field lines (M)
% Xs(x1,x2), Zs(z1,z2)      source cells (S2)
% ys                        single value
% updated on 15/06/2011
%
function [I16]=FM_3D_16xz(OM,Xs,ys,Zs)  
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
clear Io Is xo yo zo xs1 xs2 ys1 ys2 zs1 zs2;

I16=0; 
[F16]=Formula_3D_16(x11,dy,z11);
I16=I16+F16;

[F16]=Formula_3D_16(x11,dy,z12);
I16=I16-F16;

[F16]=Formula_3D_16(x12,dy,z11);
I16=I16-F16;

[F16]=Formula_3D_16(x12,dy,z12);
I16=I16+F16;
end