function [Bsxz,Bsyz]=Formula_3D_Bsz(x,y,z)

xx=x.*x; yy=y.*y; zz=z.*z;
R=(xx+yy+zz).^(1/2);

Bsxz= y.*z./(xx+yy)./R;
Bsyz=-x.*z./(xx+yy)./R;

end