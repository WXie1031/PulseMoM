function [Txx,Txy,Tyy] = mag_rec_T(xs1,xs2,ys1,ys2,xf,yf,len)
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
%  Output:         mag_T  --  integral result

% the self integral is negtive !
idself = find(sign((xf-xs1).*(xf-xs2))<0 & sign((yf-ys1).*(yf-ys2))<0);%


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

% 1. hm(x,y) = hm(y,x) = T1 + T2 + T3 + T4

T1 = asinh( len./sqrt(x2.^2 + y2.^2) );

T2 = -asinh( len./sqrt(x1.^2 + y2.^2) );

T3 = asinh( len./sqrt(x1.^2 + y1.^2) );

T4 = -asinh( len./sqrt(x2.^2 + y1.^2) );

hmxy = T1 + T2 + T3 + T4;

% 2 hmxx = Q1 + Q2 + Q3 + Q4

Q1 = -atan( y2.*len./(x2.*R222) );

Q2 = atan( y1.*len./(x2.*R212) );

Q3 = atan( y2.*len./(x1.*R122) );

Q4 = -atan( y1.*len./(x1.*R112) );

hmxx = Q1 + Q2 + Q3 + Q4;

hmxx(idself) = -hmxx(idself) ;

% hmyy = Z1 + Z2 + Z3 + Z4

Z1 = -atan( x2.*len./(y2.*R222) );

Z2 = atan( x1.*len./(y2.*R122) );

Z3 = atan( x2.*len./(y1.*R212) );

Z4 = -atan( x1.*len./(y1.*R112) );

hmyy = Z1 + Z2 + Z3 + Z4;
hmyy(idself) = -hmyy(idself) ;
     
     Txx = hmxx;
     Txy = hmxy;
     Tyy = hmyy;

end

