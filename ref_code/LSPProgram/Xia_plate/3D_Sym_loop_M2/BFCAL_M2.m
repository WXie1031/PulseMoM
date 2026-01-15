% COEF_3D_PLATE_M2  Calculating matrix coef. using surface meshing
%
% Pl(3), Ps(3)     ource plate position (x0,y0,z0), plate size (wx,wy,wz)
% OJx(x1,x2,y), SX(x1,x2,y1,y2)
% OJy(x,y1,y2), SY(x1,x2,y1,y2) coordinates for Jy cells 
% OMx(x,y,z), OMy, OMz  coordinates of field points on sur. for Mx,My,Mz
% S1(y1,y2,z1,z2) coordinates of source points on x-dir plane
% S2(x1,x2,z1,z2) coordinates of source points on y-dir plane
% S3(x1,x2,y1,y2) coordinates of source points on z-dir plane
% N(3 )           number of cells in u and v dir. on x,y,z planes
% Nx=N(1), Ny=N(2), Nz=N(3)

% updated on 11/06/2011
%
function [Bx_y By_x Bz_x Bz_y Pmxx Pmxy Pmxz Pmyx Pmyy Pmyz Pmzx Pmzy Pmzz]=BFCAL_M2(Pl,Ps,Oxyz,S1,S2,S3,SX,SY,aa,N)
Ns=1;                   % sub-division along z for integration
n=12; 
t=[-0.981560634246719 -0.904117256370475 -0.769902674194305 -0.587317954286617 -0.367831498998180 -0.125233408511469 0.125233408511469 0.367831498998180 0.587317954286617 0.769902674194305 0.904117256370475 0.981560634246719];
A=[0.047175336386512 0.106939325995318 0.160078328543346 0.203167426723066 0.233492536538355 0.249147045813403 0.249147045813403 0.233492536538355 0.203167426723066 0.160078328543346 0.106939325995318 0.047175336386512];
wx=Ps(1);   wy=Ps(2);   dd=Ps(3);
X0=Pl(1);   Y0=Pl(2);   Z0=Pl(3);
Nx=N(1);    Ny=N(2);    Nz=N(3);

Nxc=Nx/2; Nyc=Ny/2;
NXxc=Nxc-1;
NYyc=Nyc-1;

NX=NXxc*Nyc;
NY=Nxc*NYyc;
Nxy=Nxc*Nyc;
Nyz=Nyc*Nz;
Nxz=Nxc*Nz;

% initilization
Bx_y1=0; Bx_y2=0;        % for Bxy
Bz_y1=0; Bz_y2=0;        % for Bzy
By_x1=0; By_x2=0;        % for Byx 
Bz_x1=0; Bz_x2=0;        % for Bzx

% (a) ocooridnates of pts on z-dir plane for Lx and Ly 
XJxs=SX(:,1:2);          % sr. pts on z-dir plane for x-KVL
YJxs=SX(:,3:4);
XJys=SY(:,1:2);          % sr. pts on z-dir plane for y-KVL
YJys=SY(:,3:4);
ds=dd/Ns;
zjs=-dd/2+(0:Ns)*ds;

% (b) cooridnates of pts on S3
Xzs=S3(:,1:2);    
Yzs=S3(:,3:4);
zzs1=-dd/2+Z0;
zzs2= dd/2+Z0;

% (c) cooridnates of pts on S2
Xys=S2(:,1:2);    
Zys=S2(:,3:4);
yys1=-wy/2+Y0;
yys2= wy/2+Y0;

% (d) cooridnates of pts on S1
Yxs=S1(:,1:2);    
Zxs=S1(:,3:4);
xxs1=-wx/2+X0;
xxs2= wx/2+X0;

% (1) generating Lx Ly Bxy Byx Bzx Bzy
for j=1:Ns
    z1=zjs(j);
    z2=zjs(j+1);
    tp=(z2-z1)/2;
    zi=0.5*(z2-z1)*t+0.5*(z2+z1);
    [ki1 ki2]=KGEN_A(aa,dd,zi);
    for i=1:n        
        % for Bxy (z plane)
        [I7]=FM_3D_7(Oxyz,XJys,YJys,zi(i)+Z0);           
        Bx_y1=ki1(i)*A(i)*tp*I7+Bx_y1;
        Bx_y2=ki2(i)*A(i)*tp*I7+Bx_y2;
        clear I7;
        
        % for Byx (z plane)
        [I8]=FM_3D_8(Oxyz,XJxs,YJxs,zi(i)+Z0);           
        By_x1=ki1(i)*A(i)*tp*I8+By_x1;
        By_x2=ki2(i)*A(i)*tp*I8+By_x2; 
        clear I8;
        
         % for Bzx (z plane)
        [I9]=FM_3D_9(Oxyz,XJxs,YJxs,zi(i)+Z0);           
        Bz_x1=ki1(i)*A(i)*tp*I9+Bz_x1;
        Bz_x2=ki2(i)*A(i)*tp*I9+Bz_x2;
        clear I9;
                
        % for Bzy (z plane)
        [I10]=FM_3D_10(Oxyz,XJys,YJys,zi(i)+Z0);           
        Bz_y1=ki1(i)*A(i)*tp*I10+Bz_y1;
        Bz_y2=ki2(i)*A(i)*tp*I10+Bz_y2;
        clear I10;
    end
end
By_x1=[By_x1(:,1:NX)+By_x1(:,NX+1:2*NX)-By_x1(:,2*NX+1:3*NX)-By_x1(:,3*NX+1:4*NX) By_x1(:,4*NX+1:4*NX+Nyc)-By_x1(:,4*NX+Nyc+1:4*NX+2*Nyc)];
By_x2=[By_x2(:,1:NX)+By_x2(:,NX+1:2*NX)-By_x2(:,2*NX+1:3*NX)-By_x2(:,3*NX+1:4*NX) By_x2(:,4*NX+1:4*NX+Nyc)-By_x2(:,4*NX+Nyc+1:4*NX+2*Nyc)];
By_x=[By_x1 By_x2];

Bz_x1=[Bz_x1(:,1:NX)+Bz_x1(:,NX+1:2*NX)-Bz_x1(:,2*NX+1:3*NX)-Bz_x1(:,3*NX+1:4*NX) Bz_x1(:,4*NX+1:4*NX+Nyc)-Bz_x1(:,4*NX+Nyc+1:4*NX+2*Nyc)];
Bz_x2=[Bz_x2(:,1:NX)+Bz_x2(:,NX+1:2*NX)-Bz_x2(:,2*NX+1:3*NX)-Bz_x2(:,3*NX+1:4*NX) Bz_x2(:,4*NX+1:4*NX+Nyc)-Bz_x2(:,4*NX+Nyc+1:4*NX+2*Nyc)];
Bz_x=[Bz_x1 Bz_x2];

Bx_y1=[Bx_y1(:,1:NY)-Bx_y1(:,NY+1:2*NY)+Bx_y1(:,2*NY+1:3*NY)-Bx_y1(:,3*NY+1:4*NY) Bx_y1(:,4*NY+1:4*NY+Nxc)-Bx_y1(:,4*NY+Nxc+1:4*NY+2*Nxc)];
Bx_y2=[Bx_y2(:,1:NY)-Bx_y2(:,NY+1:2*NY)+Bx_y2(:,2*NY+1:3*NY)-Bx_y2(:,3*NY+1:4*NY) Bx_y2(:,4*NY+1:4*NY+Nxc)-Bx_y2(:,4*NY+Nxc+1:4*NY+2*Nxc)];
Bx_y=[Bx_y1 Bx_y2];

Bz_y1=[Bz_y1(:,1:NY)-Bz_y1(:,NY+1:2*NY)+Bz_y1(:,2*NY+1:3*NY)-Bz_y1(:,3*NY+1:4*NY) Bz_y1(:,4*NY+1:4*NY+Nxc)-Bz_y1(:,4*NY+Nxc+1:4*NY+2*Nxc)];
Bz_y2=[Bz_y2(:,1:NY)-Bz_y2(:,NY+1:2*NY)+Bz_y2(:,2*NY+1:3*NY)-Bz_y2(:,3*NY+1:4*NY) Bz_y2(:,4*NY+1:4*NY+Nxc)-Bz_y2(:,4*NY+Nxc+1:4*NY+2*Nxc)];
Bz_y=[Bz_y1 Bz_y2];

% (3) generating Pmxx Pmyy Pmzz 
% Pxx, sr=z-plane/y-plane
[I12a]=FM_3D_12(Oxyz,Xys,yys1,Zys);
[I12b]=FM_3D_12(Oxyz,Xys,yys2,Zys);
[I13a]=FM_3D_13(Oxyz,Xzs,Yzs,zzs1);
[I13b]=FM_3D_13(Oxyz,Xzs,Yzs,zzs2);
I12a=I12a(:,1:Nxz)-I12a(:,Nxz+1:2*Nxz);
I12b=I12b(:,1:Nxz)-I12b(:,Nxz+1:2*Nxz);
I12=-I12a+I12b;
I13a=I13a(:,1:Nxy)-I13a(:,Nxy+1:2*Nxy)+I13a(:,2*Nxy+1:3*Nxy)-I13a(:,3*Nxy+1:4*Nxy);
I13b=I13b(:,1:Nxy)-I13b(:,Nxy+1:2*Nxy)+I13b(:,2*Nxy+1:3*Nxy)-I13b(:,3*Nxy+1:4*Nxy);
Pmxx=[I12 -I13a I13b];
clear I12a I12b I13a I13b I12;

% Pyy, sr=z-plane/y-plane
[I11a]=FM_3D_11(Oxyz,xxs1,Yxs,Zxs);
[I11b]=FM_3D_11(Oxyz,xxs2,Yxs,Zxs);
[I13a]=FM_3D_13(Oxyz,Xzs,Yzs,zzs1);
[I13b]=FM_3D_13(Oxyz,Xzs,Yzs,zzs2);
I11a=I11a(:,1:Nyz)-I11a(:,Nyz+1:2*Nyz);
I11b=I11b(:,1:Nyz)-I11b(:,Nyz+1:2*Nyz);
I11=-I11a+I11b;
I13a=I13a(:,1:Nxy)+I13a(:,Nxy+1:2*Nxy)-I13a(:,2*Nxy+1:3*Nxy)-I13a(:,3*Nxy+1:4*Nxy);
I13b=I13b(:,1:Nxy)+I13b(:,Nxy+1:2*Nxy)-I13b(:,2*Nxy+1:3*Nxy)-I13b(:,3*Nxy+1:4*Nxy);
Pmyy=[I11 -I13a I13b];
clear I11a I11b I13a I13b I11;

% Pzz, sr=x-plane/y-plane
[I11a]=FM_3D_11(Oxyz,xxs1,Yxs,Zxs);
[I11b]=FM_3D_11(Oxyz,xxs2,Yxs,Zxs);
[I12a]=FM_3D_12(Oxyz,Xys,yys1,Zys);
[I12b]=FM_3D_12(Oxyz,Xys,yys2,Zys);
I11a=I11a(:,1:Nyz)+I11a(:,Nyz+1:2*Nyz);
I11b=I11b(:,1:Nyz)+I11b(:,Nyz+1:2*Nyz);
I11=-I11a+I11b;
I12a=I12a(:,1:Nxz)+I12a(:,Nxz+1:2*Nxz);
I12b=I12b(:,1:Nxz)+I12b(:,Nxz+1:2*Nxz);
I12=-I12a+I12b;
Pmzz=[I11 I12];
clear I11a I11b I12a I12b I11 I12;

% (4) generating Pmxy Pmxy Pmxz Pmzx Pmyz Pmzy
% Pxy, sr=x-plane
[I14a]=FM_3D_14yz(Oxyz,xxs1,Yxs,Zxs);
[I14b]=FM_3D_14yz(Oxyz,xxs2,Yxs,Zxs);
I14a=I14a(:,1:Nyz)-I14a(:,Nyz+1:2*Nyz);
I14b=I14b(:,1:Nyz)-I14b(:,Nyz+1:2*Nyz);
Pmxy=-I14a+I14b;
% Pyx, sr=y-plane
[I14a]=FM_3D_14xz(Oxyz,Xys,yys1,Zys);
[I14b]=FM_3D_14xz(Oxyz,Xys,yys2,Zys);
I14a=I14a(:,1:Nxz)-I14a(:,Nxz+1:2*Nxz);
I14b=I14b(:,1:Nxz)-I14b(:,Nxz+1:2*Nxz);
Pmyx=-I14a+I14b;
clear I14a I14b;

% Pxz, sr=x-plane
[I15a]=FM_3D_15yz(Oxyz,xxs1,Yxs,Zxs);
[I15b]=FM_3D_15yz(Oxyz,xxs2,Yxs,Zxs);
I15a=I15a(:,1:Nyz)+I15a(:,Nyz+1:2*Nyz);
I15b=I15b(:,1:Nyz)+I15b(:,Nyz+1:2*Nyz);
Pmxz=-I15a+I15b;
% Pzx, sr=z-plane
[I15a]=FM_3D_15xy(Oxyz,Xzs,Yzs,zzs1);
[I15b]=FM_3D_15xy(Oxyz,Xzs,Yzs,zzs2);
I15a=I15a(:,1:Nxy)-I15a(:,Nxy+1:2*Nxy)+I15a(:,2*Nxy+1:3*Nxy)-I15a(:,3*Nxy+1:4*Nxy);
I15b=I15b(:,1:Nxy)-I15b(:,Nxy+1:2*Nxy)+I15b(:,2*Nxy+1:3*Nxy)-I15b(:,3*Nxy+1:4*Nxy);
Pmzx=[-I15a I15b];
clear I15a I15b;

% Pyz, sr=y-plane
[I16a]=FM_3D_16xz(Oxyz,Xys,yys1,Zys);
[I16b]=FM_3D_16xz(Oxyz,Xys,yys2,Zys);
I16a=I16a(:,1:Nxz)+I16a(:,Nxz+1:2*Nxz);
I16b=I16b(:,1:Nxz)+I16b(:,Nxz+1:2*Nxz);
Pmyz=-I16a+I16b;
% Pzy, sr=x-plane
[I16a]=FM_3D_16xy(Oxyz,Xzs,Yzs,zzs1);
[I16b]=FM_3D_16xy(Oxyz,Xzs,Yzs,zzs2);
I16a=I16a(:,1:Nxy)+I16a(:,Nxy+1:2*Nxy)-I16a(:,2*Nxy+1:3*Nxy)-I16a(:,3*Nxy+1:4*Nxy);
I16b=I16b(:,1:Nxy)+I16b(:,Nxy+1:2*Nxy)-I16b(:,2*Nxy+1:3*Nxy)-I16b(:,3*Nxy+1:4*Nxy);
Pmzy=[-I16a I16b];
clear I16a I16b;  

end