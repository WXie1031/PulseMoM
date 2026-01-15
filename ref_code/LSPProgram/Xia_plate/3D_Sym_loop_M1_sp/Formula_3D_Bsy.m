function [Bsxy,Bszy]=Formula_3D_Bsy(x,y,z)

xx=x.*x; yy=y.*y; zz=z.*z;
R=(xx+yy+zz).^(1/2);

Bsxy=-y.*z./(xx+zz)./R;
Bszy= x.*y./(xx+zz)./R;

end