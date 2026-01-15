% F_H_Iy    Formulas for Hxy and Hzy from Iy

function [Hx Hz]=F_H_Iy(x,y,z)

xx=x.*x; yy=y.*y; zz=z.*z;
R=(xx+yy+zz).^(1/2);

Hx=atan(x.*y./(z.*R));
Hz=log(y + R);

end