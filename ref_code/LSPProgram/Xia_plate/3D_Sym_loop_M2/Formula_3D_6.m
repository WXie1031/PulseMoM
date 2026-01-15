% Qy_z

function [F6]=Formula_3D_6(x,y,z)

xx=x.*x; yy=y.*y; zz=z.*z;
R=(xx+yy+zz).^(1/2);
zx=z./x;

F6=y.*z.*(log(y+R)-1)+0.5*(yy-xx).*log(z+R)-0.5*z.*R+x.*y.*(atan(zx)-atan(y.*zx./R));
 
end