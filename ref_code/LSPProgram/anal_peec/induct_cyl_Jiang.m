function Ls = induct_cyl_Jiang(r, len)
%  Function:       induct_bar_Grover
%  Description:    Calculate DC inductance of the cylinder conductor using
%                  Liang's formula. The thickness of clyinder is 0.
%                  From paper 'Skin-Effect loss modes for time- and 
%                  frequency-domain PEEC solver'
%  Calls:          
%
%  Input:          r       --  radius of conductors (N*1) (m)
%                  len     --  length of conductors (N*1)
%  Output:         Ls      --  Ls vector (have multiplied length)
%  Others:
%  Author:         Chen Hongcai
%  Email :         hc.chen@live.com
%  Date:           2015-12-16


mu0 = 4*pi*1e-7;

k = 2*r/len;
k2 = k.*k;


Ls = mu0*len/4.*( (k2/480+k2.*k2/1280+1/3600)*pi.^3 + (1/18-k2/24)*pi ...
    + (-2*log(len)+6*log(2)+2+2*log(r)-4*log(k*pi))/pi + 8*r./len/pi.^2 );



end


