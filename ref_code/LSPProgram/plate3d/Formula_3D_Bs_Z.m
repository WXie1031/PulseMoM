function [Fsx_Z Fsy_Z]=Formula_3D_Bs_Z(x,y,z)

xx=x.*x; yy=y.*y; zz=z.*z;
R=(xx+yy+zz).^(1/2);

Fsx_Z= y.*z./(yy+xx)./R;
Fsy_Z=-x.*x./(yy+xx)./R;

clear x y z xx yy zz R; 

end