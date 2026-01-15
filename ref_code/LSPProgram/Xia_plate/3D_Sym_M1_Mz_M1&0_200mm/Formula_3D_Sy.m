function [Fsy]=Formula_3D_Sy(x,y,z)

xx=x.*x; yy=y.*y; zz=z.*z;
R=(xx+yy+zz).^(1/2);

Fsy=-y.*log(y+R)+R;

clear xx yy zz R; 

end