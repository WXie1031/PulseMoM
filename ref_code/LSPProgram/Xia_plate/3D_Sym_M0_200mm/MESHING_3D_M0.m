% MESHING   Generating coordinates of cells for M0 and M1 zones
%           M1 is in the middle plate and M0 on its two sides
% 
% Ordering: lower -> upper 
%           left  -> right

% 15 Aug 2010
%
function [Oxyz Oxyz_X Oxyz_Y Oxyz_Z Sxyz Sxyz_X Sxyz_Y Sxyz_Z]=MESHING_3D_M0(u,v,ux,vx,uy,vy,uz,vz,z)

% Oxyz(xo,yo,zo),Sxyz(x1,y1,z1,x2,y2,z2)
% Oxyz_X(xo1,xo2,yo,zo),Sxyz_X(x1,y1,z1,x2,y2,z2)
% Oxyz_Y(xo,yo1,yo2,zo),Sxyz_Y(x1,y1,z1,x2,y2,z2)

Nx=length(u)-1;
Ny=length(v)-1;
Nz=length(z)-1;
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

Nxy=Nxc*Nyc;
for i=1:Nz
    for j=1:Nyc
        idx=(1:Nxc)+(j-1)*Nxc+(i-1)*Nxy;
        Sxyz1(idx,1)=u(1:Nxc);
        Sxyz1(idx,4)=u(2:Nxc+1);
        Sxyz1(idx,2)=v(j);
        Sxyz1(idx,5)=v(j+1); 
        Sxyz1(idx,3)=z(i);
        Sxyz1(idx,6)=z(i+1); 
        Sxyz4(idx,1)=fliplr(u(Nxc+1:Nx));
        Sxyz4(idx,4)=fliplr(u(Nxc+2:Nx+1));
        Sxyz4(idx,2)=v(Ny+1-j);
        Sxyz4(idx,5)=v(Ny+2-j);  
    end
end
Sxyz2=[Sxyz4(:,1) Sxyz1(:,2) Sxyz1(:,3) Sxyz4(:,4) Sxyz1(:,5) Sxyz1(:,6)];
Sxyz3=[Sxyz1(:,1) Sxyz4(:,2) Sxyz1(:,3) Sxyz1(:,4) Sxyz4(:,5) Sxyz1(:,6)];
Sxyz4(:,3)=Sxyz1(:,3);
Sxyz4(:,6)=Sxyz1(:,6);
Oxyz(:,1)=0.5*(Sxyz1(:,1)+Sxyz1(:,4));
Oxyz(:,2)=0.5*(Sxyz1(:,2)+Sxyz1(:,5));
Oxyz(:,3)=0.5*(Sxyz1(:,3)+Sxyz1(:,6));
Sxyz=[Sxyz1; Sxyz2; Sxyz3; Sxyz4];

NXxy=NXxc*NXyc;
for i=1:Nz
    for j=1:NXyc
        idx=(1:NXxc)+(j-1)*NXxc+(i-1)*NXxy;
        Sxyz1_X(idx,1)=ux(1:NXxc);
        Sxyz1_X(idx,4)=ux(2:NXxc+1); 
        Sxyz1_X(idx,2)=vx(j);
        Sxyz1_X(idx,5)=vx(j+1);
        Sxyz1_X(idx,3)=z(i);
        Sxyz1_X(idx,6)=z(i+1);     
        Sxyz4_X(idx,1)=fliplr(ux(NXxc+2:NXx));
        Sxyz4_X(idx,4)=fliplr(ux(NXxc+3:NXx+1));
        Sxyz4_X(idx,2)=vx(NXy+1-j);
        Sxyz4_X(idx,5)=vx(NXy+2-j);
        Sxyz5_X((i-1)*NXyc+j,1)=ux(NXxc+1);
        Sxyz5_X((i-1)*NXyc+j,4)=ux(NXxc+2);
        Sxyz5_X((i-1)*NXyc+j,2)=vx(j);
        Sxyz5_X((i-1)*NXyc+j,5)=vx(j+1); 
        Sxyz5_X((i-1)*NXyc+j,3)=z(i);
        Sxyz5_X((i-1)*NXyc+j,6)=z(i+1); 
        Sxyz6_X((i-1)*NXyc+j,2)=vx(NXy+1-j);
        Sxyz6_X((i-1)*NXyc+j,5)=vx(NXy+2-j);
    end
end
Sxyz2_X=[Sxyz4_X(:,1) Sxyz1_X(:,2) Sxyz1_X(:,3) Sxyz4_X(:,4) Sxyz1_X(:,5) Sxyz1_X(:,6)];
Sxyz3_X=[Sxyz1_X(:,1) Sxyz4_X(:,2) Sxyz1_X(:,3) Sxyz1_X(:,4) Sxyz4_X(:,5) Sxyz1_X(:,6)];
Sxyz4_X(:,3)=Sxyz1_X(:,3);
Sxyz4_X(:,6)=Sxyz1_X(:,6);
Sxyz6_X(:,1)=Sxyz5_X(:,1);
Sxyz6_X(:,4)=Sxyz5_X(:,4);
Sxyz6_X(:,3)=Sxyz5_X(:,3);
Sxyz6_X(:,6)=Sxyz5_X(:,6);

Oxyz1_X(:,1)=Sxyz1_X(:,1);
Oxyz1_X(:,2)=Sxyz1_X(:,4);
Oxyz1_X(:,3)=0.5*(Sxyz1_X(:,2)+Sxyz1_X(:,5));
Oxyz1_X(:,4)=0.5*(Sxyz1_X(:,3)+Sxyz1_X(:,6));
Oxyz5_X(:,1)=Sxyz5_X(:,1);
Oxyz5_X(:,2)=Sxyz5_X(:,4);
Oxyz5_X(:,3)=0.5*(Sxyz5_X(:,2)+Sxyz5_X(:,5));
Oxyz5_X(:,4)=0.5*(Sxyz5_X(:,3)+Sxyz5_X(:,6));

Sxyz_X=[Sxyz1_X; Sxyz2_X; Sxyz3_X; Sxyz4_X; Sxyz5_X; Sxyz6_X];
Oxyz_X=[Oxyz1_X; Oxyz5_X];

NYxy=NYxc*NYyc;
for i=1:Nz
    for j=1:NYyc
        idx=(1:NYxc)+(j-1)*NYxc+(i-1)*NYxy;           
        Sxyz1_Y(idx,1)=uy(1:NYxc);
        Sxyz1_Y(idx,4)=uy(2:NYxc+1);
        Sxyz1_Y(idx,2)=vy(j);
        Sxyz1_Y(idx,5)=vy(j+1); 
        Sxyz1_Y(idx,3)=z(i);
        Sxyz1_Y(idx,6)=z(i+1); 
        Sxyz4_Y(idx,1)=fliplr(uy(NYxc+1:NYx));
        Sxyz4_Y(idx,4)=fliplr(uy(NYxc+2:NYx+1));
        Sxyz4_Y(idx,2)=vy(NYy+1-j);
        Sxyz4_Y(idx,5)=vy(NYy+2-j);           
    end
    Sxyz5_Y((i-1)*NYxc+1:i*NYxc,1)=uy(1:NYxc);
    Sxyz5_Y((i-1)*NYxc+1:i*NYxc,4)=uy(2:NYxc+1);
    Sxyz5_Y((i-1)*NYxc+1:i*NYxc,2)=vy(NYyc+1);
    Sxyz5_Y((i-1)*NYxc+1:i*NYxc,5)=vy(NYyc+2);
    Sxyz5_Y((i-1)*NYxc+1:i*NYxc,3)=z(i);
    Sxyz5_Y((i-1)*NYxc+1:i*NYxc,6)=z(i+1);
    Sxyz6_Y((i-1)*NYxc+1:i*NYxc,1)=fliplr(uy(NYxc+1:NYx));
    Sxyz6_Y((i-1)*NYxc+1:i*NYxc,4)=fliplr(uy(NYxc+2:NYx+1));
end
Sxyz2_Y=[Sxyz4_Y(:,1) Sxyz1_Y(:,2) Sxyz1_Y(:,3) Sxyz4_Y(:,4) Sxyz1_Y(:,5) Sxyz1_Y(:,6)];
Sxyz3_Y=[Sxyz1_Y(:,1) Sxyz4_Y(:,2) Sxyz1_Y(:,3) Sxyz1_Y(:,4) Sxyz4_Y(:,5) Sxyz1_Y(:,6)];
Sxyz4_Y(:,3)=Sxyz1_Y(:,3);
Sxyz4_Y(:,6)=Sxyz1_Y(:,6);
Sxyz6_Y(:,2)=Sxyz5_Y(:,2);
Sxyz6_Y(:,5)=Sxyz5_Y(:,5);
Sxyz6_Y(:,3)=Sxyz5_Y(:,3);
Sxyz6_Y(:,6)=Sxyz5_Y(:,6);

Oxyz1_Y(:,1)=0.5*(Sxyz1_Y(:,1)+Sxyz1_Y(:,4));  
Oxyz1_Y(:,2)=Sxyz1_Y(:,2);
Oxyz1_Y(:,3)=Sxyz1_Y(:,5);
Oxyz1_Y(:,4)=0.5*(Sxyz1_Y(:,3)+Sxyz1_Y(:,6)); 
Oxyz5_Y(:,1)=0.5*(Sxyz5_Y(:,1)+Sxyz5_Y(:,4));  
Oxyz5_Y(:,2)=Sxyz5_Y(:,2);
Oxyz5_Y(:,3)=Sxyz5_Y(:,5);
Oxyz5_Y(:,4)=0.5*(Sxyz5_Y(:,3)+Sxyz5_Y(:,6));
Sxyz_Y=[Sxyz1_Y; Sxyz2_Y; Sxyz3_Y; Sxyz4_Y; Sxyz5_Y; Sxyz6_Y];
Oxyz_Y=[Oxyz1_Y; Oxyz5_Y];

NZxy=NZxc*NZyc;
for i=1:Nz
    for j=1:NZyc
        idx=(1:NZxc)+(j-1)*NZxc+(i-1)*NZxy;
        Sxyz1_Z(idx,1)=uz(1:NZxc);
        Sxyz1_Z(idx,4)=uz(2:NZxc+1);
        Sxyz1_Z(idx,2)=vz(j);
        Sxyz1_Z(idx,5)=vz(j+1); 
        Sxyz1_Z(idx,3)=z(i);
        Sxyz1_Z(idx,6)=z(i+1); 
        Sxyz4_Z(idx,1)=fliplr(uz(NZxc+1:NZx));
        Sxyz4_Z(idx,4)=fliplr(uz(NZxc+2:NZx+1));
        Sxyz4_Z(idx,2)=vz(NZy+1-j);
        Sxyz4_Z(idx,5)=vz(NZy+2-j);  
    end
end
Sxyz2_Z=[Sxyz4_Z(:,1) Sxyz1_Z(:,2) Sxyz1_Z(:,3) Sxyz4_Z(:,4) Sxyz1_Z(:,5) Sxyz1_Z(:,6)];
Sxyz3_Z=[Sxyz1_Z(:,1) Sxyz4_Z(:,2) Sxyz1_Z(:,3) Sxyz1_Z(:,4) Sxyz4_Z(:,5) Sxyz1_Z(:,6)];
Sxyz4_Z(:,3)=Sxyz1_Z(:,3);
Sxyz4_Z(:,6)=Sxyz1_Z(:,6);
Oxyz_Z(:,1)=0.5*(Sxyz1_Z(:,1)+Sxyz1_Z(:,4));
Oxyz_Z(:,2)=0.5*(Sxyz1_Z(:,2)+Sxyz1_Z(:,5));
Oxyz_Z(:,3)=0.5*(Sxyz1_Z(:,3)+Sxyz1_Z(:,6));
Sxyz_Z=[Sxyz1_Z; Sxyz2_Z; Sxyz3_Z; Sxyz4_Z];

end