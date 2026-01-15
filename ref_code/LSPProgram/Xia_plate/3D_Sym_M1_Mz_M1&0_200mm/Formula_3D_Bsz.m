function [Fsz_X,Fsz_Y]=Formula_3D_Bsz(ux,uy,uz,vx,vy,vz)

uxx=ux.*ux; uyy=uy.*uy; uzz=uz.*uz;
Ru=(uxx+uyy+uzz).^(1/2);

vxx=vx.*vx; vyy=vy.*vy; vzz=vz.*vz;
Rv=(vxx+vyy+vzz).^(1/2);

Fsz_X=-ux.*uy./(uyy+uzz)./Ru;

Fsz_Y=vx.*vy./(vxx+vzz)./Rv;

clear uxx uyy uzz Ru vxx vyy vzz Rv; 

end