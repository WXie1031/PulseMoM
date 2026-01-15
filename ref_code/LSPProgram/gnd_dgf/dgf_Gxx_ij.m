function [Ax0,Ax1,Ax2,Ax3,Ax4] = dgf_Gxx_ij(dz0,dz1,dz2,dz3,dz4,Dx, GRx_ij,GRx_ji, kz)
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
% dz3 = 2*zbdy(nn-1) + (zf-zs);
% dz4 = 2*zbdy(nn-1) - (zf-zs);

% Ae0 = exp(-1j*kz.*dz0);
% Ae1 = GRx_ij./Dx .* exp(-1j*kz.*dz1);
% Ae2 = GRx_ji./Dx .* exp(-1j*kz.*dz2);
% Ae3 = GRx_ij.*GRx_ji./Dx .* exp(-1j*kz.*dz3);
% Ae4 = GRx_ij.*GRx_ji./Dx .* exp(-1j*kz.*dz4);


Ax0 = 0;
Ax1 = 0;
Ax2 = 0;
Ax3 = 0;
Ax4 = 0;

Nf = size(dz0,1);
Nkp_int = size(Dx,2);

Ax0 = Ax0 + exp(-1j*repmat(kz,Nf,1).*repmat(dz0,1,Nkp_int));
Ax1 = Ax1 + repmat(GRx_ij,Nf,1)./repmat(Dx,Nf,1).*exp(-1j*repmat(kz,Nf,1).*repmat(dz1,1,Nkp_int));
Ax2 = Ax2 + repmat(GRx_ji,Nf,1)./repmat(Dx,Nf,1).*exp(-1j*repmat(kz,Nf,1).*repmat(dz2,1,Nkp_int));
Ax3 = Ax3 + repmat(GRx_ij,Nf,1).*repmat(GRx_ji,Nf,1)./repmat(Dx,Nf,1).*exp(-1j*repmat(kz,Nf,1).*repmat(dz3,1,Nkp_int));
Ax4 = Ax4 + repmat(GRx_ij,Nf,1).*repmat(GRx_ji,Nf,1)./repmat(Dx,Nf,1).*exp(-1j*repmat(kz,Nf,1).*repmat(dz4,1,Nkp_int));



end

