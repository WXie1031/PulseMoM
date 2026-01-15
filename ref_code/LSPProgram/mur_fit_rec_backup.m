function mur = mur_fit_rec_backup(wid, hig, R_pul, sig, Rsurge, fmur)
%  Function:       mur_fit
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


Rtmp = resis_bar_ac(wid, hig, R_pul, sig, 1, 1, fmur);
mur = (Rsurge/Rtmp).^2;


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

k0 =Rsurge./R_pul;

[xw1,~] = fsolve(@(xw)(k0 - (0.43093*xw./(1+0.041*hw_ratio.^1.19) + (1.1147+1.2868*xw)./(1.2296+1.287*xw.^3) + 0.0035*(hw_ratio-1).^1.8)),0.1);
[xw2,~] = fsolve(@(xw)(log(k0-1) - log(0.0122) - (3+0.01*xw.^2).*log(xw)),0.1);

for k = 1 : Nc
    if xw1(k) >= xw0(k)
        s_dep = sqrt(2/pi).*sqrt(wid.*hig)./xw1;
    elseif xw2(k) >= xw0(k)
        s_dep = sqrt(2/pi).*sqrt(wid.*hig)./xw2;
    end
end


mur = (1./s_dep).^2 ./(pi*fmur.*sig.*mu0);


end
    
    
