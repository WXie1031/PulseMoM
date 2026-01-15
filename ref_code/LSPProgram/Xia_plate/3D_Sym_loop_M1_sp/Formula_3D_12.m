% Pmyy

function [F12]=Formula_3D_12(x,y,z)

xx=x.*x; yy=y.*y; zz=z.*z;
R=(xx+yy+zz).^(1/2);

F12=-x.*y./(yy+zz)./R;

clear xx yy zz R; 
end