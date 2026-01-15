function [Dx,Dy] = int_bar_fila_mag_p3d_D(xs1,xs2,ys1,ys2,xf,yf,len)
%  Function:       mag_rec_D
%  Description:    calculate annulus segment-point integral 
%                  (for 3D parallel aligned conductors)
%                   z1 = 0, z2 = len 
%  Calls:          
%  Input:          xs1   --   
%                  xs2   --  
%                  ys1   --  
%                  ys2   --  
%                  xf    --  
%                  yf    --  
%                  len   --  
% 
%  Output:         mag_D  --  integral result

Ns = size(xf,1);

len = len*ones(Ns,1);

x1 = xs1 *ones(Ns,1) - xf;
x2 = xs2 *ones(Ns,1) - xf;
y1 = ys1 *ones(Ns,1) - yf;
y2 = ys2 *ones(Ns,1) - yf;

R112 = sqrt( (x1).^2 + (y1).^2 + len.^2 );

R122 = sqrt( (x1).^2 + (y2).^2 + len.^2 );

R212 = sqrt( (x2).^2 + (y1).^2 + len.^2 );

R222 = sqrt( (x2).^2 + (y2).^2 + len.^2 );

% 1. hi(x,z)
%    hi(x,z) = Kx1 + Kx2 + Ky1 + Ky2 + Kz2

Kx1 = x1.*(atanh(R122./len) - atanh(R112./len));

Kx2 = x2.*(atanh(R212./len) - atanh(R222./len));

Ky1 = y1.*(atan( x1.*len./(y1.*R112) ) - atan( x2.*len./(y1.*R212) ));

Ky2 = y2.*(atan( x2.*len./(y2.*R222) ) - atan( x1.*len./(y2.*R122) ));

Kz2 = len.*(atanh(R122./x1) - atanh(R112./x1) + atanh(R212./x2) - atanh(R222./x2)); 

hixz = Kx1 + Kx2 + Ky1 + Ky2 + Kz2;

%2. hi(y,z)
%   hi(y,z) = Mx1 + Mx2 + My1 + My2 + Mz2

Mx1 = x1.*( atan( len.*y1./(x1.*R112) ) - atan( len.*y2./(x1.*R122) ));

Mx2 = x2.*( atan( len.*y2./(x2.*R222) ) - atan( len.*y1./(x2.*R212) ));

My1 = y1.*( atanh( R212./len ) - atanh( R112./len ));

My2 = y2.*( atanh( R122./len ) - atanh( R222./len ));

Mz2 = len.*( atanh( R212./y1 ) - atanh( R112./y1 ) + atanh( R122./y2 ) - atanh( R222./y2 ));

hiyz = Mx1 + Mx2 + My1 + My2 + Mz2;

Dx = -hixz;

Dy = -hiyz;
end

