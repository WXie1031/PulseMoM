%
% Oxy(xo,yo),Oxy_X(xo1,xo2,yo),Oxy_Y(xo,yo1,yo2),zo(zo1,zo2)
%
function [Sx,Sy,Bxs,Bys,Bzs1,Bzs0]=COEF_3D_SOUR(Oxy,Oxy_X,Oxy_Y,Oxy_Z,Oxyz_Z,zo,Qs,Is)

Qsx1=[Qs(1,:); Qs(3,:)];        % start point of source x 
Qsx2=[Qs(2,:); Qs(4,:)];        % end point of source x
Qsy1=[Qs(2,:); Qs(4,:)];        % start point of source y
Qsy2=[Qs(3,:); Qs(1,:)];        % end point of source y

% initilization
[no mo]=size(Oxy);
[nox mox]=size(Oxy_X); 
[noy moy]=size(Oxy_Y);
[noz1 moz]=size(Oxy_Z);
[noz0 moz]=size(Oxyz_Z);
[ns ms]=size(Qs);

Iq=ones(1,ns/2);
Io=ones(no,1);
Iox=ones(nox,1);
Ioy=ones(noy,1);
Ioz1=ones(noz1,1);
Ioz0=ones(noz0,1);

% for Mx My
xo=Oxy(:,1)*Iq; 
yo=Oxy(:,2)*Iq; 
zo1=zo(1)*Io*Iq; 
zo2=zo(2)*Io*Iq;

ux1=Io*Qsx1(:,1)';
uy=Io*Qsx1(:,2)';
uz=Io*Qsx1(:,3)';
ux2=Io*Qsx2(:,1)';  
vx=Io*Qsy1(:,1)';
vy1=Io*Qsy1(:,2)';
vz=Io*Qsy1(:,3)';
vy2=Io*Qsy2(:,2)';  

% for Mz(M1)
xo_Z1=Oxy_Z(:,1)*Iq; 
yo_Z1=Oxy_Z(:,2)*Iq; 
zo1_Z1=zo(1)*Ioz1*Iq; 
zo2_Z1=zo(2)*Ioz1*Iq;

ux1_Z1=Ioz1*Qsx1(:,1)';
uy_Z1=Ioz1*Qsx1(:,2)';
uz_Z1=Ioz1*Qsx1(:,3)';
ux2_Z1=Ioz1*Qsx2(:,1)';  
vx_Z1=Ioz1*Qsy1(:,1)';
vy1_Z1=Ioz1*Qsy1(:,2)';
vz_Z1=Ioz1*Qsy1(:,3)';
vy2_Z1=Ioz1*Qsy2(:,2)';

% for Mz(M0)
xo_Z0=Oxyz_Z(:,1)*Iq; 
yo_Z0=Oxyz_Z(:,2)*Iq; 
zo_Z0=Oxyz_Z(:,3)*Iq; 

ux1_Z0=Ioz0*Qsx1(:,1)';
uy_Z0=Ioz0*Qsx1(:,2)';
uz_Z0=Ioz0*Qsx1(:,3)';
ux2_Z0=Ioz0*Qsx2(:,1)';  
vx_Z0=Ioz0*Qsy1(:,1)';
vy1_Z0=Ioz0*Qsy1(:,2)';
vz_Z0=Ioz0*Qsy1(:,3)';
vy2_Z0=Ioz0*Qsy2(:,2)';  

% for Jx
xo1_X=Oxy_X(:,1)*Iq;
xo2_X=Oxy_X(:,2)*Iq;
yo_X=Oxy_X(:,3)*Iq;
zo1_X=zo(1)*Iox*Iq;
zo2_X=zo(2)*Iox*Iq;

ux1_X=Iox*Qsx1(:,1)';
uy_X=Iox*Qsx1(:,2)';
uz_X=Iox*Qsx1(:,3)';
ux2_X=Iox*Qsx2(:,1)';

% for Jy
xo_Y=Oxy_Y(:,1)*Iq;
yo1_Y=Oxy_Y(:,2)*Iq;
yo2_Y=Oxy_Y(:,3)*Iq;
zo1_Y=zo(1)*Ioy*Iq;
zo2_Y=zo(2)*Ioy*Iq; 

vx_Y=Ioy*Qsy1(:,1)';
vy1_Y=Ioy*Qsy1(:,2)';
vz_Y=Ioy*Qsy1(:,3)';
vy2_Y=Ioy*Qsy2(:,2)';

clear Iq Io Iox Ioy Ioz;

% Sx
dx11=xo1_X-ux1_X;
dx12=xo1_X-ux2_X;
dx21=xo2_X-ux1_X;
dx22=xo2_X-ux2_X;
dy=yo_X-uy_X;
dz1=zo1_X-uz_X;
dz2=zo2_X-uz_X;

[Fsx]=Formula_3D_Sx(dx11,dy,dz1);
Sx1=Fsx;
[Fsx]=Formula_3D_Sx(dx12,dy,dz1);
Sx1=Sx1-Fsx;
[Fsx]=Formula_3D_Sx(dx21,dy,dz1);
Sx1=Sx1-Fsx;
[Fsx]=Formula_3D_Sx(dx22,dy,dz1);
Sx1=Sx1+Fsx;

[Fsx]=Formula_3D_Sx(dx11,dy,dz2);
Sx2=Fsx;
[Fsx]=Formula_3D_Sx(dx12,dy,dz2);
Sx2=Sx2-Fsx;
[Fsx]=Formula_3D_Sx(dx21,dy,dz2);
Sx2=Sx2-Fsx;
[Fsx]=Formula_3D_Sx(dx22,dy,dz2);
Sx2=Sx2+Fsx;

Sx=[Sx1;Sx2]*[Is;Is];
clear xo1_X xo2_X yo_X zo1_X zo2_X;
clear ux1_X uy_X uz_X ux2_X;
clear dx11 dx12 dx21 dx22 dy dz1 dz2;

% Sy
dx=xo_Y-vx_Y;
dy11=yo1_Y-vy1_Y;
dy12=yo1_Y-vy2_Y;
dy21=yo2_Y-vy1_Y;
dy22=yo2_Y-vy2_Y;
dz1=zo1_Y-vz_Y;
dz2=zo2_Y-vz_Y;

[Fsy]=Formula_3D_Sy(dx,dy11,dz1);
Sy1=Fsy;
[Fsy]=Formula_3D_Sy(dx,dy12,dz1);
Sy1=Sy1-Fsy;
[Fsy]=Formula_3D_Sy(dx,dy21,dz1);
Sy1=Sy1-Fsy;
[Fsy]=Formula_3D_Sy(dx,dy22,dz1);
Sy1=Sy1+Fsy;

[Fsy]=Formula_3D_Sy(dx,dy11,dz2);
Sy2=Fsy;
[Fsy]=Formula_3D_Sy(dx,dy12,dz2);
Sy2=Sy2-Fsy;
[Fsy]=Formula_3D_Sy(dx,dy21,dz2);
Sy2=Sy2-Fsy;
[Fsy]=Formula_3D_Sy(dx,dy22,dz2);
Sy2=Sy2+Fsy;

Sy=[Sy1;Sy2]*[Is;Is];
clear xo_Y yo1_Y yo2_Y zo1_Y zo2_Y;
clear vx_Y vy1_Y vz_Y vy2_Y;
clear dx dy11 dy12 dy21 dy22 dz1 dz2;

% Bsx Bsy
dux1=xo-ux1;
dux2=xo-ux2;
duy=yo-uy;
duz1=zo1-uz;
duz2=zo2-uz;
dvx=xo-vx;
dvy1=yo-vy1;
dvy2=yo-vy2;
dvz1=zo1-vz;
dvz2=zo2-vz;

[Fsx_Y]=Formula_3D_Bsx(dvx,dvy1,dvz1);
Bx_Y1=Fsx_Y;
[Fsx_Y]=Formula_3D_Bsx(dvx,dvy2,dvz1);
Bx_Y1=-Bx_Y1+Fsx_Y;

[Fsx_Y]=Formula_3D_Bsx(dvx,dvy1,dvz2);
Bx_Y2=Fsx_Y;
[Fsx_Y]=Formula_3D_Bsx(dvx,dvy2,dvz2);
Bx_Y2=-Bx_Y2+Fsx_Y;

[Fsy_X]=Formula_3D_Bsy(dux1,duy,duz1);
By_X1=Fsy_X;
[Fsy_X]=Formula_3D_Bsy(dux2,duy,duz1);
By_X1=-By_X1+Fsy_X;

[Fsy_X]=Formula_3D_Bsy(dux1,duy,duz2);
By_X2=Fsy_X;
[Fsy_X]=Formula_3D_Bsy(dux2,duy,duz2);
By_X2=-By_X2+Fsy_X;
clear xo yo zo1 zo2;
clear ux1 uy uz ux2 vx vy1 vz vy2; 

% Bsz1
dux1=xo_Z1-ux1_Z1;
dux2=xo_Z1-ux2_Z1;
duy=yo_Z1-uy_Z1;
duz1=zo1_Z1-uz_Z1;
duz2=zo2_Z1-uz_Z1;

dvx=xo_Z1-vx_Z1;
dvy1=yo_Z1-vy1_Z1;
dvy2=yo_Z1-vy2_Z1;
dvz1=zo1_Z1-vz_Z1;
dvz2=zo2_Z1-vz_Z1;

[Fsz_X,Fsz_Y]=Formula_3D_Bsz(dux1,duy,duz1,dvx,dvy1,dvz1);
Bz_X1=Fsz_X;
Bz_Y1=Fsz_Y;
[Fsz_X,Fsz_Y]=Formula_3D_Bsz(dux2,duy,duz1,dvx,dvy2,dvz1);
Bz_X1=-Bz_X1+Fsz_X;
Bz_Y1=-Bz_Y1+Fsz_Y;

[Fsz_X,Fsz_Y]=Formula_3D_Bsz(dux1,duy,duz2,dvx,dvy1,dvz2);
Bz_X2=Fsz_X;
Bz_Y2=Fsz_Y;
[Fsz_X,Fsz_Y]=Formula_3D_Bsz(dux2,duy,duz2,dvx,dvy2,dvz2);
Bz_X2=-Bz_X2+Fsz_X;
Bz_Y2=-Bz_Y2+Fsz_Y;

% Bsz0
dux1=xo_Z0-ux1_Z0;
dux2=xo_Z0-ux2_Z0;
duy=yo_Z0-uy_Z0;
duz=zo_Z0-uz_Z0;

dvx=xo_Z0-vx_Z0;
dvy1=yo_Z0-vy1_Z0;
dvy2=yo_Z0-vy2_Z0;
dvz=zo_Z0-vz_Z0;

[Fsz_X,Fsz_Y]=Formula_3D_Bsz(dux1,duy,duz,dvx,dvy1,dvz);
Bz_X0=Fsz_X;
Bz_Y0=Fsz_Y;
[Fsz_X,Fsz_Y]=Formula_3D_Bsz(dux2,duy,duz,dvx,dvy2,dvz);
Bz_X0=-Bz_X0+Fsz_X;
Bz_Y0=-Bz_Y0+Fsz_Y;

Bxs=[Bx_Y1;Bx_Y2]*[Is;Is];
Bys=[By_X1;By_X2]*[Is;Is];
Bzs1=[Bz_X1+Bz_Y1;Bz_X2+Bz_Y2]*[Is;Is];
Bzs0=[Bz_X0+Bz_Y0]*[Is;Is];

clear xo_Z yo_Z zo1_Z zo2_Z;
clear ux1_Z uy_Z uz_Z ux2_Z vx_Z vy1_Z vz_Z vy2_Z;
clear dux1 dux2 duy duz1 duz2 dvx dvy1 dvy2 dvz1 dvz2;
clear Fsx Fsy Fsy_X Fsx_Y Fsz_X Fsz_Y By_X1 By_X2 Bx_Y1 Bx_Y2 Bz_Y1 Bz_X1 Bz_Y2 Bz_X2;
end
