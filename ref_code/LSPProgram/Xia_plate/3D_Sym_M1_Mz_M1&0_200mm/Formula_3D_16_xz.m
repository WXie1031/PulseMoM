% Pmxz

function [F16]=Formula_3D_16_xz(x,y,z)

xx=x.*x; yy=y.*y; zz=z.*z;
R=(xx+yy+zz).^(1/2);

F16=-log(y+R);

clear xx yy zz R; 
end