% F15   Pmzy

function [F15]=Formula_3D_15_zy(x,y,z)

xx=x.*x; yy=y.*y; zz=z.*z;
R=(xx+yy+zz).^(1/2);

F15=-x.*z./(yy+zz)./R;

clear xx yy zz R; 

end