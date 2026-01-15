function [Cx0,Cx2] = dgf_tail_C_1(dz0,dz2,p, Rx_ji)
%  Function:       dgf_tail_C_1
%  Description:    calculate the C tail term in DGF.
%                  kz and rr must have the same demension.
%  Calls:          
%
%  Input:          p   --  rho in x-y plane
%                  dz  --  input distance in z direction
%                  D   --  is the distance in 3D spatial
%
%  Output:         int  --  result
%
%  Others:
%  Author:         Chen Hongcai
%  Email :         hc.chen@live.com
%  Date:           2017-05-31

% int = p ./ D.^3;

r0 = max(1e-9,sqrt(dz0.^2+p.^2));
% r1 = max(1e-9,sqrt(dz1.^2+p.^2));
r2 = max(1e-9,sqrt(dz2.^2+p.^2));


Cx0 = 1 ./ r0;
% Cx1 = Rx_ij .* 1 ./ r1;
Cx2 = Rx_ji .* 1 ./ r2;



end

