function Txmm = dgf_kernal_TVmm(zf, zm1,zm2,dm, GRx_ij, kzm)
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


% Nf = size(zf,1);
Nint = size(GRx_ij,2);


Txmm = (1+GRx_ij.*exp(-2*1j*kzm.*repmat(abs(zf-zm1),1,Nint))) ...
    .* exp(-1j*kzm.*repmat(abs(zm2-zf),1,Nint))  ...
    ./ (1+GRx_ij.*exp(-2*1j*kzm.*repmat(dm,1,Nint)));

end

