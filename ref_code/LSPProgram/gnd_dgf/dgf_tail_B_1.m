function [Bx0,Bx2] = dgf_tail_B_1(dz0,dz2,p, Rx_ji)
%  Function:       dgf_tail_B
%  Description:    calculate the B tail term in DGF.
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

r0 = max(1e-9,sqrt(dz0.^2+p.^2));
% r1 = max(1e-9,sqrt(dz1.^2+p.^2));
r2 = max(1e-9,sqrt(dz2.^2+p.^2));



Bx0 = p ./ (r0.^3);
% Bx1 = Rx_ij .* 1./p .* (1-dz1./r1);
Bx2 = Rx_ji .* p ./ (r2.^3);


end

