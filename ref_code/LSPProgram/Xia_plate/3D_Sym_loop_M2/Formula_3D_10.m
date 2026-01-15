% Bz_y

function [F10]=Formula_3D_10(x,y,z)

xx=x.*x; yy=y.*y; zz=z.*z;
R=(xx+yy+zz).^(1/2);

F10=log(y + R);

end