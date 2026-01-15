% Bz_x

function [F9]=Formula_3D_9(x,y,z)

xx=x.*x; yy=y.*y; zz=z.*z;
R=(xx+yy+zz).^(1/2);

F9=-log(x+R);
 
end