% Pmxx Pmyy

function [F13]=Formula_3D_13(x,y,z)

xx=x.*x; yy=y.*y; zz=z.*z;
R=(xx+yy+zz).^(1/2);

F13=-atan(x.*y./(z.*R));

end