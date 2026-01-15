function [Coe_jx,Coe_jy,Coe_ux,Coe_uy]=Coef(NxX,NyX,NxY,NyY,Nz)

Nx=NxX+1;
NxyX=NxX*NyX;
NpX=Nx*NyX-1;

Ny=NyY+1;
NxyY=NxY*NyY;
NpY=NxY*Ny-1;

one=ones(1,NxX);
Ejx1=diag(one,1);
Ejx1=Ejx1(:,2:Nx);
Ejx2=diag(one,-1);
Ejx2=Ejx2(:,1:NxX);
Ejx=Ejx1-Ejx2;
Ejx=sparse(Ejx);
clear Ejx1 Ejx2;

Eux1=speye(NxX,Nx);
Eux2=diag(one,1);
Eux2=Eux2(1:NxX,:);
Eux=Eux1-Eux2;
Eux=sparse(Eux);
clear one Eux1 Eux2;

for i=1:Nz
    for j=1:NyX
        Coe_j(Nx*(j-1)+1:Nx*j,NxX*(j-1)+1:NxX*j)=Ejx;
        Coe_u(NxX*(j-1)+1:NxX*j,Nx*(j-1)+1:Nx*j)=Eux;
    end
    Coe_jx(NpX*(i-1)+1:NpX*i,NxyX*(i-1)+1:NxyX*i)=Coe_j(1:NpX,:); 
    Coe_ux(NxyX*(i-1)+1:NxyX*i,NpX*(i-1)+1:NpX*i)=Coe_u(:,1:NpX);    
end

clear Ejx Eux Coe_j Coe_u;

Ejy=speye(NxY);
for k=1:Nz
    for i=1:NyY        
        Coe_j(NxY*(i-1)+1:NxY*i,NxY*(i-1)+1:NxY*i)=Ejy;
        Coe_j(NxY*i+1:NxY*(i+1),NxY*(i-1)+1:NxY*i)=-Ejy;
        Coe_u(NxY*(i-1)+1:NxY*i,NxY*(i-1)+1:NxY*i)=Ejy;
        Coe_u(NxY*(i-1)+1:NxY*i,NxY*i+1:NxY*(i+1))=-Ejy;
    end
    Coe_jy(NpY*(k-1)+1:NpY*k,NxyY*(k-1)+1:NxyY*k)=Coe_j(1:NpY,:); 
    Coe_uy(NxyY*(k-1)+1:NxyY*k,NpY*(k-1)+1:NpY*k)=Coe_u(:,1:NpY);  
end

clear Ejy Coe_j Coe_u;

end