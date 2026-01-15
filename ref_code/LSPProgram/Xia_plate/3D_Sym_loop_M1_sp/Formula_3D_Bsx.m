% effect from source Ix
%
function [Bsyx,Bszx]=Formula_3D_Bsx(x,y,z)

xx=x.*x; yy=y.*y; zz=z.*z;
R=(xx+yy+zz).^(1/2);

Bsyx= x.*z./(yy+zz)./R;
Bszx=-x.*y./(yy+zz)./R;

end