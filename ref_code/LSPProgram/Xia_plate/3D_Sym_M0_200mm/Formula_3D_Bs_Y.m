function [Fsx_Y,Fsz_Y]=Formula_3D_Bs_Y(x,y,z)

xx=x.*x; yy=y.*y; zz=z.*z;
R=(xx+yy+zz).^(1/2);

Fsx_Y=-y.*z./(xx+zz)./R;
Fsz_Y=x.*y./(xx+zz)./R;

clear x y z xx yy zz R; 

end