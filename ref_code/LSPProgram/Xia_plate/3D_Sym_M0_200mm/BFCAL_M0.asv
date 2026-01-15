function [Bx_y By_x Bz_x Bz_y Pmxx Pmxy Pmxz Pmyx Pmyy Pmyz Pmzx Pmzy Pmzz]=BFCAL_M0(Oxyz,Sxyz,Sxyz_X,Sxyz_Y,Nx,Ny,NXx,NXy,NYx,NYy,NZx,NZy,Nz)
n=12; 
t=[-0.981560634246719 -0.904117256370475 -0.769902674194305 -0.587317954286617 -0.367831498998180 -0.125233408511469 0.125233408511469 0.367831498998180 0.587317954286617 0.769902674194305 0.904117256370475 0.981560634246719];
A=[0.047175336386512 0.106939325995318 0.160078328543346 0.203167426723066 0.233492536538355 0.249147045813403 0.249147045813403 0.233492536538355 0.203167426723066 0.160078328543346 0.106939325995318 0.047175336386512];

Nxc=Nx/2; Nyc=Ny/2;
NXxc=fix(NXx/2); NXyc=NXy/2;
NYxc=NYx/2; NYyc=fix(NYy/2);
NZxc=NZx/2; NZyc=NZy/2;

% initilization
Bx_y=0; 
By_x=0; 
Bz_x=0; Bz_y=0; 

[I11,I12,I13,I14,I15,I16]=FM_3D_G(Oxyz,Sxyz);
Pmxx=I11; Pmyy=I12; Pmzz=I13; 
Pmxy=I14; Pmyz=I15; Pmxz=I16;
Pmyx=Pmxy; Pmzy=Pmyz; Pmzx=Pmxz;

clear I11 I12 I13 I14 I15 I16;

% Oxyz(xo,yo,zo),Sxyz(x1,y1,z1,x2,y2,z2)
% Oxyz_X(xo1,xo2,yo,zo),Sxyz_X(x1,y1,z1,x2,y2,z2)
% Oxyz_Y(xo,yo1,yo2,zo),Sxyz_Y(x1,y1,z1,x2,y2,z2)
[NN,Def]=size(Oxyz);
zx1=Sxyz_X(:,3); zx2=Sxyz_X(:,6); tpx=(zx2-zx1)/2;
zy1=Sxyz_Y(:,3); zy2=Sxyz_Y(:,6); tpy=(zy2-zy1)/2;

one=ones(NN,1);
Tp_x=one*tpx';
Tp_y=one*tpy';

clear one tpx tpy;

Sxy_X=[Sxyz_X(:,1) Sxyz_X(:,2) Sxyz_X(:,4) Sxyz_X(:,5)];
Sxy_Y=[Sxyz_Y(:,1) Sxyz_Y(:,2) Sxyz_Y(:,4) Sxyz_Y(:,5)];

for i=1:n;    
    zsx=0.5*(zx2-zx1)*t(i)+0.5*(zx2+zx1);
    zsy=0.5*(zy2-zy1)*t(i)+0.5*(zy2+zy1);    
    Sxyz_X=[Sxy_X zsx];
    Sxyz_Y=[Sxy_Y zsy];      
    
    [I7,I10]=FM_3D_E(Oxyz,Sxyz_Y);
    Bx_y=I7*A(i).*Tp_y+Bx_y;
    Bz_y=I10*A(i).*Tp_y+Bz_y;
    
    [I8,I9]=FM_3D_F(Oxyz,Sxyz_X);
    By_x=I8*A(i).*Tp_x+By_x;
    Bz_x=I9*A(i).*Tp_x+Bz_x;
end
  
Nxy=Nxc*Nyc;
NXxy=NXxc*NXyc;
NYxy=NYxc*NYyc;
NZxy=NZxc*NZyc;
NN=Nxy*Nz;
NX=NXxy*Nz;
NY=NYxy*Nz;
NZ=NZxy*Nz;

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

clear I7 I8 I9 I10;
clear Tp_x Tp_y;
clear Sxy_X Sxy_Y;

end