function [Dxz,Dyz,Txx,Txy,Tyy] = int_anl_fila_mag_old(a1, a2, r1, r2, r0, b0, l, Nint1,Nint2)
%  Function:       int_anl_fila_p3d
%  Description:    calculate annulus segment-point integral 
%                  (for 3D parallel aligned conductors)
%  Calls:          
%  Input:          a1   --  start angle of the annulus segment
%                  a2   --  end angle of the annulus segment
%                  r1   --  inner radius of the annulus segment
%                  r2   --  outer radius of the annulus segment
%                  r0   --  distance between the point and the centre of 
%                           the circle(which annulus segment belong to)                
%                  b0   --  angle between the point and the centre of the 
%                           circle (which annulus segment belong to)
%                  l    --  length of the conductor
%                  Nint1 --  order of the guass integration of angle(usually n == 5 or 6)
%                  Nint2 --  order of the guass integration of radius(usually n == 5 or 6)
%  Output:         intD,intT  --  integral result

Ns = length(b0);


[TT1, AA1] = gauss_int_coef(Nint1);
[TT2, AA2] = gauss_int_coef(Nint2);

a = (a2+a1)/2*ones(1,Nint1) + (a2-a1)/2*TT1;
r = (r2+r1)/2*ones(1,Nint2) + (r2-r1)/2*TT2;

% dS = (a2-a1).*(r2.*r2-r1.*r1)/2;

% delta = (a2-a1)*(r2.*r2-r1.*r1)/2;

% change to vectors for speeding

r0 = r0*ones(1,Nint1);
r0d = r0.*r0;
a = ones(Ns,1)*a;
r = ones(Ns,1)*r;
l = l*ones(1,Nint1);
l2 = l.*l;

AA1 = ones(Ns,1)*AA1;
AA2 = ones(Ns,1)*AA2;

b0 = b0*ones(1,Nint1);
rx0 = r0.*cos(b0);
ry0 = r0.*sin(b0);

% ang = b0-a;
cosb = cos(a);
sinb = sin(a);

ra = rx0.*cosb + ry0.*sinb;


finDxz = zeros(Ns,Nint2);
finDyz = zeros(Ns,Nint2);
finTxx = zeros(Ns,Nint2);
finTyy = zeros(Ns,Nint2);
finTxy = zeros(Ns,Nint2);

for k = 1:Nint2
    
    rk = r(:,k)*ones(1,Nint1);
    x = rk.*cosb;
    y = rk.*sinb;
%     x2 = x.*x;
%     y2 = y.*y;
    rkd = rk.*rk;
    x2ay2 = rkd + r0d - 2*rk.*ra;
    
  % matrix D: -y/R^3 dv   
  
  % I1 = 2*rk.*rk.*sinb - 2*ry.*rk;
    I1 = 2*y.*rk - 2*ry0.*rk;
    I2 = sqrt(x2ay2);
    I3 = I1.*(sqrt(x2ay2 + l2));
    I4 = x2ay2;
    
    faD1 = I1./I2 - I3./I4;
    
  % matrix D: x/R^3 dv
  
    I5 = 2*x.*rk - 2*rx0.*rk;
    I6 = I5.*(sqrt(x2ay2 + l2));
   
    faD2 =  (I5./I2 - I6./I4);
 
  % matrix T: 3x^2/R^5 dv
  
    T1 = 2*(x-rx0).^2.*rk;
    T2 = x2ay2.^1.5;
    T3 = T1.*(2*l2+x2ay2);
    T4 = x2ay2.^2.*sqrt(l2+x2ay2);
    
    faT1 = -T1./T2 + T3./T4;
    
 % matrix T: 3y^2/R^5 dv
    
    T5 = 2*(y-ry0).^2.*rk;
    T6 = T2;
    T7 = T5.*(2*l2+x2ay2);
    T8 = T4;
    
    faT2 = -T5./T6 + T7./T8;
    
  % matrix T: 1/R^3 dv
  
    faT3 = - 2*rk./I2 + 2*rk.*sqrt(x2ay2 + l2)./x2ay2;
  
  % matrix T: 3xy/R^5 dv
  
     T9 = 2*(x-rx0).*(y-ry0).*rk;   
     T10 = T9.*(2*l2+x2ay2);
     
    faT4 = -T9./T2 + T10./T4;
     
    finDxz(:,k) = (a2-a1)/2.* sum(AA1.*(faD1),2);
    finDyz(:,k) = (a2-a1)/2.* sum(AA1.*(faD2),2);
    
    finTxx(:,k) = (a2-a1)/2.* sum(AA1.*(faT1-faT3),2);
    finTxy(:,k) = (a2-a1)/2.* sum(AA1.*(faT4),2);
    finTyy(:,k) = (a2-a1)/2.* sum(AA1.*(faT2-faT3),2);
   
end


Dxz = (r2-r1)/2.*sum(AA2.*finDxz,2);
Dyz = (r2-r1)/2.*sum(AA2.*finDyz,2);

Txx = (r2-r1)/2.*sum(AA2.*(finTxx),2);
Txy = (r2-r1)/2.*sum(AA2.*(finTxy),2);
Tyy = (r2-r1)/2.*sum(AA2.*(finTyy),2);



end