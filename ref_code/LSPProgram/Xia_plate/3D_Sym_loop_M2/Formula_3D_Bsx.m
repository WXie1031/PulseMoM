function [Fsx_Y]=Formula_3D_Bsx(x,y,z)

xx=x.*x; yy=y.*y; zz=z.*z;
R=(xx+yy+zz).^(1/2);

Fsx_Y=-y.*z./(xx+zz)./R;

end