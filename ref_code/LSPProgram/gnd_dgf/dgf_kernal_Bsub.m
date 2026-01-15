function [Ax0,Ax1,Ax2,Ax3,Ax4] = dgf_kernal_Bsub(dz0,dz1,dz2,dz3,dz4, Rx_ij,Rx_ji, kp)
%  Function:       dgf_kernal_A
%  Description:
%
%                  left medium | L1 | L2 | ... | LM | right medium
%                    interface 1    2    3     M   M+1
%
%                  The left and the right medium is related with the
%                  direction of the waves. If the wave is in the right
%                  direction, 'left medium' is the first layer, and the
%                  'right medium' is the last medium. In this situation,
%                  the relection coefficient of the 'right medium' is 0.
%  Calls:
%
%  Input:          para_lyr --  parameter of each layer (Nx1)
%                  kz   --  wave number
%
%  Output:         int  --  reflection coefficients (N-1)x1
%
%  Others:
%  Author:         Chen Hongcai
%  Email :         hc.chen@live.com
%  Date:           2017-03-27


% the location of the points   nn
% dz0 = abs(zs-zf);
% dz1 = 2*zbdy(nn) - (zf+zs);
% dz2 = (zf+zs) - 2*zbdy(nn-1);
% dz3 = 2*zbdy(nn-1) - (zf-zs);
% dz4 = 2*zbdy(nn-1) + (zf-zs);

% Ae0 = exp(-1j*kz.*dz0);
% Ae1 = GRx_ij./Dx .* exp(-1j*kz.*dz1);
% Ae2 = GRx_ji./Dx .* exp(-1j*kz.*dz2);
% Ae3 = GRx_ij.*GRx_ji./Dx .* exp(-1j*kz.*dz3);
% Ae4 = GRx_ij.*GRx_ji./Dx .* exp(-1j*kz.*dz4);

% kx=kz -- Asub
% kx=kp -- Bsub

Nf = size(dz0,1);
Nint = size(kp,2);

Ax0 = exp(-repmat(kp,Nf,1).*repmat(dz0,1,Nint));
Ax1 = Rx_ij.*exp(-repmat(kp,Nf,1).*repmat(dz1,1,Nint));
Ax2 = Rx_ji.*exp(-repmat(kp,Nf,1).*repmat(dz2,1,Nint));
Ax3 = Rx_ij.*Rx_ji.*exp(-repmat(kp,Nf,1).*repmat(dz3,1,Nint));
Ax4 = Rx_ij.*Rx_ji.*exp(-repmat(kp,Nf,1).*repmat(dz4,1,Nint));


end

