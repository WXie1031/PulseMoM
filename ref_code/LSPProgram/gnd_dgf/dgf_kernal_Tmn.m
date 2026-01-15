function Txmn = dgf_kernal_Tmn(id_s,id_f, d_lyr, GRx_ij, kz, mode)
%  Function:       dgf_kernal_Tmn
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


Nf = size(id_f,1);
Nint = size(GRx_ij,2);
Txmn = ones(Nf,Nint);

if nargin < 6 || isempty(mode)
    mode = 'V';
end

if strcmpi(mode,'I')
    s = -1;
else
    s = 1;
end

idf_uni = unique(id_f);
Nf_uni = length(idf_uni);

Txmn_tmp = ones(Nf_uni,Nint);
for ig = 1:Nf_uni
    % calculate the Txmn for every different id_f. each id_f need calculate
    % only once.
    for ik = id_s+1:idf_uni(ig)-1
%         Txmn(id_mn,1:Nint) = Txmn(id_mn,1:Nint) .* (1+repmat(GRx_ij(ik,1:Nint),Nid_mn,1)) ...
%             .*exp(-1j*repmat(kz(ik,1:Nint),Nid_mn,1).*(d_lyr(ik))) ...
%             ./ (1+repmat(GRx_ij(ik,1:Nint),Nid_mn,1).*exp(-2*1j*repmat(kz(ik,1:Nint),Nid_mn,1).*d_lyr(ik)));
        Txmn_tmp(ig,1:Nint) = Txmn_tmp(ig,1:Nint) .* (1+s*GRx_ij(ik,1:Nint)) ...
            .*exp(-1j*kz(ik,1:Nint).*d_lyr(ik)) ...
            ./ (1+s*GRx_ij(ik,1:Nint).*exp(-2*1j*kz(ik,1:Nint).*d_lyr(ik)));
    end
    
    % duplicate the calculated value to the matrix
    id_mn = id_f==idf_uni(ig);
    Nid_mn = sum(id_mn);
    Txmn(id_mn,1:Nint) = repmat(Txmn_tmp(ig,1:Nint) ,Nid_mn,1) ;
end



end

