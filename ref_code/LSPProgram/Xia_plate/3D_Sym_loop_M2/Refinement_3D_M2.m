% refinement for edge area of the plate
% dep      the depth affected by skin effect
% ne       the number of meshes for dep
% q        the ratio for generating mesh for dep
%
function [u v]=Refinement_3D_M2(Ps,N,dep,ne,q)
wx=Ps(1);  wy=Ps(2); 
Nx=N(1);   Ny=N(2);   
u=(-0.5+(0:Nx)/Nx)*wx;
v=(-0.5+(0:Ny)/Ny)*wy;
du=wx/Nx;
dv=wy/Ny;
p=1.2;      % the ratio for generating mesh for the over zone

ae=dep*(1-q)/(1-q^ne);      % the first item of mesh during dep
ue=ae*q.^(0:ne-1);          % generating mesh for dep

am=ae*q^ne;                 % the first item of mesh during transition zone

um=[]; vm=[];
au=am; av=am;
while (au<du)               % generating mesh for transition zone          
    um=[um au];
    au=au*q;    
end
while (av<dv)
    vm=[vm av];
    av=av*q;    
end
num=length(um);
nvm=length(vm);
Nue=ne+num;                 % the number of mesh for edge zone
Nve=ne+nvm;
eu=sum(um)+dep;             % the width of edge zone
ev=sum(vm)+dep;
Nu=1+fix(eu/du);
Nv=1+fix(ev/dv);

tu=Nu*du-eu;
tv=Nv*dv-ev;
atu=tu*(1-p)/(1-p^Nue);
atv=tv*(1-p)/(1-p^Nve);
ut=atu*p.^(0:Nue-1);
vt=atv*p.^(0:Nve-1);        % the difference mesh

ur=[ue um]+ut;
vr=[ue vm]+vt;
u1=[]; v1=[];
ud=0; vd=0;
for i=1:Nue
    ud=ud+ur(i);
    u1=[u1 ud];
end
for i=1:Nve
    vd=vd+vr(i);
    v1=[v1 vd];
end
    
ur=u1+u(1);
vr=v1+v(1);
ur=[u(1) ur];
vr=[v(1) vr];

u=[ur  u(Nu+2:Nx-Nu) -fliplr(ur)];           % the total partition
v=[vr  v(Nv+2:Ny-Nv) -fliplr(vr)];

end