% Ly

function [F4]=Formula_3D_4(x,y,z)

xx=x.*x; yy=y.*y; zz=z.*z;
R=(xx+yy+zz).^(1/2);
xz=x./z;

F4=x.*y.*(log(y+R)-1)+0.5*(yy-zz).*log(x+R)-0.5*x.*R+y.*z.*(atan(xz)-atan(y.*xz./R));

end