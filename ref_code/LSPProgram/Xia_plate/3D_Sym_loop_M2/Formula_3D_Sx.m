function [Fsx]=Formula_3D_Sx(x,y,z)

xx=x.*x; yy=y.*y; zz=z.*z;
R=(xx+yy+zz).^(1/2);

Fsx=-x.*log(x+R)+R;

end