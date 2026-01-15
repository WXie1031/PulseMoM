function Lext = induct_cir_ext(re, len)
%  Function:       induct_cir_ext
%  Description:    Calculate external indcutance for round condcutors.
%                  It is the same with filament formula
%  Calls:
%  Input:          len  --  length of circular line
%                  re   --  equivalent radius of circular line
%
%  Output:         Ls  --  inductance result
%  Others:         multi lines are supported
%  Author:         Chen Hongcai
%  Email :         hc.chen@live.com
%  Date:           2015-07-07


% using the exact formulas for calculation
Lext = 2e-7*len.*( log(len./re+sqrt((len./re).^2+1)) ...
    - sqrt(1+(re./len).^2) + re./len );

end

