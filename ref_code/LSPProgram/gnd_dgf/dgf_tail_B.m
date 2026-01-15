function [Bx0,Bx1,Bx2,Bx3,Bx4] = dgf_tail_B(dz0,dz1,dz2,dz3,dz4,p, Rx_ij,Rx_ji)
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
r1 = max(1e-9,sqrt(dz1.^2+p.^2));
r2 = max(1e-9,sqrt(dz2.^2+p.^2));
r3 = max(1e-9,sqrt(dz3.^2+p.^2));
r4 = max(1e-9,sqrt(dz4.^2+p.^2));


Bx0 = p ./ (r0.^3);
Bx1 = Rx_ij .* p ./ (r1.^3);
Bx2 = Rx_ji .* p ./ (r2.^3);
Bx3 = Rx_ij.*Rx_ji .* p ./ (r3.^3);
Bx4 = Rx_ij.*Rx_ji .* p ./ (r4.^3);


end

