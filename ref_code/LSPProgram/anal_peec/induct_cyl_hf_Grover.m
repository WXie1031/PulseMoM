function Lext = induct_cyl_hf_Grover(re, len)
%  Function:       int_line_p
%  Description:    Calculate external indcutance for round condcutors
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


% using the exact formulas for calculation
Lext = 2e-7*len.*( log(2*len./re) -1 );

end

