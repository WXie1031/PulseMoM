function out = gmd_anl_self(ro,ri, ver)
%  Function:       gmd_anl_self
%  Description:    Calculate GMD of self annular.
%
%  Calls:          
%
%  Input:          gmd  --  geomatric mean distance of conductors (N*1) (m)
%                  len       --  length of conductors (N*1)
%  Output:         Ls    --  Ls vector (have multiplied length)
%  Others:
%  Author:         Chen Hongcai
%  Email :         hc.chen@live.com
%  Date:           2015-12-16

if nargin < 3
    ver = 0;
end

ro2 = ro.^2;
ri2 = ri.^2;
ri4 = ri2.^2;

ln_gmd = log(ro) - ri4./(ro2-ri2).^2.*log(ro./ri) ...
    + (3*ri2-ro2)./(4*(ro2-ri2));

if ver==1
    out = ln_gmd;
else
    out = exp(ln_gmd);
end


end



