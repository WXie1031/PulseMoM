% MESHING   Generating coordinates of cells for M0 and M1 zones
%           M1 is in the middle plate and M0 on its two sides
% 
% Ordering: lower -> upper 
%           left  -> right

% 15 Oct 2010
%
function [Oxy Oxy_X Oxy_Y Oxy_Z Sxy Sxy_X Sxy_Y Sxy_Z]=MESHING_3D(u,v,ux,vx,uy,vy,uz,vz)
% Oxy(xo,yo),Sxy(x1,y1,x2,y2)
% Oxy_X(xo1,xo2,yo),Sxy_X(x1,y1,x2,y2)
% Oxy_Y(xo,yo1,yo2),Sxy_Y(x1,y1,x2,y2)
Nx=length(u)-1;
Ny=length(v)-1;
NxX=length(ux)-1;
NyX=length(vx)-1;
NxY=length(uy)-1;
NyY=length(vy)-1;
NxZ=length(uz)-1;
NyZ=length(vz)-1;

for i=1:Ny
    idx=(1:Nx)+(i-1)*Nx;
    Sxy(idx,1)=u(1:Nx);
    Sxy(idx,3)=u(2:Nx+1);
    Sxy(idx,2)=v(i);
    Sxy(idx,4)=v(i+1);      
end
Oxy(:,1)=0.5*(Sxy(:,1)+Sxy(:,3));
Oxy(:,2)=0.5*(Sxy(:,2)+Sxy(:,4));

for i=1:NyX   
    idx=(1:NxX)+(i-1)*NxX;
    Sxy_X(idx,1)=ux(1:NxX);
    Sxy_X(idx,3)=ux(2:NxX+1); 
    Sxy_X(idx,2)=vx(i);
    Sxy_X(idx,4)=vx(i+1); 
end
Oxy_X(:,1)=Sxy_X(:,1);
Oxy_X(:,2)=Sxy_X(:,3);
Oxy_X(:,3)=0.5*(Sxy_X(:,2)+Sxy_X(:,4));

for i=1:NyY
    idx=(1:NxY)+(i-1)*NxY;           
    Sxy_Y(idx,1)=uy(1:NxY);
    Sxy_Y(idx,3)=uy(2:NxY+1);
    Sxy_Y(idx,2)=vy(i);
    Sxy_Y(idx,4)=vy(i+1);    
end
Oxy_Y(:,1)=0.5*(Sxy_Y(:,1)+Sxy_Y(:,3));  
Oxy_Y(:,2)=Sxy_Y(:,2);
Oxy_Y(:,3)=Sxy_Y(:,4);

for i=1:NyZ
    idx=(1:NxZ)+(i-1)*NxZ;           
    Sxy_Z(idx,1)=uz(1:NxZ);
    Sxy_Z(idx,3)=uz(2:NxZ+1);
    Sxy_Z(idx,2)=vz(i);
    Sxy_Z(idx,4)=vz(i+1);    
end
Oxy_Z(:,1)=0.5*(Sxy_Z(:,1)+Sxy_Z(:,3));  
Oxy_Z(:,2)=0.5*(Sxy_Z(:,2)+Sxy_Z(:,4));

end