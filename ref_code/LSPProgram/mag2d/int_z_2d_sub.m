% infinite 2D integal of functions for 2D Ferro-magnetic plates (dx & dy)
% using the G. Lucca method (method 0)
%    fa=dx/(dx^2+dy^2)
%    fb=dy/(dx^2+dy^2)
%    fc=2*dx*dy/(dx^2+dy^2)^2
%    fd=(dx^2-dy^2)/(dx^2+dy^2)^2
%    fe=1/2*log(dx^2+dy^2)
%    I=int(f(dx,dy),x',y')

%    2*Ie=dx*dy*[log(dx^2+dy^2)-3]+dx^2*atan(dy/dx)+dy^2*atan(dx/dy) 
% dx[:,:]    x-x'
% dy[:,:]    y-y'

% edited on 8 July 2009

function Iz = int_z_2d_sub(dx,dy)

dx2 = dx.*dx;
dy2 = dy.*dy;
r2  = dx2+dy2;
dxy = dx.*dy;
t1  = atan(dy./dx);
t2  = atan(dx./dy);

% Ic = -0.5*log(r2);
% Iz = 0.5 * (dxy.*(-2*Ic-3)+dx2.*t1+dy2.*t2);
Iz = 0.5 * (dxy.*(log(r2)-3)+dx2.*t1+dy2.*t2);

end

