function [F11,F12,F13,F14,F15,F16]=Formula_3D_G(x,y,z)

xx=x.*x; yy=y.*y; zz=z.*z;
R=(xx+yy+zz).^(1/2);

F11=sign(x.*y).*atan(abs(y./x).*z./R);
F12=sign(x.*y).*atan(abs(x./y).*z./R);
F13=sign(y.*z).*atan(abs(y./z).*x./R);
F14=-log(z+R);
F15=-log(x+R);
F16=-log(y+R);

clear x y z xx yy zz R; 
end