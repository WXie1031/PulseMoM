% MESHING   Generating coordinates of cells for M0 and M1 zones
%           M1 is in the middle plate and M0 on its two sides
% 
% Ordering: lower -> upper 
%           left  -> right

% 15 Aug 2010
%
function [Oxyz Oxyz_X Oxyz_Y Sxyz Sxyz_X Sxyz_Y]=MESHING_3D(u,v,z,ux,vy,kz)

% Oxyz(xo,yo,zo),Sxyz(x1,y1,z1,x2,y2,z2)
% Oxyz_X(xo1,xo2,yo,zo),Sxyz_X(x1,y1,z1,x2,y2,z2)
% Oxyz_Y(xo,yo1,yo2,zo),Sxyz_Y(x1,y1,z1,x2,y2,z2)

Nx=length(u)-1;
Ny=length(v)-1;
Nz=length(z)-1;
NxX=Nx-1;
NyY=Ny-1;

Nxy=Nx*Ny;
for i=1:Nz
    for j=1:Ny
        idx=(1:Nx)+(j-1)*Nx+(i-1)*Nxy;
        Sxyz(idx,1)=u(1:Nx);
        Sxyz(idx,4)=u(2:Nx+1);
        Sxyz(idx,2)=v(j);
        Sxyz(idx,5)=v(j+1);
        Sxyz(idx,3)=z(i);
        Sxyz(idx,6)=z(i+1);   
    end
end
Oxyz(:,1)=0.5*(Sxyz(:,1)+Sxyz(:,4));
Oxyz(:,2)=0.5*(Sxyz(:,2)+Sxyz(:,5));
Oxyz(:,3)=Sxyz(:,3)+kz*(Sxyz(:,6)-Sxyz(:,3));

NxyX=NxX*Ny;
for i=1:Nz
    for j=1:Ny
        idx=(1:NxX)+(j-1)*NxX+(i-1)*NxyX;
        Sxyz_X(idx,1)=ux(1:NxX);
        Sxyz_X(idx,4)=ux(2:NxX+1); 
        Sxyz_X(idx,2)=v(j);
        Sxyz_X(idx,5)=v(j+1);
        Sxyz_X(idx,3)=z(i);
        Sxyz_X(idx,6)=z(i+1);        
    end
end
Oxyz_X(:,1)=Sxyz_X(:,1);
Oxyz_X(:,2)=Sxyz_X(:,4);
Oxyz_X(:,3)=0.5*(Sxyz_X(:,2)+Sxyz_X(:,5));
Oxyz_X(:,4)=Sxyz_X(:,3)+kz*(Sxyz_X(:,6)-Sxyz_X(:,3));

NxyY=Nx*NyY;
for i=1:Nz
    for j=1:NyY
        idx=(1:Nx)+(j-1)*Nx+(i-1)*NxyY;           
        Sxyz_Y(idx,1)=u(1:Nx);
        Sxyz_Y(idx,4)=u(2:Nx+1);
        Sxyz_Y(idx,2)=vy(j);
        Sxyz_Y(idx,5)=vy(j+1);
        Sxyz_Y(idx,3)=z(i);
        Sxyz_Y(idx,6)=z(i+1);   
    end
end
Oxyz_Y(:,1)=0.5*(Sxyz_Y(:,1)+Sxyz_Y(:,4));  
Oxyz_Y(:,2)=Sxyz_Y(:,2);
Oxyz_Y(:,3)=Sxyz_Y(:,5);
Oxyz_Y(:,4)=Sxyz_Y(:,3)+kz*(Sxyz_Y(:,6)-Sxyz_Y(:,3));

end