function Ls = induct_agi_gmd(wid, hig, len)
%  Function:       induct_agi_gmd
%  Description:    Calculate DC inductance of the angle conductor using
%                  GMD formula.
%
%  Calls:          
%
%  Input:          wid       --  width of conductors (N*1) (m)
%                  hig       --  thick of conductors (N*1) (m)
%                  len       --  length of conductors (N*1)
%  Output:         Ls    --  Ls vector (have multiplied length)
%  Others:
%  Author:         Chen Hongcai
%  Email :         hc.chen@live.com
%  Date:           2015-02-16


%ind = find(len > 10000, 1);
%if ~isempty(ind)
%    error('Rectangle Conductor is TOO Long. May Cause Instability !');
%end

% include self internal inductance

re = gmd_self_agi(wid, hig);

Ls = 2e-7*len.*( log(len./re+sqrt((len./re).^2+1)) ...
    - sqrt(1+(re./len).^2) + re./len );

end