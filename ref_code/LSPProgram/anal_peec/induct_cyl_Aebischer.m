function Ls = induct_cyl_Aebischer(re, len)
%  Function:       induct_cyl_Aebischer
%  Description:    Calculate DC inductance of the cylinder conductor using
%                  Aebischer's formula. The thickness of clyinder is 0.
%                  From paper 'Improved Formulae for the Inductance of 
%                  Straight Wires'
%  Calls:          
%
%  Input:          r       --  radius of conductors (N*1) (m)
%                  len     --  length of conductors (N*1)
%  Output:         Ls      --  Ls vector (have multiplied length)
%  Others:
%  Author:         Chen Hongcai
%  Email :         hc.chen@live.com
%  Date:           2015-12-16


%mu0 = 4*pi*1e-7;


l2 = len.^2;
re2 = re.^2;


Ls = 2e-7*( len.*log(sqrt(l2+2*re2)+len) - len.*log(re) ...
    - sqrt(l2+2*re2) + 4/pi*re );


end


