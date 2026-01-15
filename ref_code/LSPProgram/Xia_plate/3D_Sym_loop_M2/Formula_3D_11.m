% Pmyy Pmzz

function [F11]=Formula_3D_11(x,y,z)

xx=x.*x; yy=y.*y; zz=z.*z;
R=(xx+yy+zz).^(1/2);

F11=-atan(y.*z./(x.*R));

end