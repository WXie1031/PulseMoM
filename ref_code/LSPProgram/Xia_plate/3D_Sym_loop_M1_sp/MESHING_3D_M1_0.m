% mesh generation 
function [Oxy Oxy_X Oxy_Y Oxy_Z Sxy Sxy_X Sxy_Y Sxy_Z u v u0 v0]=MESHING_3D_M1_0(Pl,Ps,N,Ne,r)
X0=Pl(1);  Y0=Pl(2);    
wx=Ps(1);  wy=Ps(2); 
Nx=N(1);   Ny=N(2); 
dep=0.002; ne=5; q=1.5;      % for refinement

if (r)
    [u v]=Refinement_3D_M1_0(Ps,N,dep,ne,q);
    Nx=length(u)-1;
    Ny=length(v)-1;    
else
    u=(-0.5+(0:Nx)/Nx)*wx;         % mesh evenly for plate
    v=(-0.5+(0:Ny)/Ny)*wy;    
end
u=u+X0; v=v+Y0; 

u0=0.5*(u(1:Nx)+u(2:Nx+1));
v0=0.5*(v(1:Ny)+v(2:Ny+1));

uz=u(Ne+1:Nx-Ne+1);                   % the grid applying M1 for Mz (middle area) 
vz=v(Ne+1:Ny-Ne+1);

[Oxy Oxy_X Oxy_Y Oxy_Z Sxy Sxy_X Sxy_Y Sxy_Z]=MESHING_3D_1(u,v,u0,v,u,v0,uz,vz);

% [Oxyz_Z0 Sxyz_Z0]=MESHING_3D_0(u,v,w,Ne);

end