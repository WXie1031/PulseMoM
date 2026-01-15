% mesh generation for the edge blocks contain Mz
% 
% Ordering: lower -> upper 
%           left  -> right
% ****C******2******D****
% ***********************
% ****3*************4****
% ***********************
% ****A******1******B****
%
% 17/02/2011
%
function [Oxyz_Z0 Sxyz_Z0]=MESHING_3D_M0_1(u,v,ez,Ne)
Nx=length(u)-1;
Ny=length(v)-1;
Nez=length(ez)-1;
Nex=Nx-2*Ne;
Ney=Ny-2*Ne;
Nxc=Nex/2;
Nyc=Ney/2;

exA=u(1:Ne+1);       % for 4 corners
eyA=v(1:Ne+1);
exD=u(Nx-Ne+1:Nx+1);
eyD=v(Ny-Ne+1:Ny+1);

ue=u(Ne+1:Nx-Ne+1);           % x坐标数组
ve=v(Ne+1:Ny-Ne+1);           % y坐标数组

NNex=Ne*Nxc;
for i=1:Nez
    for j=1:Ne
        idx=(1:Nxc)+(j-1)*Nxc+(i-1)*NNex;
        Sxyz1_X(idx,1)=ue(1:Nxc);
        Sxyz1_X(idx,4)=ue(2:Nxc+1);        
        Sxyz1_X(idx,2)=eyA(j);
        Sxyz1_X(idx,5)=eyA(j+1);        
        Sxyz1_X(idx,3)=ez(i);
        Sxyz1_X(idx,6)=ez(i+1);         
        Sxyz4_X(idx,1)=fliplr(ue(Nxc+1:Nex));
        Sxyz4_X(idx,4)=fliplr(ue(Nxc+2:Nex+1));    
        Sxyz4_X(idx,2)=eyD(Ne+1-j);
        Sxyz4_X(idx,5)=eyD(Ne+2-j);
    end
end
Sxyz2_X=[Sxyz4_X(:,1) Sxyz1_X(:,2) Sxyz1_X(:,3) Sxyz4_X(:,4) Sxyz1_X(:,5) Sxyz1_X(:,6)];
Sxyz3_X=[Sxyz1_X(:,1) Sxyz4_X(:,2) Sxyz1_X(:,3) Sxyz1_X(:,4) Sxyz4_X(:,5) Sxyz1_X(:,6)];
Sxyz4_X(:,3)=Sxyz1_X(:,3);
Sxyz4_X(:,6)=Sxyz1_X(:,6);

Oxyz1_X(:,1)=0.5*(Sxyz1_X(:,1)+Sxyz1_X(:,4));
Oxyz1_X(:,2)=0.5*(Sxyz1_X(:,2)+Sxyz1_X(:,5));
Oxyz1_X(:,3)=0.5*(Sxyz1_X(:,3)+Sxyz1_X(:,6));

NNey=Ne*Nyc;
for i=1:Nez
    for j=1:Ne
        idx=(1:Nyc)+(j-1)*Nyc+(i-1)*NNey;
        Sxyz1_Y(idx,1)=exA(j);
        Sxyz1_Y(idx,4)=exA(j+1);        
        Sxyz1_Y(idx,2)=ve(1:Nyc);
        Sxyz1_Y(idx,5)=ve(2:Nyc+1);        
        Sxyz1_Y(idx,3)=ez(i);
        Sxyz1_Y(idx,6)=ez(i+1);         
        Sxyz4_Y(idx,1)=exD(Ne+1-j);        
        Sxyz4_Y(idx,4)=exD(Ne+2-j);            
        Sxyz4_Y(idx,2)=fliplr(ve(Nyc+1:Ney));
        Sxyz4_Y(idx,5)=fliplr(ve(Nyc+2:Ney+1));
    end
end
Sxyz2_Y=[Sxyz1_Y(:,1) Sxyz4_Y(:,2) Sxyz1_Y(:,3) Sxyz1_Y(:,4) Sxyz4_Y(:,5) Sxyz1_Y(:,6)];
Sxyz3_Y=[Sxyz4_Y(:,1) Sxyz1_Y(:,2) Sxyz1_Y(:,3) Sxyz4_Y(:,4) Sxyz1_Y(:,5) Sxyz1_Y(:,6)];
Sxyz4_Y(:,3)=Sxyz1_Y(:,3);
Sxyz4_Y(:,6)=Sxyz1_Y(:,6);

Oxyz1_Y(:,1)=0.5*(Sxyz1_Y(:,1)+Sxyz1_Y(:,4));
Oxyz1_Y(:,2)=0.5*(Sxyz1_Y(:,2)+Sxyz1_Y(:,5));
Oxyz1_Y(:,3)=0.5*(Sxyz1_Y(:,3)+Sxyz1_Y(:,6));

NNe=Ne*Ne;
for i=1:Nez
    for j=1:Ne
        idx=(1:Ne)+(j-1)*Ne+(i-1)*NNe;
        SxyzA(idx,1)=exA(1:Ne);
        SxyzA(idx,4)=exA(2:Ne+1);        
        SxyzA(idx,2)=eyA(j);
        SxyzA(idx,5)=eyA(j+1);        
        SxyzA(idx,3)=ez(i);
        SxyzA(idx,6)=ez(i+1);   
        SxyzD(idx,1)=fliplr(exD(1:Ne));
        SxyzD(idx,4)=fliplr(exD(2:Ne+1));        
        SxyzD(idx,2)=eyD(Ne+1-j);
        SxyzD(idx,5)=eyD(Ne+2-j);        
    end
end
SxyzD(:,3)=SxyzA(:,3);
SxyzD(:,6)=SxyzA(:,6);

OxyzA(:,1)=0.5*(SxyzA(:,1)+SxyzA(:,4));
OxyzA(:,2)=0.5*(SxyzA(:,2)+SxyzA(:,5));
OxyzA(:,3)=0.5*(SxyzA(:,3)+SxyzA(:,6));

SxyzB=[SxyzD(:,1) SxyzA(:,2) SxyzA(:,3) SxyzD(:,4) SxyzA(:,5) SxyzA(:,6)];
SxyzC=[SxyzA(:,1) SxyzD(:,2) SxyzA(:,3) SxyzA(:,4) SxyzD(:,5) SxyzA(:,6)];

Oxyz_Z0=[Oxyz1_X; Oxyz1_Y; OxyzA];
Sxyz_Z0=[Sxyz1_X; Sxyz1_Y; SxyzA; Sxyz2_X; Sxyz3_Y; SxyzB; Sxyz3_X; Sxyz2_Y; SxyzC; Sxyz4_X; Sxyz4_Y; SxyzD];

clear Oxyz1 Oxyz2 Oxyz3 Oxyz4 OxyzA OxyzB OxyzC OxyzD;
clear Sxyz1 Sxyz2 Sxyz3 Sxyz4 SxyzA SxyzB SxyzC SxyzD;

end
