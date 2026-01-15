function [NVw1, NVw2, NVh1, NVh2, NVs, w1, w2, h1, h2] = ...
    mesh2d_box_mag_sub( wo, ho, s_dep )
%  Function:       mesh2D_box_sub
%  Description:    Sub function of mesh2D_box. The centre of the box is
%                  diged out
%  Calls:          
%  Input:          wo   --  width of the conductor
%                  ho   --  higth of the conductor
%                  s_dep -  skin depth
%  Output:         Nw1  --  (vector)the number of segments of the wide
%                  Nw2  --   
%                  Nh1  --  (vector)the number of segments of the height
%                  Nh2  --
%                  NVs  --  (vector)the number of segments
%                  w1   --  the width of the segments
%                  w2   --  
%                  h1   --  the height of the segments
%                  h2   --
%  Others:         
%  Author:         Chen Hongcai    
%  Email :         hc.chen@live.com      
%  Date:           2013-11-13
%  History:         


% used in h2 and w2 deciding the interval length
gama = 1.1;
%cof = REC_dat.w./REC_dat.h;
Nmin = 3;

Nc = int32(size(wo,1));
NVs = zeros(Nc,3);

%% 1. the table of 10% decrease depth according to skin depth
dt_tab = s_dep*[0.10536 0.22314 0.35667 0.51083 0.69315 0.91629 1.2040 ...
    1.6094 2.3026 3.2026 4.5026 6.3026 8.7026 11.803 15.703 20.503 ];

Ntab = size(dt_tab,2);
NVw1 = zeros(1,Nc);
NVw2 = zeros(1,Nc);
NVh1 = zeros(1,Nc);
NVh2 = zeros(1,Nc);
w1 = zeros(Nc,Ntab);
h1 = zeros(Nc,Ntab);

t_tab = cumsum(dt_tab,2);

cal_tw = min(wo/2,max(t_tab,[],2));    % exact calculation higth
cal_th = min(ho/2,max(t_tab,[],2));    % exact calculation width


%% 2. h1 h2
% h1
Nhtmp = 0;
for ik = 1:Nc

    for ig = 1:Ntab
        if cal_th(ik) < t_tab(ik,ig) 
            Nhtmp = ig-1;
            break;
        else
            Nhtmp = ig;
        end
    end
    h1(ik,1:Nhtmp) = dt_tab(ik,1:Nhtmp);
    NVh1(ik) = Nhtmp;

end

for ik = 1:Nc
    if NVh1(ik) < Nmin
        NVh1(ik) = Nmin;
        h1(ik,1:NVh1(ik)) = cal_th(ik)/NVh1(ik);
    end
end

% h2
for ik = 1:Nc
    dt = h1(ik,NVh1(ik));
    cnt = 0;
    dh2t = 0;
    while 1
        dh2t = dh2t + gama^cnt*dt;
        dh = ho(ik)/2 - sum(h1(ik,:)) - dh2t;
        cnt = cnt+1;
        if dh <= 0
            break
        end
    end
    NVh2(ik) = cnt;
end

h2 = zeros(Nc,max(NVh2));
for ik = 1:Nc
    dt = h1(ik,NVh1(ik));
    if NVh2(ik) > 1
        h2(ik,1:NVh2(ik)-1) = gama.^(1:NVh2(ik)-1)*dt;
    end
    tend = ho(ik)/2 - sum(h1(ik,:)) - sum(h2(ik,:));
    h2(ik,NVh2(ik)) = tend;
    
    if NVh2(ik) >= 1 && h2(ik,NVh2(ik)) < dt/2
        NVh2(ik) = NVh2(ik)-1;
        if NVh2(ik) == 0 
            h1(ik,NVh1(ik)) = h1(ik,NVh1(ik))+h2(ik,NVh2(ik)+1);
        else
            h2(ik,NVh2(ik)) = h2(ik,NVh2(ik))+h2(ik,NVh2(ik)+1);
        end
    end
    
end


%% 3. w1 w2
% w1
for ik = 1:Nc

    for ig = 1:Ntab
        if cal_tw(ik) < t_tab(ik,ig)
            Nhtmp = ig-1;
            break;
        else
            Nhtmp = ig;
        end
    end
    w1(ik,1:Nhtmp) = dt_tab(ik,1:Nhtmp);
    NVw1(ik) = Nhtmp;

end

for ik = 1:Nc
    if NVw1(ik) < Nmin
        NVw1(ik) = Nmin;
        w1(ik,1:NVw1(ik)) = cal_tw(ik)/NVw1(ik);
    end
end

% w2
for ik = 1:Nc
    dt = w1(ik,NVw1(ik));
    cnt = 0;
    dw2t = 0;
    while 1
        dw2t = dw2t + gama^cnt*dt;
        dw = wo(ik)/2 - sum(w1(ik,:)) - dw2t;
        cnt = cnt+1;
        if dw <= 0
            break
        end
    end
    NVw2(ik) = cnt;
end

w2 = zeros(Nc,max(NVw2));
for ik = 1:Nc
    dt = w1(ik,NVw1(ik));
    
    if NVw2(ik) > 1
        w2(ik,1:NVw2(ik)-1) = gama.^(1:NVw2(ik)-1)*dt;
    end
    tend = wo(ik)/2 - sum(w1(ik,:)) - sum(w2(ik,:));
    w2(ik,NVw2(ik)) = tend;
    
    if NVw2(ik) >= 1 && w2(ik,NVw2(ik)) < dt/2
        NVw2(ik) = NVw2(ik)-1;
        if NVw2(ik) == 0
            w1(ik,NVw1(ik)) = w1(ik,NVw1(ik))+w2(ik,NVw2(ik)+1);
        else
            w2(ik,NVw2(ik)) = w2(ik,NVw2(ik))+w2(ik,NVw2(ik)+1);
        end
    end
    
end

%% 4. count the number of segments
for ik = 1:Nc
    NVs(ik,1) = NVw1(ik)*NVh1(ik);
    NVs(ik,2) = NVw2(ik)*NVh1(ik);
    NVs(ik,3) = NVw1(ik)*NVh2(ik);
end


end
