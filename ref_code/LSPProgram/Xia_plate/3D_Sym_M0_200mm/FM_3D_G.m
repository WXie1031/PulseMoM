
% updated on 13/08/2010

% Pmxx Pmyx Pmzx (Mx), Pmxy Pmyy Pmzy (My), Pmxz Pmyz Pmzz (Mz),

% I11=int(f11,x',y',z'),   f11=3*dx*dx/(dx*dx+dy*dy+dz*dz)^(5/2)-1/(dx*dx+dy*dy+dz*dz)^(3/2)
% I12=int(f12,x',y',z'),   f12=3*dy*dy/(dx*dx+dy*dy+dz*dz)^(5/2)-1/(dx*dx+dy*dy+dz*dz)^(3/2)
% I13=int(f13,x',y',z'),   f13=3*dz*dz/(dx*dx+dy*dy+dz*dz)^(5/2)-1/(dx*dx+dy*dy+dz*dz)^(3/2)
% I14=int(f14,x',y',z'),   f14=3*dx*dy/(dx*dx+dy*dy+dz*dz)^(5/2)
% I15=int(f15,x',y',z'),   f15=3*dy*dz/(dx*dx+dy*dy+dz*dz)^(5/2)
% I16=int(f16,x',y',z'),   f16=3*dz*dx/(dx*dx+dy*dy+dz*dz)^(5/2)

% Oxyz, Sxyz

function [I11,I12,I13,I14,I15,I16]=FM_3D_G(Oxyz,Sxyz)   
% Oxyz(xo,yo,zo),Sxyz(x1,y1,z1,x2,y2,z2)

[no mo]=size(Oxyz);
[ns ms]=size(Sxyz);

Is=ones(1,ns);
Io=ones(no,1);
xo=Oxyz(:,1)*Is;
yo=Oxyz(:,2)*Is;
zo=Oxyz(:,3)*Is;
xs1=Io*Sxyz(:,1)';
xs2=Io*Sxyz(:,4)';
ys1=Io*Sxyz(:,2)';
ys2=Io*Sxyz(:,5)';
zs1=Io*Sxyz(:,3)';
zs2=Io*Sxyz(:,6)';

x11=xo-xs1;
x12=xo-xs2;
y11=yo-ys1;
y12=yo-ys2;
z11=zo-zs1;
z12=zo-zs2;

clear Is Io xo xs1 xs2 yo ys1 ys2 zo zs1 zs2;

I11=0; I12=0; I13=0; I14=0; I15=0; I16=0;

% x11
[F11,F12,F13,F14,F15,F16]=Formula_3D_G(x11,y11,z11);
I11=I11-F11; I12=I12-F12; I13=I13-F13; 
I14=I14-F14; I15=I15-F15; I16=I16-F16;

[F11,F12,F13,F14,F15,F16]=Formula_3D_G(x11,y11,z12);
I11=I11+F11; I12=I12+F12; I13=I13+F13; 
I14=I14+F14; I15=I15+F15; I16=I16+F16;

[F11,F12,F13,F14,F15,F16]=Formula_3D_G(x11,y12,z11);
I11=I11+F11; I12=I12+F12; I13=I13+F13; 
I14=I14+F14; I15=I15+F15; I16=I16+F16;

[F11,F12,F13,F14,F15,F16]=Formula_3D_G(x11,y12,z12);
I11=I11-F11; I12=I12-F12; I13=I13-F13; 
I14=I14-F14; I15=I15-F15; I16=I16-F16;

% x12
[F11,F12,F13,F14,F15,F16]=Formula_3D_G(x12,y11,z11);
I11=I11+F11; I12=I12+F12; I13=I13+F13; 
I14=I14+F14; I15=I15+F15; I16=I16+F16;

[F11,F12,F13,F14,F15,F16]=Formula_3D_G(x12,y11,z12);
I11=I11-F11; I12=I12-F12; I13=I13-F13; 
I14=I14-F14; I15=I15-F15; I16=I16-F16;

[F11,F12,F13,F14,F15,F16]=Formula_3D_G(x12,y12,z11);
I11=I11-F11; I12=I12-F12; I13=I13-F13; 
I14=I14-F14; I15=I15-F15; I16=I16-F16;

[F11,F12,F13,F14,F15,F16]=Formula_3D_G(x12,y12,z12);
I11=I11+F11; I12=I12+F12; I13=I13+F13; 
I14=I14+F14; I15=I15+F15; I16=I16+F16;

clear x11 x12 y11 y12 z11 z12 F11 F12 F13 F14 F15 F16;

end