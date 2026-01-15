function [Fsy_X,Fsz_X]=Formula_3D_Bs_X(x,y,z)

xx=x.*x; yy=y.*y; zz=z.*z;
R=(xx+yy+zz).^(1/2);

Fsy_X=x.*z./(yy+zz)./R;
Fsz_X=-x.*y./(yy+zz)./R;

clear x y z xx yy zz R; 

end