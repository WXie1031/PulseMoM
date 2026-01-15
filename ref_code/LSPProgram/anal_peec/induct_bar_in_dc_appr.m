function Lin_dc = induct_bar_in_dc_appr(w, t, len)
%  Function:       induct_rec_ext
%  Description:    Calculate internal DC inductance of the rectangle 
%                  conductor using formula in "Addition to "DC Internal 
%                  Inductance for a Conductor of Rectangular Cross
%                  Section""
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

% 2. from - Addition to "DC Internal Inductance for a Conductor of 
%  Rectangular Cross Section"
%  approximation formular 

% internal inductance
% modification factor for length of inductance
Nlen = size(len,1);

if Nlen==0
    Lin_dc=[];
    return;
% elseif Nlen==1
%     if len>=0.8;
%         cof = (1 + log(len)./(1.2-log(w+t)));
%     else
%         cof = len;
%     end
% else
%     id_short = len<0.8;
%     cof = (1 + log(len)./(1.2-log(w+t)));
%     cof(id_short) = len(id_short);
end

Lin_dc = mu0/8/pi* len .* (4.1888*w.^3.*t + 51.906*w.^2.*t.^2 + 4.1888*w.*t.^3) ...
    ./(w.^4 + 16.09*w.^3.*t + 28.2*w.^2.*t.^2 + 16.09*w.*t.^3 + t.^4);

% t_w = t./w;
% Lin_dc = mu/4/pi* cof .* (0.045 + 200.23*t_w - 443.01*t_w.^2 ...
%     + 724.18*t_w.^3 - 823.7*t_w.^4 + 537.4*t_w.^5 - 146.845*t_w.^6);

end



