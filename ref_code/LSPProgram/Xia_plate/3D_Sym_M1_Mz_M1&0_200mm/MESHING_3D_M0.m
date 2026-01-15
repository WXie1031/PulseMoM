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
function [Oxyz_Z0 Sxyz_Z0]=MESHING_3D_M0(u,v,ez,Ne)
Nx=length(u)-1;
Ny=length(v)-1;
Nez=length(ez)-1;
Nex=Nx-2*Ne;
Ney=Ny-2*Ne;

exA=u(1:Ne+1);       % for 4 corners
eyA=v(1:Ne+1);
exD=u(Nx-Ne+1:Nx+1);
eyD=v(Ny-Ne+1:Ny+1);

ue=u(Ne+1:Nx-Ne+1);           % x坐标数组
ve=v(Ne+1:Ny-Ne+1);           % y坐标数组

NNex=Ne*Nex;
for i=1:Nez
    for j=1:Ne
        idx=(1:Nex)+(j-1)*Nex+(i-1)*NNex;
        Sxyz1(idx,1)=ue(1:Nex);
        Sxyz1(idx,4)=ue(2:Nex+1);        
        Sxyz1(idx,2)=eyA(j);
        Sxyz1(idx,5)=eyA(j+1);        
        Sxyz1(idx,3)=ez(i);
        Sxyz1(idx,6)=ez(i+1); 
        Sxyz2(idx,2)=eyD(j);
        Sxyz2(idx,5)=eyD(j+1);
    end
end
Sxyz2(:,1)=Sxyz1(:,1);
Sxyz2(:,4)=Sxyz1(:,4);
Sxyz2(:,3)=Sxyz1(:,3);
Sxyz2(:,6)=Sxyz1(:,6);
Oxyz1(:,1)=0.5*(Sxyz1(:,1)+Sxyz1(:,4));
Oxyz1(:,2)=0.5*(Sxyz1(:,2)+Sxyz1(:,5));
Oxyz1(:,3)=0.5*(Sxyz1(:,3)+Sxyz1(:,6));
Oxyz2(:,1)=Oxyz1(:,1);
Oxyz2(:,2)=0.5*(Sxyz2(:,2)+Sxyz2(:,5));
Oxyz2(:,3)=Oxyz1(:,3);

NNey=Ne*Ney;
for i=1:Nez
    for j=1:Ne
        idx=(1:Ney)+(j-1)*Ney+(i-1)*NNey;
        Sxyz3(idx,1)=exA(j);
        Sxyz3(idx,4)=exA(j+1);
        Sxyz3(idx,2)=ve(1:Ney);
        Sxyz3(idx,5)=ve(2:Ney+1);                  
        Sxyz3(idx,3)=ez(i);
        Sxyz3(idx,6)=ez(i+1); 
        Sxyz4(idx,1)=exD(j);
        Sxyz4(idx,4)=exD(j+1);
    end
end
Sxyz4(:,2)=Sxyz3(:,2);
Sxyz4(:,5)=Sxyz3(:,5);
Sxyz4(:,3)=Sxyz3(:,3);
Sxyz4(:,6)=Sxyz3(:,6);
Oxyz3(:,1)=0.5*(Sxyz3(:,1)+Sxyz3(:,4));
Oxyz3(:,2)=0.5*(Sxyz3(:,2)+Sxyz3(:,5));
Oxyz3(:,3)=0.5*(Sxyz3(:,3)+Sxyz3(:,6));
Oxyz4(:,1)=0.5*(Sxyz4(:,1)+Sxyz4(:,4));
Oxyz4(:,2)=Oxyz3(:,2);
Oxyz4(:,3)=Oxyz3(:,3);

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
        SxyzD(idx,1)=exD(1:Ne);
        SxyzD(idx,4)=exD(2:Ne+1);        
        SxyzD(idx,2)=eyD(j);
        SxyzD(idx,5)=eyD(j+1);        
    end
end
SxyzD(:,3)=SxyzA(:,3);
SxyzD(:,6)=SxyzA(:,6);

OxyzA(:,1)=0.5*(SxyzA(:,1)+SxyzA(:,4));
OxyzA(:,2)=0.5*(SxyzA(:,2)+SxyzA(:,5));
OxyzA(:,3)=0.5*(SxyzA(:,3)+SxyzA(:,6));
OxyzD(:,1)=0.5*(SxyzD(:,1)+SxyzD(:,4));
OxyzD(:,2)=0.5*(SxyzD(:,2)+SxyzD(:,5));
OxyzD(:,3)=OxyzA(:,3);

SxyzB=[SxyzD(:,1) SxyzA(:,2) SxyzA(:,3) SxyzD(:,4) SxyzA(:,5) SxyzA(:,6)];
SxyzC=[SxyzA(:,1) SxyzD(:,2) SxyzA(:,3) SxyzA(:,4) SxyzD(:,5) SxyzA(:,6)];

OxyzB=[OxyzD(:,1) OxyzA(:,2) OxyzA(:,3)];
OxyzC=[OxyzA(:,1) OxyzD(:,2) OxyzA(:,3)];

Oxyz_Z0=[Oxyz1; Oxyz2; Oxyz3; Oxyz4; OxyzA; OxyzB; OxyzC; OxyzD];
Sxyz_Z0=[Sxyz1; Sxyz2; Sxyz3; Sxyz4; SxyzA; SxyzB; SxyzC; SxyzD];

clear Oxyz1 Oxyz2 Oxyz3 Oxyz4 OxyzA OxyzB OxyzC OxyzD;
clear Sxyz1 Sxyz2 Sxyz3 Sxyz4 SxyzA SxyzB SxyzC SxyzD;

end
