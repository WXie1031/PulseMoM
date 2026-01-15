function [Fsz]=Formula_3D_Sz(x,y,z)

xx=x.*x; yy=y.*y; zz=z.*z;
R=(xx+yy+zz).^(1/2);

Fsz=-z.*log(z+R)+R;

end