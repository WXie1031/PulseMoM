% Pmzz

function [F13]=Formula_3D_13(x,y,z)

xx=x.*x; yy=y.*y; zz=z.*z;
R=(xx+yy+zz).^(1/2);

%F13=sign(x.*z).*atan(abs(x./z).*y./R);
F13=x.*y.*(xx+yy+2*zz)./(yy+zz)./(xx+zz)./R;

clear xx yy zz R; 

end