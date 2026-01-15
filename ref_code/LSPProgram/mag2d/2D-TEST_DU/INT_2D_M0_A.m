% infinite 2D integal of functions for 2D Ferro-magnetic plates (dx & dy)
% using the G. Lucca method (method 0)
%    fa=dx/(dx^2+dy^2)
%    fb=dy/(dx^2+dy^2)
%    fc=2*dx*dy/(dx^2+dy^2)^2
%    fdx=(dx^2-dy^2)/(dx^2+dy^2)^2
%    fdy=(dy^2-dx^2)/(dx^2+dy^2)^2
%    fe=1/2*log(dx^2+dy^2)
%    I=int(f(dx,dy),x',y')
%    Ia=0.5*dy*ln(dx^2+dy^2)+dx*atan(dy/dx) 
%    Ib=0.5*dx*ln(dx^2+dy^2)+dy*atan(dx/dy) 
%    Ic=-0.5*ln(dx^2+dy^2) 
%    Idx=atan(dx/dy) 
%    Idy=atan(dy/dx) 
%    2*Ie=dx*dy*[log(dx^2+dy^2)-3]+dx^2*atan(dy/dx)+dy^2*atan(dx/dy) 
% dx[:,:]    x-x'
% dy[:,:]    y-y'

% edited on 8 July 2009

function [Ia,Ib,Ic,Idx,Idy,Ie]=INT_2D_M0_A(dx,dy,r0)
x2=dx.*dx;
y2=dy.*dy;
r2=x2+y2;
xy=dx.*dy;
t1=atan(dy./(dx+r0));
t2=atan(dx./(dy+r0));

Ic=-0.5*log(r2);
Idx=t2;
Idy=t1;
Ia=-dy.*Ic+dx.*t1;
Ib=-dx.*Ic+dy.*t2;
Ie=xy.*(-2*Ic-3)+x2.*t1+y2.*t2;
Ie=0.5*Ie;
end