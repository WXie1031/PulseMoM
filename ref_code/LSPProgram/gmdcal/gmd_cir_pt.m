function out = gmd_cir_pt(P0,rout, Qpt, ver)
%  Function:       induct_cir_ac
%  Description:    Calculate GMD of point to circular.
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

if nargin < 4
    ver = 0;
end

R = sqrt( (P0(:,1)-Qpt(:,1)).^2+(P0(:,2)-Qpt(:,2)).^2 );


if ver==1
    %out = ln_gmd;
    
    if R<rout
        out = log(rout)-1/2;
    else
        out = log(R);
    end
else
    %out = exp(ln_gmd);
    if R<rout
        out = 0.6065*rout;
    else
        out = R;
    end
end


end



