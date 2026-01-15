function int = int_anl_fila_p3d(a1, a2, r1, r2, r0, b0, d0, l, nint)
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
%                  d0   --  distance between the point and the middle of  
%                           the annulus segment
%                  l    --  length of the conductor
%                  n    --  order of the guass integration(usually n == 5 or 6)
%  Output:         int  --  integral result
%  Others:         1 source line and multi- field lines is supported 
%  Author:         Chen Hongcai    
%  Email :         hc.chen@live.com      
%  Date:           2014-2-20
%  History:         

ver = 2;

Ns = length(d0);


[TT, AA] = gauss_int_coef(nint);
a = (a2+a1)/2*ones(1,nint) + (a2-a1)/2*TT;

delta = (a2-a1)*(r2.*r2-r1.*r1)/2;

% change to vectors for speeding
r1 = r1*ones(Ns,nint);
r2 = r2*ones(Ns,nint);
r0 = r0*ones(1,nint);
%l = l*ones(1,n);
%d0 = d0*ones(1,n);
a = ones(Ns,1)*a;
AA = ones(Ns,1)*AA;
b0 = b0*ones(1,nint);

ang = b0-a;

r1d = r1.*r1;
r2d = r2.*r2;
r0d = r0.*r0;
d02 = d0.*d0;
cosb = cos(ang);
sinb = sin(ang);
%cosbd = cosb.*cosb;
cos2b = cos(2*ang);
R1 = r1d + r0d - 2*r1.*r0.*cosb;
R2 = r2d + r0d - 2*r2.*r0.*cosb;

I1 =  2*(l.*log(l+sqrt(l.*l+d02)) - sqrt(l.*l+d02) + d0 ).*delta;
%I1 =  2*(l.*log(l+sqrt(l.*l+dd)) - sqrt(l.*l+dd) ).*delta;

I12 = ( 1/2*(r1d+2*r1.*r0.*cosb) - 1/2*(r1d-r0d.*cos2b).*log(R1)...
    - 2*r0d.*sinb.*cosb.*atan((r1-r0.*cosb)./(r0.*sinb)) );
I22 = ( 1/2*(r2d+2*r2.*r0.*cosb) - 1/2*(r2d-r0d.*cos2b).*log(R2)...
    - 2*r0d.*sinb.*cosb.*atan((r2-r0.*cosb)./(r0.*sinb)) );


switch ver
    case 1  % no simplification
        cosbd = cosb.*cosb;
        
        I13 = ( sqrt(R1).*( 2/3*R1+r0.*r1.*cosb-r0d.*cosbd )...
            + r0.*r0.*r0.*sinb.^2.*cosb.*log(r1-r0.*cosb+sqrt(R1)) );
        I23 = ( sqrt(R2).*( 2/3*R2+r0.*r2.*cosb-r0d.*cosbd )...
            + r0.*r0.*r0.*sinb.^2.*cosb.*log(r2-r0.*cosb+sqrt(R2)) );
        
        I2int = l.*(a2-a1)/2.*sum(AA.*(I22-I12),2) + (a2-a1)/2.*sum(AA.*(I23-I13),2);
        
    case 2  
        I2int = l .* (a2-a1)/2 .* sum(AA.*(I22-I12),2);
        
end


int = ( I1 + I2int );


end

