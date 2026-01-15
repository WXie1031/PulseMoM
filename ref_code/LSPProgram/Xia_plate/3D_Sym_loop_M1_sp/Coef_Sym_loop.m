% Jx=Coex*I;  Jy=Coey*I;

function [Coex,Coey]=Coef_Sym_loop(u,v,N)
   
Nx=N(1);   Ny=N(2);

Nxc=Nx/2; Nyc=Ny/2;
NXxc=Nxc-1;
%NYyc=Nyc-1;

du=u(2:Nx+1)-u(1:Nx);
dv=v(2:Ny+1)-v(1:Ny);
du=du.^(-1);
dv=dv.^(-1);

Ex=eye(NXxc,Nxc);
Ey1=diag(du(1:Nxc),0);
Ey2=diag(du(2:Nxc),-1);
Ey=Ey1-Ey2;

for i=1:Nyc
    Coex(NXxc*(i-1)+1:NXxc*i,Nxc*(i-1)+1:Nxc*i)=-Ex*dv(i);
    Coem(i,Nxc*i)=-dv(i);    
    if (i~=Nyc)
        Coex(NXxc*i+1:NXxc*(i+1),Nxc*(i-1)+1:Nxc*i)=Ex*dv(i+1);
        Coem(i+1,Nxc*i)=dv(i+1);
    end    
    Coey(Nxc*(i-1)+1:Nxc*i,Nxc*(i-1)+1:Nxc*i)=Ey; 
end
Coex=[Coex; Coem];

end