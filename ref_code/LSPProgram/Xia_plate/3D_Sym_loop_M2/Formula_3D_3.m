% Qx_z

function [F3]=Formula_3D_3(x,y,z)

xx=x.*x; yy=y.*y; zz=z.*z;
R=(xx+yy+zz).^(1/2);
zy=z./y;

F3=x.*z.*(log(x+R)-1)+0.5*(xx-yy).*log(z+R)-0.5*z.*R+x.*y.*(atan(zy)-atan(x.*zy./R));

end