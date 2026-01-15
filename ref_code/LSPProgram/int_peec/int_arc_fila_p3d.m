function int = int_arc_fila_p3d(a1, a2, r, r0, b0, d0, l, Nint)
%  Function:       int_anl_fila_p3d
%  Description:    calculate arc segment-filament integral 
%                  (for 3D parallel aligned conductors)
%  Calls:          
%  Input:          a1   --  start angle of the arc segment
%                  a2   --  end angle of the arc segment
%                  r    --  radius of the arc segment
%                  r0   --  distance between the point and the centre of 
%                           the circle(which arc segment belong to)                
%                  b0   --  angle between the point and the centre of the 
%                           circle (which annulus segment belong to)
%                  d0   --  distance between the point and the middle of  
%                           the annulus segment
%                  l    --  length of the conductor
%                  n    --  order of the guass integration(usually n == 5 or 6)
%  Output:         int  --  integral result
%  Output:         int  --  integral result
%  Others:         1 source line and multi- field lines is supported 
%  Author:         Chen Hongcai    
%  Email :         hc.chen@live.com      
%  Date:           2018-5-20
%  History:  


% if nargin <= 8
%     ver = 2;
% end

ver = 1;

Ns = length(b0);

[TT, AA] = gauss_int_coef(Nint);
a = (a2+a1)/2*ones(1,Nint) + (a2-a1)/2*TT;

da = (a2-a1); % arc length
% dr = r; % arc length
dr = 1;

% change to vectors for speeding
r = r*ones(Ns,Nint);
r0 = r0*ones(1,Nint);
a = ones(Ns,1)*a;
AA = ones(Ns,1)*AA;
b0 = b0*ones(1,Nint);

d2 = r.*r + r0.*r0 - 2.*r.*r0.*cos(b0-a);

switch ver
    case 1 % Analytic with Much Simplification
        d02 = d0.*d0;
        I1 = 2*( l.*log(l+sqrt(l.*l+d02)) - sqrt(l.*l+d02) + sqrt(d02) )*da;
        
        I2 = -l.*log(d2);
        Iint = (a2-a1)/2.*sum(AA.*I2,2);
        
        int = ( I1 + Iint ).*dr;
        
    case 2 % Analytic and Numerical Integation with Patial Simplification
        d02 = d0.*d0;
        I1 = 2*( l.*log(l+sqrt(l.*l+d02)) - sqrt(l.*l+d02) )*da;

        I2 = -l.*log(d2);
        I3 = 2*sqrt(d2);

        Iint = (a2-a1)/2.*sum(AA.*(I2+I3),2);

        int = ( I1 + Iint ).*dr;
        
    case 3 % Numerical Integation
        l = l*ones(1,Nint);

        I = l.* ( 2*log(l+sqrt(l.*l+d2)) - log(d2) ) ...
            - 2*( sqrt(l.*l+d2) - sqrt(d2) );
             
        int = (a2-a1)/2.*sum(AA.*I,2).*dr ;

end


end


