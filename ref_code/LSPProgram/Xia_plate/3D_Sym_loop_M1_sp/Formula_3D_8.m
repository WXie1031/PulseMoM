% By_x

function [F8]=Formula_3D_8(x,y,z)

xx=x.*x; yy=y.*y; zz=z.*z;
R=(xx+yy+zz).^(1/2);

F8=-atan(x./z.*y./R);

end