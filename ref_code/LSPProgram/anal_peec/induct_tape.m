function Ldc = induct_tape(W, l)
%  Function:       induct_tape
%  Description:    Calculate DC inductance of the thin tape using
%                  exact integral formula.
%
%  Calls:          
%
%  Input:          W     --  width of tapes (N*1) (m)
%                  l     --  length of conductors (N*1)
%  Output:         Ls    --  Ls vector (have multiplied length)
%  Others:
%  Author:         Chen Hongcai
%  Email :         hc.chen@live.com
%  Date:           2015-12-16


mu0 = 4*pi*1e-7;

W2 = W.*W;
l2 = l.*l;
R2 = W2+l2;
R = sqrt(R2);

Ldc = mu0/4/pi * 2./(3.*W2) .* ( 3*W2.*l.*log((l+R)./W) - R.*R2 ...
    + 3*l2.*W.*log((W+R)./l) + l.*l2 + W.*W2 ) ;

end

