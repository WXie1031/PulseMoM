% MESHING   Generating coordinates of cells for M2
% 
% Ordering: lower -> upper 
%           left  -> right
%
% Ps=[wx wy wz];        Plate size
% Pl=[xo yo zo];        plare location
% N=[Nx Ny Nz]; no of segments (Mx,My,Mz)in x,y and z dir.
% le                    % the depth for refinement in edge area
%
% S1(y1,y2,z1,z2)       % x-plane (globe)
% S2(x1,x2,z1,z2)       % y-plane (globe)
% S3(x1,x2,y1,y2)       % z-plane (globe)
% SX(x1,x2,y1,y2)       % z-plane for Jx (globe)
% SY(x1,x2,y1,y2)       % z-plane for Jy (globe)
% OMx(xo,yo,zo)         % field pts for Mx (S2+S3),globe
% OMy(xo,yo,zo)         % field pts for My (S1+S3),globe
% OMz(xo,yo,zo)         % field pts for Mz (S1+S2),globe
% OJx(x1,x2,yo,zo)      % field pts for Jx, globe
% OJy(xo,y1,y2,zo)      % field pts for Jy, globe
%
% 21 Dec 2011
%
function [OMx OMy OMz OJx OJy S1 S2 S3 SX SY u,v,w]=MESHING_3D_M2(Pl,Ps,N,r)
X0=Pl(1);  Y0=Pl(2);  Z0=Pl(3);  
wx=Ps(1);  wy=Ps(2);  wz=Ps(3);  
Nx=N(1);   Ny=N(2);   Nz=N(3);

if (r)
    [u v]=Refinement_3D_M2(Ps,N,0.002,5,1.5);
    Nx=length(u)-1;
    Ny=length(v)-1;    
else
    u=(-0.5+(0:Nx)/Nx)*wx;
    v=(-0.5+(0:Ny)/Ny)*wy;    
end
w=(-0.5+(0:Nz)/Nz)*wz;
u=u+X0; v=v+Y0; w=w+Z0;

u0=0.5*(u(1:Nx)+u(2:Nx+1));
v0=0.5*(v(1:Ny)+v(2:Ny+1));

Nxc=Nx/2; Nyc=Ny/2;
NXxc=Nxc-1;
NYyc=Nyc-1;

% for S5/S6 (x-y plane)
for i=1:Nyc
    % for Mx My 
    idx=(1:Nxc)+(i-1)*Nxc;
    Sxy1(idx,1)=u(1:Nxc);
    Sxy1(idx,2)=u(2:Nxc+1);
    Sxy1(idx,3)=v(i);
    Sxy1(idx,4)=v(i+1); 
    Sxy4(idx,1)=fliplr(u(Nxc+1:Nx));
    Sxy4(idx,2)=fliplr(u(Nxc+2:Nx+1));
    Sxy4(idx,3)=v(Ny+1-i);
    Sxy4(idx,4)=v(Ny+2-i);
    
    % for Jx
    idx=(1:NXxc)+(i-1)*NXxc;
    Sxy1_X(idx,1)=u0(1:NXxc);
    Sxy1_X(idx,2)=u0(2:NXxc+1);
    Sxy1_X(idx,3)=v(i);
    Sxy1_X(idx,4)=v(i+1); 
    Sxy4_X(idx,1)=fliplr(u0(NXxc+2:Nx-1));
    Sxy4_X(idx,2)=fliplr(u0(NXxc+3:Nx));
    Sxy4_X(idx,3)=v(Ny+1-i);
    Sxy4_X(idx,4)=v(Ny+2-i);
    Sxy5_X(i,1)=u0(NXxc+1);
    Sxy5_X(i,2)=u0(NXxc+2);
    Sxy5_X(i,3)=v(i);
    Sxy5_X(i,4)=v(i+1); 
    Sxy6_X(i,3)=v(Ny+1-i);
    Sxy6_X(i,4)=v(Ny+2-i);
end
Sxy2=[Sxy4(:,1) Sxy4(:,2) Sxy1(:,3) Sxy1(:,4)];
Sxy3=[Sxy1(:,1) Sxy1(:,2) Sxy4(:,3) Sxy4(:,4)];
S3=[Sxy1; Sxy2; Sxy3; Sxy4];           % M sources on x-y plane

Oxy(:,1)=0.5*(Sxy1(:,1)+Sxy1(:,2));     
Oxy(:,2)=0.5*(Sxy1(:,3)+Sxy1(:,4));
Ixy=ones(Nxc*Nyc,1);
Oxya=[Oxy Ixy*w(1)];          % M observation on x-y plane
Oxyb=[Oxy Ixy*w(end)];

Sxy2_X=[Sxy4_X(:,1) Sxy4_X(:,2) Sxy1_X(:,3) Sxy1_X(:,4)];
Sxy3_X=[Sxy1_X(:,1) Sxy1_X(:,2) Sxy4_X(:,3) Sxy4_X(:,4)];
Sxy6_X(:,1:2)=Sxy5_X(:,1:2);
SX=[Sxy1_X; Sxy2_X; Sxy3_X; Sxy4_X; Sxy5_X; Sxy6_X];        % Jx sources on x-y plane

Oxy1_X(:,1:2)=Sxy1_X(:,1:2);
Oxy1_X(:,3)=0.5*(Sxy1_X(:,3)+Sxy1_X(:,4));
Oxy5_X(:,1:2)=Sxy5_X(:,1:2);
Oxy5_X(:,3)=0.5*(Sxy5_X(:,3)+Sxy5_X(:,4));
Oxy_X=[Oxy1_X; Oxy5_X];         
Oxy_Xa=[Oxy_X Ixy*w(1)];          
Oxy_Xb=[Oxy_X Ixy*w(end)];
OJx=[Oxy_Xa;Oxy_Xb];              % Jx observation on x-y plane

for i=1:NYyc
    idx=(1:Nxc)+(i-1)*Nxc;           
    Sxy1_Y(idx,1)=u(1:Nxc);
    Sxy1_Y(idx,2)=u(2:Nxc+1);
    Sxy1_Y(idx,3)=v0(i);
    Sxy1_Y(idx,4)=v0(i+1); 
    Sxy4_Y(idx,1)=fliplr(u(Nxc+1:Nx));
    Sxy4_Y(idx,2)=fliplr(u(Nxc+2:Nx+1));
    Sxy4_Y(idx,3)=v0(Ny-i);
    Sxy4_Y(idx,4)=v0(Ny+1-i);   
end
Sxy2_Y=[Sxy4_Y(:,1) Sxy4_Y(:,2) Sxy1_Y(:,3) Sxy1_Y(:,4)];
Sxy3_Y=[Sxy1_Y(:,1) Sxy1_Y(:,2) Sxy4_Y(:,3) Sxy4_Y(:,4)];
Sxy5_Y=[Sxy1_Y(1:Nxc,1:2) Sxy1_Y(end-Nxc+1:end,4) Sxy3_Y(end-Nxc+1:end,3)];
Sxy6_Y=[Sxy4_Y(1:Nxc,1:2) Sxy5_Y(:,3:4)];
SY=[Sxy1_Y; Sxy2_Y; Sxy3_Y; Sxy4_Y; Sxy5_Y; Sxy6_Y];        % Jy sources on x-y plane
    
Oxy1_Y(:,1)=0.5*(Sxy1_Y(:,1)+Sxy1_Y(:,2));  
Oxy1_Y(:,2:3)=Sxy1_Y(:,3:4);
Oxy5_Y(:,1)=0.5*(Sxy5_Y(:,1)+Sxy5_Y(:,2));  
Oxy5_Y(:,2:3)=Sxy5_Y(:,3:4);
Oxy_Y=[Oxy1_Y; Oxy5_Y];         
Oxy_Ya=[Oxy_Y Ixy*w(1)];          
Oxy_Yb=[Oxy_Y Ixy*w(end)];
OJy=[Oxy_Ya;Oxy_Yb];            % Jy observation on x-y plane

for i=1:Nz   
    % for Mx/Mz x-z plane
    idx=(1:Nxc)+(i-1)*Nxc;
    Sxz1(idx,1)=u(1:Nxc);
    Sxz1(idx,2)=u(2:Nxc+1); 
    Sxz1(idx,3)=w(i);
    Sxz1(idx,4)=w(i+1); 
    Sxz2(idx,1)=fliplr(u(Nxc+1:Nx));
    Sxz2(idx,2)=fliplr(u(Nxc+2:Nx+1)); 
    
    % for My/Mz y-z plane
    idy=(1:Nyc)+(i-1)*Nyc;
    Syz1(idy,1)=v(1:Nyc);
    Syz1(idy,2)=v(2:Nyc+1); 
    Syz1(idy,3)=w(i);
    Syz1(idy,4)=w(i+1);  
    Syz2(idy,1)=fliplr(v(Nyc+1:Ny));
    Syz2(idy,2)=fliplr(v(Nyc+2:Ny+1));    
end
Sxz2(:,3:4)=Sxz1(:,3:4);
S2=[Sxz1; Sxz2];

Syz2(:,3:4)=Syz1(:,3:4);
S1=[Syz1; Syz2];

Oxz(:,1)=0.5*(Sxz1(:,1)+Sxz1(:,2));     
Oxz(:,2)=0.5*(Sxz1(:,3)+Sxz1(:,4));
Ixz=ones(Nxc*Nz,1);
Oxza=[Oxz(:,1) Ixz*v(1) Oxz(:,2)];          % M observation on x-z plane

Oyz(:,1)=0.5*(Syz1(:,1)+Syz1(:,2));     
Oyz(:,2)=0.5*(Syz1(:,3)+Syz1(:,4));
Iyz=ones(Nyc*Nz,1);
Oyza=[Iyz*u(1) Oyz];          % M observation on y-z plane

OMx=[Oxza;Oxya;Oxyb];

OMy=[Oyza;Oxya;Oxyb];

OMz=[Oyza;Oxza];

end