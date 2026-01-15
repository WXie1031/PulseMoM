% transform the coef. matrix for X-cell and Y-cell to matrix for loop current
function [L Qx Qy Qz Bx By Bz S]=Trans_loop(Lx,Qx_y,Qx_z,Ly,Qy_x,Qy_z,Bx_y,By_x,Bz_x,Bz_y,Sx,Sy,Coex,Coey,N)
Nx=N(1);    Ny=N(2);    Nz=N(3);

Nxc=Nx/2; Nyc=Ny/2;
NXxc=Nxc-1;
NYyc=Nyc-1;
NX=NXxc*Nyc;
NY=Nxc*NYyc;
Nxy=Nxc*Nyc;

% for Jx
Lx11=Lx(1:Nxy,1:Nxy);
Lx12=Lx(1:Nxy,Nxy+1:2*Nxy);
Lx21=Lx(Nxy+1:2*Nxy,1:Nxy);
Lx22=Lx(Nxy+1:2*Nxy,Nxy+1:2*Nxy);

Qx_y1=Qx_y(1:Nxy,:);
Qx_y2=Qx_y(Nxy+1:2*Nxy,:);
Qx_z1=Qx_z(1:Nxy,:);
Qx_z2=Qx_z(Nxy+1:2*Nxy,:);

Sx1=Sx(1:Nxy,:);
Sx2=Sx(Nxy+1:2*Nxy,:);

% for Jy
Ly11=Ly(1:Nxy,1:Nxy);
Ly12=Ly(1:Nxy,Nxy+1:2*Nxy);
Ly21=Ly(Nxy+1:2*Nxy,1:Nxy);
Ly22=Ly(Nxy+1:2*Nxy,Nxy+1:2*Nxy);

Qy_x1=Qy_x(1:Nxy,:);
Qy_x2=Qy_x(Nxy+1:2*Nxy,:);
Qy_z1=Qy_z(1:Nxy,:);
Qy_z2=Qy_z(Nxy+1:2*Nxy,:);

Sy1=Sy(1:Nxy,:);
Sy2=Sy(Nxy+1:2*Nxy,:);

for i=1:Nyc
    L1y1(Nxc*(i-1)+1:Nxc*i-1,:)=Ly11(Nxc*(i-1)+1:Nxc*i-1,:)-Ly11(Nxc*(i-1)+2:Nxc*i,:);
    L1y2(Nxc*(i-1)+1:Nxc*i-1,:)=Ly12(Nxc*(i-1)+1:Nxc*i-1,:)-Ly12(Nxc*(i-1)+2:Nxc*i,:);
    L2y1(Nxc*(i-1)+1:Nxc*i-1,:)=Ly21(Nxc*(i-1)+1:Nxc*i-1,:)-Ly21(Nxc*(i-1)+2:Nxc*i,:);
    L2y2(Nxc*(i-1)+1:Nxc*i-1,:)=Ly22(Nxc*(i-1)+1:Nxc*i-1,:)-Ly22(Nxc*(i-1)+2:Nxc*i,:);
    
    Qyx1(Nxc*(i-1)+1:Nxc*i-1,:)=Qy_x1(Nxc*(i-1)+1:Nxc*i-1,:)-Qy_x1(Nxc*(i-1)+2:Nxc*i,:);
    Qyx2(Nxc*(i-1)+1:Nxc*i-1,:)=Qy_x2(Nxc*(i-1)+1:Nxc*i-1,:)-Qy_x2(Nxc*(i-1)+2:Nxc*i,:);
    
    Qyz1(Nxc*(i-1)+1:Nxc*i-1,:)=Qy_z1(Nxc*(i-1)+1:Nxc*i-1,:)-Qy_z1(Nxc*(i-1)+2:Nxc*i,:);
    Qyz2(Nxc*(i-1)+1:Nxc*i-1,:)=Qy_z2(Nxc*(i-1)+1:Nxc*i-1,:)-Qy_z2(Nxc*(i-1)+2:Nxc*i,:);
    
    YS1(Nxc*(i-1)+1:Nxc*i-1,:)=Sy1(Nxc*(i-1)+1:Nxc*i-1,:)-Sy1(Nxc*(i-1)+2:Nxc*i,:);
    YS2(Nxc*(i-1)+1:Nxc*i-1,:)=Sy2(Nxc*(i-1)+1:Nxc*i-1,:)-Sy2(Nxc*(i-1)+2:Nxc*i,:);      
    if (i<Nyc)
        L1x1(Nxc*(i-1)+1:Nxc*i-1,:)=-Lx11(NXxc*(i-1)+1:NXxc*i,:)+Lx11(NXxc*i+1:NXxc*(i+1),:);
        L1x2(Nxc*(i-1)+1:Nxc*i-1,:)=-Lx12(NXxc*(i-1)+1:NXxc*i,:)+Lx12(NXxc*i+1:NXxc*(i+1),:);
        L2x1(Nxc*(i-1)+1:Nxc*i-1,:)=-Lx21(NXxc*(i-1)+1:NXxc*i,:)+Lx21(NXxc*i+1:NXxc*(i+1),:);
        L2x2(Nxc*(i-1)+1:Nxc*i-1,:)=-Lx22(NXxc*(i-1)+1:NXxc*i,:)+Lx22(NXxc*i+1:NXxc*(i+1),:);
        
        Qxy1(Nxc*(i-1)+1:Nxc*i-1,:)=-Qx_y1(NXxc*(i-1)+1:NXxc*i,:)+Qx_y1(NXxc*i+1:NXxc*(i+1),:);
        Qxy2(Nxc*(i-1)+1:Nxc*i-1,:)=-Qx_y2(NXxc*(i-1)+1:NXxc*i,:)+Qx_y2(NXxc*i+1:NXxc*(i+1),:);
        
        Qxz1(Nxc*(i-1)+1:Nxc*i-1,:)=-Qx_z1(NXxc*(i-1)+1:NXxc*i,:)+Qx_z1(NXxc*i+1:NXxc*(i+1),:);
        Qxz2(Nxc*(i-1)+1:Nxc*i-1,:)=-Qx_z2(NXxc*(i-1)+1:NXxc*i,:)+Qx_z2(NXxc*i+1:NXxc*(i+1),:);
        
        XS1(Nxc*(i-1)+1:Nxc*i-1,:)=-Sx1(NXxc*(i-1)+1:NXxc*i,:)+Sx1(NXxc*i+1:NXxc*(i+1),:);
        XS2(Nxc*(i-1)+1:Nxc*i-1,:)=-Sx2(NXxc*(i-1)+1:NXxc*i,:)+Sx2(NXxc*i+1:NXxc*(i+1),:);
        
        L1x1(Nxc*i,:)=-Lx11(NX+i,:)+Lx11(NX+i+1,:);
        L1x2(Nxc*i,:)=-Lx12(NX+i,:)+Lx12(NX+i+1,:);
        L2x1(Nxc*i,:)=-Lx21(NX+i,:)+Lx21(NX+i+1,:);
        L2x2(Nxc*i,:)=-Lx22(NX+i,:)+Lx22(NX+i+1,:);
        
        Qxy1(Nxc*i,:)=-Qx_y1(NX+i,:)+Qx_y1(NX+i+1,:);
        Qxy2(Nxc*i,:)=-Qx_y2(NX+i,:)+Qx_y2(NX+i+1,:);
        
        Qxz1(Nxc*i,:)=-Qx_z1(NX+i,:)+Qx_z1(NX+i+1,:);
        Qxz2(Nxc*i,:)=-Qx_z2(NX+i,:)+Qx_z2(NX+i+1,:);
        
        XS1(Nxc*i,:)=-Sx1(NX+i,:)+Sx1(NX+i+1,:);
        XS2(Nxc*i,:)=-Sx2(NX+i,:)+Sx2(NX+i+1,:);  
        
        L1y1(Nxc*i,:)=2*Ly11(Nxc*i,:);
        L1y2(Nxc*i,:)=2*Ly12(Nxc*i,:);
        L2y1(Nxc*i,:)=2*Ly21(Nxc*i,:);
        L2y2(Nxc*i,:)=2*Ly22(Nxc*i,:);
        
        Qyx1(Nxc*i,:)=2*Qy_x1(Nxc*i,:);
        Qyx2(Nxc*i,:)=2*Qy_x2(Nxc*i,:);
        
        Qyz1(Nxc*i,:)=2*Qy_z1(Nxc*i,:);
        Qyz2(Nxc*i,:)=2*Qy_z2(Nxc*i,:);
        
        YS1(Nxc*i,:)=2*Sy1(Nxc*i,:);
        YS2(Nxc*i,:)=2*Sy2(Nxc*i,:);   
    else
        L1x1(Nxc*(i-1)+1:Nxc*i-1,:)=-2*Lx11(NXxc*(i-1)+1:NXxc*i,:);
        L1x2(Nxc*(i-1)+1:Nxc*i-1,:)=-2*Lx12(NXxc*(i-1)+1:NXxc*i,:);
        L2x1(Nxc*(i-1)+1:Nxc*i-1,:)=-2*Lx21(NXxc*(i-1)+1:NXxc*i,:);
        L2x2(Nxc*(i-1)+1:Nxc*i-1,:)=-2*Lx22(NXxc*(i-1)+1:NXxc*i,:);
        
        Qxy1(Nxc*(i-1)+1:Nxc*i-1,:)=-2*Qx_y1(NXxc*(i-1)+1:NXxc*i,:);
        Qxy2(Nxc*(i-1)+1:Nxc*i-1,:)=-2*Qx_y2(NXxc*(i-1)+1:NXxc*i,:);
        
        Qxz1(Nxc*(i-1)+1:Nxc*i-1,:)=-2*Qx_z1(NXxc*(i-1)+1:NXxc*i,:);
        Qxz2(Nxc*(i-1)+1:Nxc*i-1,:)=-2*Qx_z2(NXxc*(i-1)+1:NXxc*i,:);
        
        XS1(Nxc*(i-1)+1:Nxc*i-1,:)=-2*Sx1(NXxc*(i-1)+1:NXxc*i,:);
        XS2(Nxc*(i-1)+1:Nxc*i-1,:)=-2*Sx2(NXxc*(i-1)+1:NXxc*i,:);        
    end    
end    
  
L1x1(Nxy,:)=-Lx11(Nxy,:);        L1x2(Nxy,:)=-Lx12(Nxy,:);
L2x1(Nxy,:)=-Lx21(Nxy,:);        L2x2(Nxy,:)=-Lx22(Nxy,:);
Qxy1(Nxy,:)=-Qx_y1(Nxy,:);        Qxy2(Nxy,:)=-Qx_y2(Nxy,:);
Qxz1(Nxy,:)=-Qx_z1(Nxy,:);        Qxz2(Nxy,:)=-Qx_z2(Nxy,:);  
XS1(Nxy,:)=-Sx1(Nxy,:);        XS2(Nxy,:)=-Sx2(Nxy,:);

L1y1(Nxy,:)=Ly11(Nxy,:);        L1y2(Nxy,:)=Ly12(Nxy,:);
L2y1(Nxy,:)=Ly21(Nxy,:);        L2y2(Nxy,:)=Ly22(Nxy,:);
Qyx1(Nxy,:)=Qy_x1(Nxy,:);        Qyx2(Nxy,:)=Qy_x2(Nxy,:);
Qyz1(Nxy,:)=Qy_z1(Nxy,:);        Qyz2(Nxy,:)=Qy_z2(Nxy,:);
YS1(Nxy,:)=Sy1(Nxy,:);        YS2(Nxy,:)=Sy2(Nxy,:);
        
L1=[L1x1*Coex+L1y1*Coey L1x2*Coex+L1y2*Coey];
L2=[L2x1*Coex+L2y1*Coey L2x2*Coex+L2y2*Coey];
L=[L1; L2];

Qx=[Qyx1; Qyx2];
Qy=[Qxy1; Qxy2];
Qz=[Qxz1+Qyz1; Qxz2+Qyz2];

S=[XS1+YS1; XS2+YS2];

% for B matrix
Bxy1=Bx_y(:,1:Nxy);
Bxy2=Bx_y(:,Nxy+1:2*Nxy);
Bx=[Bxy1*Coey Bxy2*Coey];

Byx1=By_x(:,1:Nxy);
Byx2=By_x(:,Nxy+1:2*Nxy);
By=[Byx1*Coex Byx2*Coex];

Bzx1=Bz_x(:,1:Nxy);
Bzx2=Bz_x(:,Nxy+1:2*Nxy);
Bzy1=Bz_y(:,1:Nxy);
Bzy2=Bz_y(:,Nxy+1:2*Nxy);
Bz=[Bzx1*Coex+Bzy1*Coey Bzx2*Coex+Bzy2*Coey];

end