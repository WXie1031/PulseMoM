% Pmzz

function [F13]=Formula_3D_13_0(x,y,z)

xx=x.*x; yy=y.*y; zz=z.*z;
R=(xx+yy+zz).^(1/2);

F13=sign(x.*z).*atan(abs(x./z).*y./R);

clear x y z xx yy zz R; 
end