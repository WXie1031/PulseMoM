function Ls = induct_gmd_long(gmd, len)
%  Function:       induct_cir_ac
%  Description:    Calculate DC inductance of the rectangle conductor using
%                  Grover's formula.
%
%  Calls:          
%
%  Input:          gmd  --  geomatric mean distance of conductors (N*1) (m)
%                  len       --  length of conductors (N*1)
%  Output:         Ls    --  Ls vector (have multiplied length)
%  Others:
%  Author:         Chen Hongcai
%  Email :         hc.chen@live.com
%  Date:           2015-09-16


% include self internal inductance
Ls = 2e-7*len.*( log(2*len./gmd) - 1 + gmd./len );

end