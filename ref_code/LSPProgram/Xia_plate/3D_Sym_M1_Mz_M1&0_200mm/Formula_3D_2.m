% Qx_y

function [F2]=Formula_3D_2(x,y,z)

xx=x.*x; yy=y.*y; zz=z.*z;
R=(xx+yy+zz).^(1/2);

F2=z.*log(y+R)+sign(z).*abs(x).*atan(abs(x./z).*y./R);

clear xx yy zz R; 

end