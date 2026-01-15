
function [Sx,Sy,Sz]=Cal_3D_Js(XOxyz,YOxyz,ZOxyz,Qs,Is,Ns)

Nsx=Ns(1); Nsy=Ns(2); Nsz=Ns(3);
Isx=ones(1,Nsx); Isy=ones(1,Nsy); Isz=ones(1,Nsz);
Ix=Is(1:Nsx)'; Iy=Is(Nsx+1:Nsx+Nsy)'; Iz=Is(Nsx+Nsy+1:Nsx+Nsy+Nsz)';
Qsx=Qs(1:Nsx,:);
Qsy=Qs(Nsx+1:Nsx+Nsy,:);
Qsz=Qs(Nsx+Nsy+1:Nsx+Nsy+Nsz,:);

[nox mox]=size(XOxyz);
[noy moy]=size(YOxyz);
[noz moz]=size(ZOxyz);

Iox=ones(nox,1);
Ioy=ones(noy,1);
Ioz=ones(noz,1);

Sx=[];
if (Nsx&&nox)
    xo1=XOxyz(:,1)*Isx;
    xo2=XOxyz(:,2)*Isx;
    yo=XOxyz(:,3)*Isx;
    zo=XOxyz(:,4)*Isx;
    
    xs1=Iox*Qsx(:,1)';
    xs2=Iox*Qsx(:,2)';
    ys=Iox*Qsx(:,3)';
    zs=Iox*Qsx(:,4)';
    
    dx11=xo1-xs1;
    dx12=xo1-xs2;
    dx21=xo2-xs1;
    dx22=xo2-xs2;
    dy=yo-ys;
    dz=zo-zs;

    [Fsx]=Formula_3D_Sx(dx11,dy,dz);
    Sx=Fsx;
    [Fsx]=Formula_3D_Sx(dx12,dy,dz);
    Sx=Sx-Fsx;
    [Fsx]=Formula_3D_Sx(dx21,dy,dz);
    Sx=Sx-Fsx;
    [Fsx]=Formula_3D_Sx(dx22,dy,dz);
    Sx=Sx+Fsx;
    
    Sx=Sx*Ix;  
end

Sy=[];
if (Nsy&&noy)
    xo=YOxyz(:,1)*Isy;
    yo1=YOxyz(:,2)*Isy;
    yo2=YOxyz(:,3)*Isy;
    zo=YOxyz(:,4)*Isy;
    
    xs=Ioy*Qsy(:,1)';
    ys1=Ioy*Qsy(:,2)';
    ys2=Ioy*Qsy(:,3)';    
    zs=Ioy*Qsy(:,4)';
    
    dx=xo-xs;
    dy11=yo1-ys1;
    dy12=yo1-ys2;
    dy21=yo2-ys1;
    dy22=yo2-ys2;
    dz=zo-zs;
    
    [Fsy]=Formula_3D_Sy(dx,dy11,dz);
    Sy=Fsy;
    [Fsy]=Formula_3D_Sy(dx,dy12,dz);
    Sy=Sy-Fsy;
    [Fsy]=Formula_3D_Sy(dx,dy21,dz);
    Sy=Sy-Fsy;
    [Fsy]=Formula_3D_Sy(dx,dy22,dz);
    Sy=Sy+Fsy;
    
    Sy=Sy*Iy;    
end

Sz=[];
if (Nsz&&noz)
    xo=ZOxyz(:,1)*Isz;
    yo=ZOxyz(:,2)*Isz;
    zo1=ZOxyz(:,3)*Isz;
    zo2=ZOxyz(:,4)*Isz;
    
    xs=Ioz*Qsz(:,1)';
    ys=Ioz*Qsz(:,2)';
    zs1=Ioz*Qsz(:,3)';
    zs2=Ioz*Qsz(:,4)';
    
    dx=xo-xs;    
    dy=yo-ys;
    dz11=zo1-zs1;
    dz12=zo1-zs2;
    dz21=zo2-zs1;
    dz22=zo2-zs2;
    
    [Fsz]=Formula_3D_Sz(dx,dy,dz11);
    Sz=Fsz;
    [Fsz]=Formula_3D_Sz(dx,dy,dz12);
    Sz=Sz-Fsz;
    [Fsz]=Formula_3D_Sz(dx,dy,dz21);
    Sz=Sz-Fsz;
    [Fsz]=Formula_3D_Sz(dx,dy,dz22);
    Sz=Sz+Fsz;
    
    Sz=Sz*Iz;  
end

end
