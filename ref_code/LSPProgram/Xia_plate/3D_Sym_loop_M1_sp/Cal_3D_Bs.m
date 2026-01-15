
function [Bxs,Bys,Bzs]=Cal_3D_Bs(Oxyz,Qs,Is,Ns)

Nsx=Ns(1); Nsy=Ns(2); Nsz=Ns(3);
Isx=ones(1,Nsx); Isy=ones(1,Nsy); Isz=ones(1,Nsz);
Ix=Is(1:Nsx)'; Iy=Is(Nsx+1:Nsx+Nsy)'; Iz=Is(Nsx+Nsy+1:Nsx+Nsy+Nsz)';
Qsx=Qs(1:Nsx,:);
Qsy=Qs(Nsx+1:Nsx+Nsy,:);
Qsz=Qs(Nsx+Nsy+1:Nsx+Nsy+Nsz,:);

[no mo]=size(Oxyz);
Io=ones(no,1);

Byx=0; Bzx=0;
if (Nsx)
    xo=Oxyz(:,1)*Isx;
    yo=Oxyz(:,2)*Isx;
    zo=Oxyz(:,3)*Isx;
    
    xs1=Io*Qsx(:,1)';
    xs2=Io*Qsx(:,2)';
    ys=Io*Qsx(:,3)';
    zs=Io*Qsx(:,4)';
    
    dx1=xo-xs1;
    dx2=xo-xs2;
    dy=yo-ys;
    dz=zo-zs;
    
    [Bsyx,Bszx]=Formula_3D_Bsx(dx1,dy,dz);
    Byx=-Bsyx;
    Bzx=-Bszx;
    
    [Bsyx,Bszx]=Formula_3D_Bsx(dx2,dy,dz);
    Byx=Byx+Bsyx;
    Bzx=Bzx+Bszx;
    
    Byx=Byx*Ix;
    Bzx=Bzx*Ix;    
end

Bxy=0; Bzy=0;
if (Nsy)
    xo=Oxyz(:,1)*Isy;
    yo=Oxyz(:,2)*Isy;
    zo=Oxyz(:,3)*Isy;
    
    xs=Io*Qsy(:,1)';
    ys1=Io*Qsy(:,2)';
    ys2=Io*Qsy(:,3)';    
    zs=Io*Qsy(:,4)';
    
    dx=xo-xs;
    dy1=yo-ys1;
    dy2=yo-ys2;
    dz=zo-zs;
    
    [Bsxy,Bszy]=Formula_3D_Bsy(dx,dy1,dz);
    Bxy=-Bsxy;
    Bzy=-Bszy;
    
    [Bsxy,Bszy]=Formula_3D_Bsy(dx,dy2,dz);
    Bxy=Bxy+Bsxy;
    Bzy=Bzy+Bszy;
    
    Bxy=Bxy*Iy;
    Bzy=Bzy*Iy;    
end

Bxz=0; Byz=0;
if (Nsz)
    xo=Oxyz(:,1)*Isz;
    yo=Oxyz(:,2)*Isz;
    zo=Oxyz(:,3)*Isz;
    
    xs=Io*Qsz(:,1)';
    ys=Io*Qsz(:,2)';
    zs1=Io*Qsz(:,3)';
    zs2=Io*Qsz(:,4)';
    
    dx=xo-xs;    
    dy=yo-ys;
    dz1=zo-zs1;
    dz2=zo-zs2;
    
    [Bsxz,Bsyz]=Formula_3D_Bsz(dx,dy,dz1);
    Bxz=-Bsxz;
    Byz=-Bsyz;
    
    [Bsxz,Bsyz]=Formula_3D_Bsz(dx,dy,dz2);
    Bxz=Bxz+Bsxz;
    Byz=Byz+Bsyz;
    
    Bxz=Bxz*Iz;
    Byz=Byz*Iz;    
end

Bxs=Bxy+Bxz;
Bys=Byx+Byz;
Bzs=Bzx+Bzy;

end
