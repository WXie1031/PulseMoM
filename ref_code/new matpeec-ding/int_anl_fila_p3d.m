function int = int_anl_fila_p3d(a1, a2, r1, r2, r0, b0, d0, l, n )
%  Function:       int_anl_fila_p2d
%  Description:    calculate annulus segment-point integral 
%                  (for 3D parallel aligned conductors)
%  Calls:          angle_line
%  Input:          ps1  --  start angle of the annulus segment
%                  ps2  --  end angle of the annulus segment
%                  dv1  --  inner radius of the annulus segment
%                  l1   --  outer radius of the annulus segment
%                  r0   --  distance between the point and the centre of 
%                           the circle(which annulus segment belong to)                
%                  b0   --  angle between the point and the centre of the 
%                           circle (which annulus segment belong to)
%                  d0   --  distance between the point and the middle of  
%                           the annulus segment
%                  l    --  length of the conductor
%                  n    --  order of the guass integration(usually n == 5 or 6)
%  Output:         int  --  integral result
%  Others:         1 source line and multi- field linesis are supported 
%  Author:         Chen Hongcai    
%  Email :         hc.chen@live.com      
%  Date:           2014-2-20
%  History:         


Ns = length(d0);


[TT, AA] = gauss_int_coef(n);
a = (a2+a1)/2*ones(1,n) + (a2-a1)/2*TT;

delta = (a2-a1)*(r2.*r2-r1.*r1)/2;

% change to vectors for speeding
r1 = r1*ones(Ns,n);
r2 = r2*ones(Ns,n);
r0 = r0*ones(1,n);
%l = l*ones(1,n);
%d0 = d0*ones(1,n);
a = ones(Ns,1)*a;
AA = ones(Ns,1)*AA;
b0 = b0*ones(1,n);

ang = b0-a;

r1d = r1.*r1;
r2d = r2.*r2;
r0d = r0.*r0;
d02 = d0.*d0;
cosb = cos(ang);
sinb = sin(ang);
%cosbd = cosb.*cosb;
cos2b = cos(2*ang);
R1 = r1d - 2*r1.*r0.*cosb + r0d;
R2 = r2d - 2*r2.*r0.*cosb + r0d;

I1 =  2*(l.*log(l+sqrt(l.*l+d02)) - sqrt(l.*l+d02) + d0 ).*delta;
%I1 =  2*(l.*log(l+sqrt(l.*l+dd)) - sqrt(l.*l+dd) ).*delta;

I12 = ( 1/2*(r1d+2*r1.*r0.*cosb) - 1/2*(r1d-r0d.*cos2b).*log(R1)...
    - 2*r0d.*sinb.*cosb.*atan((r1-r0.*cosb)./(r0.*sinb)) );
I22 = ( 1/2*(r2d+2*r2.*r0.*cosb) - 1/2*(r2d-r0d.*cos2b).*log(R2)...
    - 2*r0d.*sinb.*cosb.*atan((r2-r0.*cosb)./(r0.*sinb)) );

%I13 = ( sqrt(R1).*( 2/3*R1+r0.*r1.*cosb-r0d.*cosbd )...
%    + r0.*r0.*r0.*sinb.^2.*cosb.*log(r1-r0.*cosb+sqrt(R1)) );
%I23 = ( sqrt(R2).*( 2/3*R2+r0.*r2.*cosb-r0d.*cosbd )...
%    + r0.*r0.*r0.*sinb.^2.*cosb.*log(r2-r0.*cosb+sqrt(R2)) );

% I3 = 2*d0;
%I2int = l.*(a2-a1)/2.*sum(A.*(I22-I12),2) + (a2-a1)/2.*sum(A.*(I23-I13),2);
I2int = l .* (a2-a1)/2 .* sum(AA.*(I22-I12),2);

int = ( I1 + I2int );


end

