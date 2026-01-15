function int = dgf_kernal_1(zf,zs, zbdy,nn, Dx, GRx_ij,GRx_ji, kz)
%  Function:       dgf_kernal_1
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
dz0 = abs(zs-zf);

% dz1 = 2*zbdy(nn) - (zf+zs);
% dz2 = (zf+zs) - 2*zbdy(nn-1);
% dz3 = 2*zbdy(nn-1) + (zf-zs);
% dz4 = 2*zbdy(nn-1) - (zf-zs);

% Ae0 = exp(-1j*kz.*dz0);
% Ae1 = GRx_ij./Dx .* exp(-1j*kz.*dz1);
% Ae2 = GRx_ji./Dx .* exp(-1j*kz.*dz2);
% Ae3 = GRx_ij.*GRx_ji./Dx .* exp(-1j*kz.*dz3);
% Ae4 = GRx_ij.*GRx_ji./Dx .* exp(-1j*kz.*dz4);
% 
% int = 1/2*(Ae0+Ae1+Ae2+Ae3+Ae4);


dz1 = zbdy(nn)-max(zf,zs);
dz2 = min(zf,zs) - zbdy(nn-1);

int = 0.5*exp(-1j*kz.*dz0)./Dx .* (1+GRx_ij.*exp(-2*1j.*kz.*dz1)) ...
    .* (1+GRx_ji.*exp(-2*1j.*kz.*dz2));


end

