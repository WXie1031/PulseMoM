function [Coe_jx,Coe_jy,Coe_ux,Coe_uy]=Coef_M2(Ps,N)

wx=Ps(1);  wy=Ps(2);    
Nx=N(1);   Ny=N(2);   
u=(-0.5+(0:Nx)/Nx)*wx;
v=(-0.5+(0:Ny)/Ny)*wy;
du=u(2:Nx+1)-u(1:Nx);
dv=v(2:Ny+1)-v(1:Ny);

Nxc=Nx/2; Nyc=Ny/2;
NXxc=Nxc-1;
NYyc=Nyc-1;

du=du(1:Nxc);
dv=dv(1:Nyc);

one=ones(1,NXxc);
Ejx1=diag(one,1);
Ejx1=Ejx1(:,2:Nxc);
Ejx2=diag(one,-1);
Ejx2=Ejx2(:,1:NXxc);
Ejx=Ejx1-Ejx2;
Ejx=sparse(Ejx);
clear Ejx1 Ejx2;

Eux1=speye(NXxc,Nxc);
Eux2=diag(one,1);
Eux2=Eux2(1:NXxc,:);
Eux=Eux1-Eux2;
Eux=sparse(Eux);
clear one Eux1 Eux2;

for j=1:Nyc
    Coe_j(Nxc*(j-1)+1:Nxc*j,NXxc*(j-1)+1:NXxc*j)=Ejx*dv(j);
    Coe_u(NXxc*(j-1)+1:NXxc*j,Nxc*(j-1)+1:Nxc*j)=Eux; 
    Coe_j_m(j*Nxc,j)=1*dv(j);
    Coe_u_m(j,j*Nxc)=2;        
end
Coe_jx=[Coe_j  Coe_j_m];
Coe_ux=[Coe_u; Coe_u_m];

clear Ejx Eux Coe_j Coe_u Coe_j_m Coe_u_m;

Ejy=sparse(1:Nxc,1:Nxc,du);
Euy=speye(Nxc);
for i=1:NYyc        
    Coe_j(Nxc*(i-1)+1:Nxc*i,Nxc*(i-1)+1:Nxc*i)=Ejy;
    Coe_j(Nxc*i+1:Nxc*(i+1),Nxc*(i-1)+1:Nxc*i)=-Ejy;
    Coe_u(Nxc*(i-1)+1:Nxc*i,Nxc*(i-1)+1:Nxc*i)=Euy;
    Coe_u(Nxc*(i-1)+1:Nxc*i,Nxc*i+1:Nxc*(i+1))=-Euy;   
end
Coe_j_m(Nxc*NYyc+1:Nxc*Nyc,1:Nxc)=Ejy;
Coe_u_m(1:Nxc,Nxc*NYyc+1:Nxc*Nyc)=2*Euy;

Coe_jy=[Coe_j  Coe_j_m];
Coe_uy=[Coe_u; Coe_u_m];

clear Ey Coe_j Coe_u Coe_j_m Coe_u_m;

end