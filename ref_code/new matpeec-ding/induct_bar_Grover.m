function Ls = induct_bar_Grover(wid, hig, len, mur)
%  Function:       induct_bar_Grover
%  Description:    Calculate DC inductance of the rectangle conductor using
%                  Grover's formula.
%
%  Calls:
%
%  Input:          wid   --  width of conductors (N*1) (m)
%                  hig   --  thick of conductors (N*1) (m)
%                  len   --  length of conductors (N*1)
%                  mur   --  Relative Permeability of the conductor
%  Output:         Ls    --  Ls vector (have multiplied length)
%  Others:
%  Author:         Chen Hongcai
%  Email :         hc.chen@live.com
%  Date:           2015-02-16

mu0 = 4*pi*1e-7;

%ind = find(len > 10000, 1);
%if ~isempty(ind)
%    error('Rectangle Conductor is TOO Long. May Cause Instability !');
%end

% include self internal inductance
if nargin < 4
    Ls = mu0/4/pi*2*len.*( log(2*len./(wid+hig)) + 0.5 + 0.2235*(wid+hig)./len );
else
    mur = max(mur,1);
    % ln(D) = ln(W+T)-3/2
    %Ls = mu0/4/pi*2*len.*( log(2*len./(wid+hig)) - 1 + 0.2235*(wid+hig)./len + 3/2*mur );
    
    % ln(D) = ln((W+T)/2) + ln(2) - 3/2
    Ls = mu0/4/pi*2*len.*( log(4*len./(wid+hig)) - 1 + 0.2235*(wid+hig)./len + (3/2-log(2)).*mur );
end


end

