function [F5,F6]=Formula_3D_D(x,y,z)

xx=x.*x; yy=y.*y; zz=z.*z;
R=(xx+yy+zz).^(1/2);

F5=-z.*log(x+R)-sign(z).*abs(y).*atan(abs(y./z).*x./R);
% F6=R-0.5*y.*log((y+R)./(y-R));
F6=R+1i*y.*atan(1i*R./y);

clear x y z xx yy zz R; 
end