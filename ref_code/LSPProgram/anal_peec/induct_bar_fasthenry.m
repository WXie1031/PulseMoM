%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% L_SELF_BAR_FASTHENRY calculate the self inductance of the rectangle conductor
%                 (FastHenry's Formula)
% %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% INPUT %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% wid           [N*1] width of the tape
% hig           [N*1] hight of the tape 
% len           [N*1] length of the tape
% %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% OUTPUT %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Ls            self inductance
% %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% revised by Chen Hongcai 2014.2
function Ls = induct_bar_fasthenry(wid, hig, len)

u0 = 4*pi*1e-7;

w = wid./len;
t = hig./len;
    
w2 = w.*w;
t2 = t.*t;
r = sqrt(w2+t2);
aw = sqrt(w2+1);
at = sqrt(t2+1);
ar = sqrt(w2+t2+1);
    
    
wxt = w.*t;
wxt2 = w.*t2;
txw2 = t.*w2;
    
w_aw = w+aw;
t_at = t+at;
r_ar = r+ar;
at_ar = at+ar;
aw_ar = aw+ar;
    
wlr = w./r;
tlr = t./r;
wlt = w./t;
tlw = 1./wlt;
w2lt2 = wlt.*wlt;
t2lw2 = tlw.*tlw;
wlat = w./at;
tlaw = t./aw;

Ls = 2*u0/pi.*len.*(...
    1/4.*(1./w.*asinh(wlat) + 1./t.*asinh(tlaw) + asinh(1./r))...
    + 1/24.*(tlw.*t.*asinh(wlt./(at.*r_ar)) + wlt.*w.*asinh(tlw./(aw.*r_ar))...
    + t2lw2.*asinh(wlt.*wlr./at_ar) + w2lt2.*asinh(tlw.*tlr./aw_ar)...
    + 1./wxt2.*asinh(wxt2./(at.*aw_ar)) + 1./txw2.*asinh(txw2./(aw.*at_ar)))...
    - 1/6.*( 1./wxt.*atan(wxt./ar) + tlw.*atan(wlt./ar) + wlt.*atan(tlw./ar) )...
    - 1/60.*( (r_ar+t_at).*t2./(r_ar.*(r+t).*t_at.*at_ar )...
    + (r_ar+w_aw).*w2./(r_ar.*(r+w).*w_aw.*aw_ar)...
    + (aw_ar+at+1)./(aw_ar.*(aw+1).*(at+1).*at_ar) )...
    - 1/20.*(1./r_ar + 1./aw_ar + 1./at_ar)...
    );

end

