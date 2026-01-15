% Pmzx and Pmzx

function [F15]=Formula_3D_15(x,y,z)

xx=x.*x; yy=y.*y; zz=z.*z;
R=(xx+yy+zz).^(1/2);

F15=-log(y+R);

end