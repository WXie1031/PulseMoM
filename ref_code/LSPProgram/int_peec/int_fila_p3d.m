function int = int_fila_p3d(Xo, Yo, Xf, Yf, l, r0)
%  Function:       int_fila_p3d
%  Description:    Calculate the Parallel Filament-Filament integral.
%
%  Calls:          
%
%  Input:          Xo  --  [N*1] x-axis of the line1(source)
%                  Xf  --  [N*1] x-axis of the line2(field)
%                  Yo  --  [N*1] y-axis of the line1(source)
%                  Yf  --  [N*1] y-axis of the lin2(field)
%                  l   --  [N*1] length of the tape
%                  r0  --  [N*1] equivalent Radius of the conductor for avoid 0
%
%  Output:         int  --  result of the integral
%  Others:
%  Author:         Chen Hongcai
%  Email :         hc.chen@live.com
%  Date:           2014-05-03
%  Update:


No = length(Xo);
Nf = length(Xf);

dX = repmat(Xf',No,1) - repmat(Xo,1,Nf);
dY = repmat(Yf',No,1) - repmat(Yo,1,Nf);

rm = repmat(r0',No,1);
% dX = Xf-Xo;
% dY = Yf-Yo;

% Add tiny number to avoid log(0)
d2 = max( dX.*dX+dY.*dY, rm.*rm );
d = sqrt(d2);

R = sqrt(l.*l+d2);

int = 2*l.*( log((l+R)./d) - R./l + d./l );

end


