function [F2,F3]=Formula_3D_B(x,y,z)

xx=x.*x; yy=y.*y; zz=z.*z;
R=(xx+yy+zz).^(1/2);

F2=z.*log(y+R)+sign(z).*abs(x).*atan(abs(x./z).*y./R);
% F3=-R+0.5*x.*log((x+R)./(x-R));
F3=-R-1i*x.*atan(1i*R./x);

clear x y z xx yy zz R; 

end