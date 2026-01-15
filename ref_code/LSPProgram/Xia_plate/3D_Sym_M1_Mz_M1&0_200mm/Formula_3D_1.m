% Lx 

function [F1]=Formula_3D_1(x,y,z)

xx=x.*x; yy=y.*y; zz=z.*z;
R=(xx+yy+zz).^(1/2);

F1=x.*y.*(log(x+R)-1)+0.5*(xx-zz).*log(y+R)+x.*z.*atan(y./z)-0.5*y.*R-abs(x.*z).*atan(abs(x./z).*y./R);

clear xx yy zz R; 

end