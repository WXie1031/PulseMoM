function [Dx0,Dx1,Dx2,Dx3,Dx4] = dgf_tail_D(dz0,dz1,dz2,dz3,dz4,p, Rx_ij,Rx_ji)
%  Function:       dgf_tail_D
%  Description:    calculate the D tail term in DGF.
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

% int = (D-dz)./p;

r0 = max(1e-9,sqrt(dz0.^2+p.^2));
r1 = max(1e-9,sqrt(dz1.^2+p.^2));
r2 = max(1e-9,sqrt(dz2.^2+p.^2));
r3 = max(1e-9,sqrt(dz3.^2+p.^2));
r4 = max(1e-9,sqrt(dz4.^2+p.^2));



Dx0 = (r0-dz0)./p;
Dx1 = Rx_ij .* (r1-dz1)./p;
Dx2 = Rx_ji .* (r2-dz2)./p;
Dx3 = Rx_ij.*Rx_ji .* (r3-dz3)./p;
Dx4 = Rx_ij.*Rx_ji .* (r4-dz4)./p;



end

