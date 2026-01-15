function [F8,F9]=Formula_3D_F(x,y,z)

xx=x.*x; yy=y.*y; zz=z.*z;
R=(xx+yy+zz).^(1/2);

F8=-sign(x.*z).*atan(abs(x./z).*y./R);
% F9=-0.5*log((x+R)./(x-R));
F9=-log(x+R);

clear x y z xx yy zz R; 
end