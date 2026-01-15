function mur = mur_fit_agi(wid, hig, R_pul, sig, Rsurge, fmur)
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
%                  2015-09-02

hig = abs(hig);
wid = abs(wid);

hw_ratio = max(hig./wid, wid./hig);

Rtmp = resis_agi_ac(wid, hig, R_pul, sig, 1, 1, fmur);

mur = hw_ratio/32*(Rsurge./Rtmp).^2;

mur = max(mur,1);

end
    
    
