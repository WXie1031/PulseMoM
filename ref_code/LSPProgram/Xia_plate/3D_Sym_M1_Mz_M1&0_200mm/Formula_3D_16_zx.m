% F16   Pmzx

function [F16]=Formula_3D_16_zx(x,y,z)

xx=x.*x; yy=y.*y; zz=z.*z;
R=(xx+yy+zz).^(1/2);

F16=-y.*z./(xx+zz)./R;

clear xx yy zz R; 
end