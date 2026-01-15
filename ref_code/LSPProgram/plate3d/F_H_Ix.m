% F_H_Ix    Formulas for Hyx and Hzy from Ix

function [Hx Hz]=F_H_Ix(x,y,z)

xx=x.*x; yy=y.*y; zz=z.*z;
R=(xx+yy+zz).^(1/2);

Hx=-atan(x.*y./(z.*R));
Hz=-log(x+R);
 
end