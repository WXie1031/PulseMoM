% Qy_x

function [F5]=Formula_3D_5(x,y,z)

xx=x.*x; yy=y.*y; zz=z.*z;
R=(xx+yy+zz).^(1/2);

F5=-z.*log(x+R)-y.*atan(y.*x./z./R);

end