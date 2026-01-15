% Pmyz

function [F15]=Formula_3D_15_yz(x,y,z)

xx=x.*x; yy=y.*y; zz=z.*z;
R=(xx+yy+zz).^(1/2);

F15=-log(x+R);

clear xx yy zz R; 

end