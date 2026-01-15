function [Fsy_X]=Formula_3D_Bsy(x,y,z)

xx=x.*x; yy=y.*y; zz=z.*z;
R=(xx+yy+zz).^(1/2);

Fsy_X=x.*z./(yy+zz)./R;

end