function [F7,F10]=Formula_3D_E(x,y,z)

xx=x.*x; yy=y.*y; zz=z.*z;
R=(xx+yy+zz).^(1/2);

F7=sign(x.*z).*atan(abs(x./z).*y./R);
F10=log(y + R);

clear x y z xx yy zz R; 
end