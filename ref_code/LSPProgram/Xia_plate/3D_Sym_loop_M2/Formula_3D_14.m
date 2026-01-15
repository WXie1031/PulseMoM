% Pmxy  Pmyx

function [F14]=Formula_3D_14(x,y,z)

xx=x.*x; yy=y.*y; zz=z.*z;
R=(xx+yy+zz).^(1/2);

F14=-log(z+R);
 
end