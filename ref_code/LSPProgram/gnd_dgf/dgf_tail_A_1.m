function [Ax0,Ax2] = dgf_tail_A_1(dz0,dz2,p,Rx_ji, kn)
%  Function:       dgf_tail_A_sub
%  Description:    calculate the A tail term in DGF.
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


Ax0 = exp(-1j*kn.*r0)./r0;
% Ax1 = Rx_ij .* exp(-1j*kn.*r1)./r1;
Ax2 = Rx_ji .* exp(-1j*kn.*r2)./r2;


end

