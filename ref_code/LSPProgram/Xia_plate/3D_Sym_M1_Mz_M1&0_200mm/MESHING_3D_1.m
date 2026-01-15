% MESHING   Generating coordinates of cells for M0 and M1 zones
%           M1 is in the middle plate and M0 on its two sides
% 
% Ordering: lower -> upper 
%           left  -> right

% 14 Apr 2011
%
function [Oxy Oxy_X Oxy_Y Oxy_Z Sxy Sxy_X Sxy_Y Sxy_Z]=MESHING_3D_1(u,v,ux,vx,uy,vy,uz,vz)
% Oxy(xo,yo),Sxy(x1,y1,x2,y2)
% Oxy_X(xo1,xo2,yo),Sxy_X(x1,y1,x2,y2)
% Oxy_Y(xo,yo1,yo2),Sxy_Y(x1,y1,x2,y2)
Nx=length(u)-1;
Ny=length(v)-1;
NXx=length(ux)-1;
NXy=length(vx)-1;
NYx=length(uy)-1;
NYy=length(vy)-1;
NZx=length(uz)-1;
NZy=length(vz)-1;

Nxc=Nx/2; Nyc=Ny/2;
NXxc=fix(NXx/2); NXyc=NXy/2;
NYxc=NYx/2; NYyc=fix(NYy/2);
NZxc=NZx/2; NZyc=NZy/2;

for i=1:Nyc
    idx=(1:Nxc)+(i-1)*Nxc;
    Sxy1(idx,1)=u(1:Nxc);
    Sxy1(idx,3)=u(2:Nxc+1);
    Sxy1(idx,2)=v(i);
    Sxy1(idx,4)=v(i+1); 
    Sxy4(idx,1)=fliplr(u(Nxc+1:Nx));
    Sxy4(idx,3)=fliplr(u(Nxc+2:Nx+1));
    Sxy4(idx,2)=v(Ny+1-i);
    Sxy4(idx,4)=v(Ny+2-i);
end
Sxy2=[Sxy4(:,1) Sxy1(:,2) Sxy4(:,3) Sxy1(:,4)];
Sxy3=[Sxy1(:,1) Sxy4(:,2) Sxy1(:,3) Sxy4(:,4)];

Oxy(:,1)=0.5*(Sxy1(:,1)+Sxy1(:,3));
Oxy(:,2)=0.5*(Sxy1(:,2)+Sxy1(:,4));

Sxy=[Sxy1; Sxy2; Sxy3; Sxy4];

for i=1:NXyc  
    idx=(1:NXxc)+(i-1)*NXxc;
    Sxy1_X(idx,1)=ux(1:NXxc);
    Sxy1_X(idx,3)=ux(2:NXxc+1);
    Sxy1_X(idx,2)=vx(i);
    Sxy1_X(idx,4)=vx(i+1); 
    Sxy4_X(idx,1)=fliplr(ux(NXxc+2:NXx));
    Sxy4_X(idx,3)=fliplr(ux(NXxc+3:NXx+1));
    Sxy4_X(idx,2)=vx(NXy+1-i);
    Sxy4_X(idx,4)=vx(NXy+2-i);
    Sxy5_X(i,1)=ux(NXxc+1);
    Sxy5_X(i,3)=ux(NXxc+2);
    Sxy5_X(i,2)=vx(i);
    Sxy5_X(i,4)=vx(i+1); 
    Sxy6_X(i,2)=vx(NXy+1-i);
    Sxy6_X(i,4)=vx(NXy+2-i);
end
Sxy2_X=[Sxy4_X(:,1) Sxy1_X(:,2) Sxy4_X(:,3) Sxy1_X(:,4)];
Sxy3_X=[Sxy1_X(:,1) Sxy4_X(:,2) Sxy1_X(:,3) Sxy4_X(:,4)];
Sxy6_X=[Sxy5_X(:,1) Sxy6_X(:,2) Sxy5_X(:,3) Sxy6_X(:,4)];
Oxy1_X(:,1)=Sxy1_X(:,1);
Oxy1_X(:,2)=Sxy1_X(:,3);
Oxy1_X(:,3)=0.5*(Sxy1_X(:,2)+Sxy1_X(:,4));
Oxy5_X(:,1)=Sxy5_X(:,1);
Oxy5_X(:,2)=Sxy5_X(:,3);
Oxy5_X(:,3)=0.5*(Sxy5_X(:,2)+Sxy5_X(:,4));

Sxy_X=[Sxy1_X; Sxy2_X; Sxy3_X; Sxy4_X; Sxy5_X; Sxy6_X];
Oxy_X=[Oxy1_X; Oxy5_X];

for i=1:NYyc
    idx=(1:NYxc)+(i-1)*NYxc;           
    Sxy1_Y(idx,1)=uy(1:NYxc);
    Sxy1_Y(idx,3)=uy(2:NYxc+1);
    Sxy1_Y(idx,2)=vy(i);
    Sxy1_Y(idx,4)=vy(i+1); 
    Sxy4_Y(idx,1)=fliplr(uy(NYxc+1:NYx));
    Sxy4_Y(idx,3)=fliplr(uy(NYxc+2:NYx+1));
    Sxy4_Y(idx,2)=vy(NYy+1-i);
    Sxy4_Y(idx,4)=vy(NYy+2-i);   
end
Sxy2_Y=[Sxy4_Y(:,1) Sxy1_Y(:,2) Sxy4_Y(:,3) Sxy1_Y(:,4)];
Sxy3_Y=[Sxy1_Y(:,1) Sxy4_Y(:,2) Sxy1_Y(:,3) Sxy4_Y(:,4)];
Sxy5_Y=[Sxy1_Y(1:NYxc,1) Sxy1_Y(end-NYxc+1:end,4) Sxy1_Y(1:NYxc,3) Sxy3_Y(end-NYxc+1:end,2)];
Sxy6_Y=[Sxy4_Y(1:NYxc,1) Sxy5_Y(:,2) Sxy4_Y(1:NYxc,3) Sxy5_Y(:,4)];
    
Oxy1_Y(:,1)=0.5*(Sxy1_Y(:,1)+Sxy1_Y(:,3));  
Oxy1_Y(:,2)=Sxy1_Y(:,2);
Oxy1_Y(:,3)=Sxy1_Y(:,4);
Oxy5_Y(:,1)=0.5*(Sxy5_Y(:,1)+Sxy5_Y(:,3));  
Oxy5_Y(:,2)=Sxy5_Y(:,2);
Oxy5_Y(:,3)=Sxy5_Y(:,4);

Sxy_Y=[Sxy1_Y; Sxy2_Y; Sxy3_Y; Sxy4_Y; Sxy5_Y; Sxy6_Y];
Oxy_Y=[Oxy1_Y; Oxy5_Y];

for i=1:NZyc
    idx=(1:NZxc)+(i-1)*NZxc;
    Sxy1_Z(idx,1)=uz(1:NZxc);
    Sxy1_Z(idx,3)=uz(2:NZxc+1);
    Sxy1_Z(idx,2)=vz(i);
    Sxy1_Z(idx,4)=vz(i+1); 
    Sxy4_Z(idx,1)=fliplr(uz(NZxc+1:NZx));
    Sxy4_Z(idx,3)=fliplr(uz(NZxc+2:NZx+1));
    Sxy4_Z(idx,2)=vz(NZy+1-i);
    Sxy4_Z(idx,4)=vz(NZy+2-i);
end
Sxy2_Z=[Sxy4_Z(:,1) Sxy1_Z(:,2) Sxy4_Z(:,3) Sxy1_Z(:,4)];
Sxy3_Z=[Sxy1_Z(:,1) Sxy4_Z(:,2) Sxy1_Z(:,3) Sxy4_Z(:,4)];

Oxy_Z(:,1)=0.5*(Sxy1_Z(:,1)+Sxy1_Z(:,3));
Oxy_Z(:,2)=0.5*(Sxy1_Z(:,2)+Sxy1_Z(:,4));

Sxy_Z=[Sxy1_Z; Sxy2_Z; Sxy3_Z; Sxy4_Z];

end