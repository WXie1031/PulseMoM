function Lext = induct_cir_dc(re, len, mur)
%  Function:       induct_cir_dc
%  Description:    Calculate whole indcutance for round condcutors.
%                  It is based on the GMD
%  Calls:
%  Input:          len  --  length of circular line
%                  re   --  equivalent radius of circular line
%                  mur  --  Relative Permeability of the conductor
%
%  Output:         Ls  --  inductance result
%  Others:         multi lines are supported
%  Author:         Chen Hongcai
%  Email :         hc.chen@live.com
%  Date:           2016-03-14

%mu0 = 4*pi*1e-7;
if nargin < 3
    mur = 1;
else
    mur = max(mur,1);
end

mur = 0;

% using the exact formulas for calculation
Lext = 2e-7*len.*( log(len./re+sqrt((len./re).^2+1)) ...
    - sqrt(1+(re./len).^2) + re./len + mur/4 );


end

