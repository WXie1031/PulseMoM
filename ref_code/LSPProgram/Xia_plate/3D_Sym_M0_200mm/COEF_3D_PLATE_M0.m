%
function [Lx Qx_y Qx_z Ly Qy_x Qy_z Bx_y By_x Bz_x Bz_y Pmxx Pmxy Pmxz Pmyx Pmyy Pmyz Pmzx Pmzy Pmzz]=COEF_3D_PLATE_M0(Oxyz,Oxyz_X,Oxyz_Y,Oxyz_Z,Sxyz,Sxyz_X,Sxyz_Y,Sxyz_Z,Nx,Ny,NXx,NXy,NYx,NYy,NZx,NZy,Nz)
n=12; 
t=[-0.981560634246719 -0.904117256370475 -0.769902674194305 -0.587317954286617 -0.367831498998180 -0.125233408511469 0.125233408511469 0.367831498998180 0.587317954286617 0.769902674194305 0.904117256370475 0.981560634246719];
A=[0.047175336386512 0.106939325995318 0.160078328543346 0.203167426723066 0.233492536538355 0.249147045813403 0.249147045813403 0.233492536538355 0.203167426723066 0.160078328543346 0.106939325995318 0.047175336386512];

Nxc=Nx/2; Nyc=Ny/2;
NXxc=fix(NXx/2); NXyc=NXy/2;
NYxc=NYx/2; NYyc=fix(NYy/2);
NZxc=NZx/2; NZyc=NZy/2;

% initilization
Lx=0; Qx_y=0; Qx_z=0;
Ly=0; Qy_x=0; Qy_z=0;

Bx_y=0; 
By_x=0; 
Bz_x=0; Bz_y=0; 

[I11,I12,I13,I14,I15,I16]=FM_3D_G(Oxyz,Sxyz);
Pmxx=I11; Pmxy=I14; Pmyy=I12;
[I11,I12,I13,I14,I15,I16]=FM_3D_G(Oxyz_Z,Sxyz);
Pmzx=I16; Pmzy=I15;
[I11,I12,I13,I14,I15,I16]=FM_3D_G(Oxyz,Sxyz_Z);
Pmxz=I16; Pmyz=I15;
[I11,I12,I13,I14,I15,I16]=FM_3D_G(Oxyz_Z,Sxyz_Z);
Pmzz=I13;  

clear I11 I12 I13 I14 I15 I16;

% Oxyz(xo,yo,zo),Sxyz(x1,y1,z1,x2,y2,z2)
% Oxyz_X(xo1,xo2,yo,zo),Sxyz_X(x1,y1,z1,x2,y2,z2)
% Oxyz_Y(xo,yo1,yo2,zo),Sxyz_Y(x1,y1,z1,x2,y2,z2)
[NN,Def]=size(Oxyz);
[NX,Def]=size(Oxyz_X);
[NY,Def]=size(Oxyz_Y);
[NZ,Def]=size(Oxyz_Z);
z1=Sxyz(:,3); z2=Sxyz(:,6); tp=(z2-z1)/2;
zx1=Sxyz_X(:,3); zx2=Sxyz_X(:,6); tpx=(zx2-zx1)/2;
zy1=Sxyz_Y(:,3); zy2=Sxyz_Y(:,6); tpy=(zy2-zy1)/2;
zz1=Sxyz_Z(:,3); zz2=Sxyz_Z(:,6); tpz=(zz2-zz1)/2;

one=ones(NN,1);
Tp_x=one*tpx';          % By_x
Tp_y=one*tpy';          % Bx_y

onex=ones(NX,1);
TpX=onex*tp';           % Qx_y
TpxX=onex*tpx';         % Lx
TpzX=onex*tpz';         % Qx_z

oney=ones(NY,1);        
TpY=oney*tp';           % Qy_x
TpyY=oney*tpy';         % Ly
TpzY=oney*tpz';         % Qy_z

onez=ones(NZ,1);        
TpxZ=onez*tpx';         % Bz_x
TpyZ=onez*tpy';         % Bz_y

clear tp tpx tpy tpz one onex oney onez;

Sxy=[Sxyz(:,1) Sxyz(:,2) Sxyz(:,4) Sxyz(:,5)];
Sxy_X=[Sxyz_X(:,1) Sxyz_X(:,2) Sxyz_X(:,4) Sxyz_X(:,5)];
Sxy_Y=[Sxyz_Y(:,1) Sxyz_Y(:,2) Sxyz_Y(:,4) Sxyz_Y(:,5)];
Sxy_Z=[Sxyz_Z(:,1) Sxyz_Z(:,2) Sxyz_Z(:,4) Sxyz_Z(:,5)];

for i=1:n
    zs=0.5*(z2-z1)*t(i)+0.5*(z2+z1); 
    zsx=0.5*(zx2-zx1)*t(i)+0.5*(zx2+zx1);
    zsy=0.5*(zy2-zy1)*t(i)+0.5*(zy2+zy1);
    zsz=0.5*(zz2-zz1)*t(i)+0.5*(zz2+zz1);
    Sxyz=[Sxy zs];
    Sxyz_X=[Sxy_X zsx];
    Sxyz_Y=[Sxy_Y zsy];
    Sxyz_Z=[Sxy_Z zsz];
    
    [I1]=FM_3D_A(Oxyz_X,Sxyz_X,1);
    Lx=I1*A(i).*TpxX+Lx;
    clear I1;
    
    [I2,I3]=FM_3D_B(Oxyz_X,Sxyz,1);    
    Qx_y=I2*A(i).*TpX+Qx_y;
    [I2,I3]=FM_3D_B(Oxyz_X,Sxyz_Z,1);
    Qx_z=I3*A(i).*TpzX+Qx_z;
    clear I2 I3;
    
    [I4]=FM_3D_C(Oxyz_Y,Sxyz_Y,1);
    Ly=I4*A(i).*TpyY+Ly;
    clear I4;
    
    [I5,I6]=FM_3D_D(Oxyz_Y,Sxyz,1);
    Qy_x=I5*A(i).*TpY+Qy_x;
    [I5,I6]=FM_3D_D(Oxyz_Y,Sxyz_Z,1);
    Qy_z=I6*A(i).*TpzY+Qy_z;
    clear I5 I6;
    
    [I7,I10]=FM_3D_E(Oxyz,Sxyz_Y);
    Bx_y=I7*A(i).*Tp_y+Bx_y;
    [I7,I10]=FM_3D_E(Oxyz_Z,Sxyz_Y);
    Bz_y=I10*A(i).*TpyZ+Bz_y;
    clear I7 I10;
    
    [I8,I9]=FM_3D_F(Oxyz,Sxyz_X);
    By_x=I8*A(i).*Tp_x+By_x;
    [I8,I9]=FM_3D_F(Oxyz_Z,Sxyz_X);
    Bz_x=I9*A(i).*TpxZ+Bz_x;
    clear I8 I9;
end
Pmyx=Pmxy;
  
Nxy=Nxc*Nyc;
NXxy=NXxc*NXyc;
NYxy=NYxc*NYyc;
NZxy=NZxc*NZyc;
NN=Nxy*Nz;
NX=NXxy*Nz;
NY=NYxy*Nz;
NZ=NZxy*Nz;

Lx=[Lx(:,1:NX)+Lx(:,NX+1:2*NX)-Lx(:,2*NX+1:3*NX)-Lx(:,3*NX+1:4*NX) Lx(:,4*NX+1:4*NX+NXyc*Nz)-Lx(:,4*NX+NXyc*Nz+1:4*NX+2*NXyc*Nz)];
Qx_y=Qx_y(:,1:NN)+Qx_y(:,NN+1:2*NN)-Qx_y(:,2*NN+1:3*NN)-Qx_y(:,3*NN+1:4*NN);
Qx_z=Qx_z(:,1:NZ)+Qx_z(:,NZ+1:2*NZ)+Qx_z(:,2*NZ+1:3*NZ)+Qx_z(:,3*NZ+1:4*NZ);

Ly=[Ly(:,1:NY)-Ly(:,NY+1:2*NY)+Ly(:,2*NY+1:3*NY)-Ly(:,3*NY+1:4*NY) Ly(:,4*NY+1:4*NY+NYxc*Nz)-Ly(:,4*NY+NYxc*Nz+1:4*NY+2*NYxc*Nz)];
Qy_x=Qy_x(:,1:NN)-Qy_x(:,NN+1:2*NN)+Qy_x(:,2*NN+1:3*NN)-Qy_x(:,3*NN+1:4*NN);
Qy_z=Qy_z(:,1:NZ)+Qy_z(:,NZ+1:2*NZ)+Qy_z(:,2*NZ+1:3*NZ)+Qy_z(:,3*NZ+1:4*NZ);

Bx_y=[Bx_y(:,1:NY)-Bx_y(:,NY+1:2*NY)+Bx_y(:,2*NY+1:3*NY)-Bx_y(:,3*NY+1:4*NY) Bx_y(:,4*NY+1:4*NY+NYxc*Nz)-Bx_y(:,4*NY+NYxc*Nz+1:4*NY+2*NYxc*Nz)];
Pmxx=Pmxx(:,1:NN)-Pmxx(:,NN+1:2*NN)+Pmxx(:,2*NN+1:3*NN)-Pmxx(:,3*NN+1:4*NN);
Pmxy=Pmxy(:,1:NN)+Pmxy(:,NN+1:2*NN)-Pmxy(:,2*NN+1:3*NN)-Pmxy(:,3*NN+1:4*NN);
Pmxz=Pmxz(:,1:NZ)+Pmxz(:,NZ+1:2*NZ)+Pmxz(:,2*NZ+1:3*NZ)+Pmxz(:,3*NZ+1:4*NZ);

By_x=[By_x(:,1:NX)+By_x(:,NX+1:2*NX)-By_x(:,2*NX+1:3*NX)-By_x(:,3*NX+1:4*NX) By_x(:,4*NX+1:4*NX+NXyc*Nz)-By_x(:,4*NX+NXyc*Nz+1:4*NX+2*NXyc*Nz)];
Pmyx=Pmyx(:,1:NN)-Pmyx(:,NN+1:2*NN)+Pmyx(:,2*NN+1:3*NN)-Pmyx(:,3*NN+1:4*NN);
Pmyy=Pmyy(:,1:NN)+Pmyy(:,NN+1:2*NN)-Pmyy(:,2*NN+1:3*NN)-Pmyy(:,3*NN+1:4*NN);
Pmyz=Pmyz(:,1:NZ)+Pmyz(:,NZ+1:2*NZ)+Pmyz(:,2*NZ+1:3*NZ)+Pmyz(:,3*NZ+1:4*NZ);

Bz_x=[Bz_x(:,1:NX)+Bz_x(:,NX+1:2*NX)-Bz_x(:,2*NX+1:3*NX)-Bz_x(:,3*NX+1:4*NX) Bz_x(:,4*NX+1:4*NX+NXyc*Nz)-Bz_x(:,4*NX+NXyc*Nz+1:4*NX+2*NXyc*Nz)];
Bz_y=[Bz_y(:,1:NY)-Bz_y(:,NY+1:2*NY)+Bz_y(:,2*NY+1:3*NY)-Bz_y(:,3*NY+1:4*NY) Bz_y(:,4*NY+1:4*NY+NYxc*Nz)-Bz_y(:,4*NY+NYxc*Nz+1:4*NY+2*NYxc*Nz)];
Pmzx=Pmzx(:,1:NN)-Pmzx(:,NN+1:2*NN)+Pmzx(:,2*NN+1:3*NN)-Pmzx(:,3*NN+1:4*NN);
Pmzy=Pmzy(:,1:NN)+Pmzy(:,NN+1:2*NN)-Pmzy(:,2*NN+1:3*NN)-Pmzy(:,3*NN+1:4*NN);
Pmzz=Pmzz(:,1:NZ)+Pmzz(:,NZ+1:2*NZ)+Pmzz(:,2*NZ+1:3*NZ)+Pmzz(:,3*NZ+1:4*NZ);

clear I1 I2 I3 I4 I5 I6 I7 I8 I9 I10;
clear Tp_x Tp_y TpX TpY TpxX TpyY;
clear Sxy Sxy_X Sxy_Y;

end