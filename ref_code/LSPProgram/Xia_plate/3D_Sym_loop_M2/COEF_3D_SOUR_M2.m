%
% OJx(x1,x2,y,z), OJy(x,y1,y2,z) 
% OMx(x,y,z), OMy, OMz  coordinates of field points on sur. for Mx,My,Mz
%
function [Sx,Sy,Bxs,Bys,Bzs]=COEF_3D_SOUR_M2(OMx,OMy,OMz,OJx,OJy,Qs,Is)

Qsx1=[Qs(1,:); Qs(3,:)];        % start point of source x 
Qsx2=[Qs(2,:); Qs(4,:)];        % end point of source x
Qsy1=[Qs(2,:); Qs(4,:)];        % start point of source y
Qsy2=[Qs(3,:); Qs(1,:)];        % end point of source y

% initilization
[nox mo]=size(OMx);
[noy mo]=size(OMy);
[noz mo]=size(OMz);
[noX mox]=size(OJx); 
[noY moy]=size(OJy);

[ns ms]=size(Qs);

Iq=ones(1,ns/2);
Iox=ones(nox,1);
Ioy=ones(noy,1);
Ioz=ones(noz,1);
IoX=ones(noX,1);
IoY=ones(noY,1);

% Sx, for Jx
Xox1=OJx(:,1)*Iq;
Xox2=OJx(:,2)*Iq;
Xoy =OJx(:,3)*Iq;
Xoz =OJx(:,4)*Iq;

Xsx1=IoX*Qsx1(:,1)';
Xsy =IoX*Qsx1(:,2)';
Xsz =IoX*Qsx1(:,3)';
Xsx2=IoX*Qsx2(:,1)';

dx11=Xox1-Xsx1;
dx12=Xox1-Xsx2;
dx21=Xox2-Xsx1;
dx22=Xox2-Xsx2;
dy=Xoy-Xsy;
dz=Xoz-Xsz;

[Fsx]=Formula_3D_Sx(dx11,dy,dz);
Sx=Fsx;
[Fsx]=Formula_3D_Sx(dx12,dy,dz);
Sx=Sx-Fsx;
[Fsx]=Formula_3D_Sx(dx21,dy,dz);
Sx=Sx-Fsx;
[Fsx]=Formula_3D_Sx(dx22,dy,dz);
Sx=Sx+Fsx;

Sx=Sx*[Is;Is];
clear Xox1 Xox2 Xoy Xoz Xsx1 Xsx2 Xsy Xsz;
clear dx11 dx12 dx21 dx22 dy dz Fsx;

% Sy, for Jy
Yox =OJy(:,1)*Iq;
Yoy1=OJy(:,2)*Iq;
Yoy2=OJy(:,3)*Iq;
Yoz =OJy(:,4)*Iq;

Ysx =IoY*Qsy1(:,1)';
Ysy1=IoY*Qsy1(:,2)';
Ysz =IoY*Qsy1(:,3)';
Ysy2=IoY*Qsy2(:,2)';
 
dx=Yox-Ysx;
dy11=Yoy1-Ysy1;
dy12=Yoy1-Ysy2;
dy21=Yoy2-Ysy1;
dy22=Yoy2-Ysy2;
dz=Yoz-Ysz;

[Fsy]=Formula_3D_Sy(dx,dy11,dz);
Sy=Fsy;
[Fsy]=Formula_3D_Sy(dx,dy12,dz);
Sy=Sy-Fsy;
[Fsy]=Formula_3D_Sy(dx,dy21,dz);
Sy=Sy-Fsy;
[Fsy]=Formula_3D_Sy(dx,dy22,dz);
Sy=Sy+Fsy;

Sy=Sy*[Is;Is];
clear Yox Yoy1 Yoy2 Yoz Ysx Ysy1 Ysy2 Ysz;
clear dx dy11 dy12 dy21 dy22 dz Fsy;

% Bsx, for Mx
xox=OMx(:,1)*Iq; 
xoy=OMx(:,2)*Iq; 
xoz=OMx(:,3)*Iq; 

xsx =Iox*Qsy1(:,1)';
xsy1=Iox*Qsy1(:,2)';
xsz =Iox*Qsy1(:,3)';
xsy2=Iox*Qsy2(:,2)';  

dx= xox-xsx;
dy1=xoy-xsy1;
dy2=xoy-xsy2;
dz= xoz-xsz;

[Fsx_Y]=Formula_3D_Bsx(dx,dy1,dz);
Bx_Y=Fsx_Y;
[Fsx_Y]=Formula_3D_Bsx(dx,dy2,dz);
Bx_Y=-Bx_Y+Fsx_Y;

Bxs=Bx_Y*[Is;Is];
clear xox xoy xoz xsx1 xsx2 xsy xsz;
clear dx1 dx2 dy dz Fsx_Y Bx_Y;

% for My
yox=OMy(:,1)*Iq; 
yoy=OMy(:,2)*Iq; 
yoz=OMy(:,3)*Iq; 

ysx1=Ioy*Qsx1(:,1)';
ysy =Ioy*Qsx1(:,2)';
ysz =Ioy*Qsx1(:,3)';
ysx2=Ioy*Qsx2(:,1)';

dx1=yox-ysx1;
dx2=yox-ysx2;
dy= yoy-ysy;
dz= yoz-ysz;

[Fsy_X]=Formula_3D_Bsy(dx1,dy,dz);
By_X=Fsy_X;
[Fsy_X]=Formula_3D_Bsy(dx2,dy,dz);
By_X=-By_X+Fsy_X;

Bys=By_X*[Is;Is];
clear yox yoy yoz ysx ysy1 ysy2 ysz;
clear dx dy1 dy2 dz Fsy_X By_X;

% Bsz, for Mz
zox=OMz(:,1)*Iq; 
zoy=OMz(:,2)*Iq; 
zoz=OMz(:,3)*Iq; 

zsx1=Ioz*Qsx1(:,1)';
zsy =Ioz*Qsx1(:,2)';
zszu=Ioz*Qsx1(:,3)';
zsx2=Ioz*Qsx2(:,1)';

zsx =Ioz*Qsy1(:,1)';
zsy1=Ioz*Qsy1(:,2)';
zszv=Ioz*Qsy1(:,3)';
zsy2=Ioz*Qsy2(:,2)';  

dux1=zox-zsx1;
dux2=zox-zsx2;
duy=zoy-zsy;
duz=zoz-zszu;

dvx=zox-zsx;
dvy1=zoy-zsy1;
dvy2=zoy-zsy2;
dvz=zoz-zszv;

[Fsz_X,Fsz_Y]=Formula_3D_Bsz(dux1,duy,duz,dvx,dvy1,dvz);
Bz_X=Fsz_X;
Bz_Y=Fsz_Y;
[Fsz_X,Fsz_Y]=Formula_3D_Bsz(dux2,duy,duz,dvx,dvy2,dvz);
Bz_X=-Bz_X+Fsz_X;
Bz_Y=-Bz_Y+Fsz_Y;

Bzs=(Bz_X+Bz_Y)*[Is;Is];

clear zox zoy zoz zsx1 zsx2 zsy zszu zsx zsy1 zsy2 zszv;
clear dux1 dux2 duy duz dvx dvy1 dvy2 dvz;
clear Fsz_X Fsz_Y Bz_X Bz_Y;
end
