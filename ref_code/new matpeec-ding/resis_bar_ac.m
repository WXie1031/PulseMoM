function Rskin = resis_bar_ac(wid, hig, R_pul, sig, mur, len, frq)
%  Function:       resis_bar_ac
%  Description:    Calculate AC resistance of the rectangle conductor using
%                  fitting method.
%
%  Calls:          
%
%  Input:          wid    --  width of conductors (N*1) (m)
%                  hig    --  thick of conductors (N*1) (m)
%                  Rdc    --  DC resistancee of conductors
%                  sig    --  conductivity of conductors (N*1) (S/m)
%                  len    --  length of conductors (N*1)
%                  f0     --  frequency (1*1)
%  Output:         Rrec   --  Rrec vector (have multiplied length)
%  Others:
%  Author:         Chen Hongcai
%  Email :         hc.chen@live.com
%  Date:           2015-06-16
%  History:        add correct factor to get more accurate fitted result
%                  2015-06-25

mu0 = 4*pi*1e-7;

Nc = length(wid);

%mur=1;
s_dep = 1./sqrt(pi*frq.*sig.*mu0.*mur);

hw_ratio = max(hig./wid, wid./hig);

xw0 = zeros(Nc,1);
for k = 1:Nc
    if hw_ratio(k) >= 1 && hw_ratio(k) < 4
        xw0(k) = 4.5-1.4*sqrt(hw_ratio(k));
    elseif hw_ratio(k) >=4 && hw_ratio(k) < 9
        xw0(k) = 1.7;
    elseif hw_ratio(k) >= 9 && hw_ratio(k) <=16
        xw0(k) = -0.1+0.6*sqrt(hw_ratio(k));
    else
        xw0(k) = 2.3;
    end
end


xw = sqrt(2/pi).*sqrt(wid.*hig)./s_dep;
%xw = 3.5.*sqrt(wid.*hig)./s_dep;

k01 = 0.43093*xw./(1+0.041*hw_ratio.^1.19) + (1.1147+1.2868*xw)./(1.2296+1.287*xw.^3)...
    + 0.0035*(hw_ratio-1).^1.8;

k02 = 1 + 0.0122*xw.^(3+0.01*xw.^2);

k0 = zeros(Nc,1);
for k = 1 : Nc
    if xw(k) >= xw0(k)
        k0(k) = k01(k);
    else
        k0(k) = k02(k);
    end
end


Rskin = k0.*R_pul.*len ;


end
    
    
