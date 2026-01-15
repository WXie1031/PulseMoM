function Ls = induct_cyl_hf_Rayleigh(re, len, mur)
%  Function:       induct_cyl_hf_Rayleigh
%  Description:    Calculate external indcutance for cylinder condcutors
%                  The limit of tht filament formula in high frequency.
%  Calls:
%  Input:          len  --  length of circular line
%                  re   --  equivalent radius of circular line
%
%  Output:         Ls  --  inductance result
%  Others:         multi lines are supported
%  Author:         Chen Hongcai
%  Email :         hc.chen@live.com
%  Date:           2015-07-07
mu0 = 4*pi*1e-7;
if nargin < 3
    mur = 1;
else
    mur = max(1,mur);
end

mu = mu0*mur;

Rdc = 
% using the exact formulas for calculation
Ls = 2e-7*len.*( log(2*len./re) -1 + 0.5*sqrt(mur*Rdc./(2*p*len)));

end

