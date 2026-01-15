% Pmxx  Pmyy
%
% I13=int(f13,x',y'),   f13=-dz/(dx*dx+dy*dy+dz*dz)^(3/2)
% OM(xo,yo,zo)              field lines (M)
% Xs(x1,x2), Ys(y1,y2)      source cells (S3)
% zs                        single value
% updated on 15/06/2011
%
function [I13]=FM_3D_13(OM,Xs,Ys,zs)   
[no mo]=size(OM);
[ns ms]=size(Xs);
Io=ones(no,1);
Is=ones(1,ns);

xo=OM(1:no,1)*Is;
yo=OM(1:no,2)*Is;
zo=OM(1:no,3)*Is;

xs1=Io*Xs(1:ns,1)';
xs2=Io*Xs(1:ns,2)';
ys1=Io*Ys(1:ns,1)';
ys2=Io*Ys(1:ns,2)';

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