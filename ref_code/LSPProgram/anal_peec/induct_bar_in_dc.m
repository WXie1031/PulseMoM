function Lin_dc = induct_bar_in_dc(w, t, len)
%  Function:       induct_rec_ext
%  Description:    Calculate internal DC inductance of the rectangle 
%                  conductor using formula in ""DC Internal Inductance for 
%                  a Conductor of Rectangular Cross Section"
%
%  Calls:          
%
%  Input:          w    --  width of conductors (N*1) (m)
%                  t    --  thick of conductors (N*1) (m)
%                  len  --  length of conductors (N*1)
%  Output:         Lin_dc  --  Lin_dc vector (have multiplied length)
%  Others:
%  Author:         Chen Hongcai
%  Email :         hc.chen@live.com
%  Date:           2015-07-16

mu0 = 4*pi*1e-7;

% 2. from - "DC Internal Inductance for a Conductor of Rectangular Cross Section"
% numerical integration is needed to obtain the result.

% internal inductance
f = @(x,y) ( (w+2*x)./4.*log( ((w/2+x).^2+(t/2-y).^2)./((w/2+x).^2+(t/2+y).^2) ) ... 
    + (w-2*x)./4.*log( ((w/2-x).^2+(t/2-y).^2)./((w/2-x).^2+(t/2+y).^2) ) ...
    + (t/2-y).*( atan((w-2*x)./(t-2*y)) + atan((w+2*x)./(t-2*y)) ) ...
    - (t/2+y).*( atan((w-2*x)./(t+2*y)) + atan((w+2*x)./(t+2*y)) ) ).^2 ... % W1
    ...
    + ( (t+2*y)./4.*log( ((w/2-x).^2+(t/2+y).^2)./((w/2+x).^2+(t/2+y).^2) ) ... 
    + (t-2*y)./4.*log( ((w/2-x).^2+(t/2-y).^2)./((w/2+x).^2+(t/2-y).^2) ) ...
    + (w/2-x).*( atan((t-2*y)./(w-2*x)) + atan((t+2*y)./(w-2*x)) ) ...
    - (w/2+x).*( atan((t-2*y)./(w+2*x)) + atan((t+2*y)./(w+2*x)) ) ).^2;  % W2

% modification factor for length of inductance
% cof = (1 + log(len)./(1.2-log(w+t)));


Lin_dc = mu0./(2*pi*w.*t).^2 .* len .* integral2(f,-w/2,w/2,-t/2,t/2);


end