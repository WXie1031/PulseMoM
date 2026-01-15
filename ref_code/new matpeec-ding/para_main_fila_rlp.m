function [Rmtx, Lmtx, Cmtx,nod_P] = para_main_fila_rlp(pt_start, pt_end, dv, re, len, ...
    nod_start,nod_end, p_flag)
%  Function:       para_main_fila_rlp
%  Description:    Calculate L and P of all conductors using
%                  filament model. Cable group which calculated using
%                  meshing method with update the matrix outside this
%                  function.
%
%  Calls:          cal_L_fila
%                  cal_P_fila
%
%  Input:          pt_start  --  start point of conductors (N*3) (m)
%                  pt_end    --  end point of conductors (N*3) (m)
%                  dv        --  direction vector of conductors (N*3)
%                  re        --  equivalent radius (N*1)
%                  Rin_pul   --  resistance of conductors (N*1) (ohm/m)
%                  Lin_pul   --  internal L of conductors (N*1) (H/m)
%                  len       --  length of conductors (N*1)
%                  Ndat_3D   --  num. of conductors (N*1)
%  Output:         Rmtx --  R matrix
%                  Lmtx --  L matrix
%  Others:
%  Author:         Chen Hongcai
%  Email :         hc.chen@live.com
%  Date:           2014-12-13

p_flag=1;




Nc = size(pt_start,1);
Rmtx = zeros(Nc, Nc);
Lmtx = zeros(Nc, Nc);

if p_flag>0    
    pt_mid = (pt_start+pt_end)/2;
    p_sta_P= [pt_start;pt_mid];
    p_end_P= [pt_mid;pt_end];
    dv_P   = [dv;dv];
    len_P  = [len;len]/2;
    re_P   = [re;re];
    [nod_a1 nod_a2]=size(nod_start);
    [nod_b1 nod_b2]=size(nod_end);
%     nod_P=zeros(nod_a1+nod_b1,max(nod_a2,nod_b2));
    for ia=1:nod_a1
        nod_P(ia,1:nod_a2)=nod_start(ia,1:nod_a2);
    end
    for ia=nod_a1+(1:nod_b1)
        nod_P(ia,1:nod_b2)=nod_end(ia-nod_b1,1:nod_b2);
    end
%     nod_P = [nod_start;nod_end];
    
    Nn = size(p_sta_P,1);
    Pmtx = zeros(Nn, Nn);
else
    Cmtx = [];
    nod_P= [];
end

%% use DC model to calculate resistance
%Rmtx = sparse(diag());

%% use filament model to calculate mutual inductance
for ik = 1:Nc

    % calculate inductance using filament model
    Lmtx(1:ik,ik) = cal_L_fila ...
        (pt_start(ik,1:3), pt_end(ik,1:3), dv(ik,1:3), len(ik), re(ik), ...
        pt_start(1:ik,1:3), pt_end(1:ik,1:3), dv(1:ik,1:3), len(1:ik), re(1:ik));

end

% for ik = 1:Nc
% 
%     % calculate inductance using filament model
%     Lmtx(1:Nc,ik) = cal_L_fila ...
%         (pt_start(ik,1:3), pt_end(ik,1:3), dv(ik,1:3), len(ik), re(ik), ...
%         pt_start(1:Nc,1:3), pt_end(1:Nc,1:3), dv(1:Nc,1:3), len(1:Nc), re(1:Nc));
% 
% end




Ls = diag(Lmtx);
Lmtx = sparse(Lmtx+Lmtx'-diag(Ls));

%% use filament model to calculate mutual inductance
if p_flag>0
    for ik = 1:Nn
        
        Pmtx(1:ik,ik) = cal_P_fila ...
            (p_sta_P(ik,1:3), p_end_P(ik,1:3), dv_P(ik,1:3), len_P(ik), re_P(ik), ...
            p_sta_P(1:ik,1:3), p_end_P(1:ik,1:3), dv_P(1:ik,1:3), len_P(1:ik), re_P(1:ik));
    end
         Pmtx=abs(Pmtx);
    Ps = diag(Pmtx);

    Pmtx = sparse(Pmtx+Pmtx'-diag(Ps));

   
end
Lmtx=full(Lmtx);
Pmtx=full(Pmtx);


u0=4*pi*1e-7;
ep0=8.85e-12;
for ia=1:Nc
    Lmtx(ia,ia)=u0/2/pi*len(ia)*(log(2*len(ia)/re(ia))-1);
    Ls0(ia)=u0/2/pi*len(ia)*log(2*len(ia)/re(ia)-1);
   
end

for ia=1:Nn
    Ps0(ia)=1./(2*pi*ep0)./(len_P(ia)*len_P(ia))*log(2*len_P(ia)/re_P(ia)-1);
    Pmtx(ia,ia)=1./(2*pi*ep0)./(len_P(ia)*len_P(ia))*log(2*len_P(ia)/re_P(ia)-1);
%     Pmtx(ia,ia)=1./(2*pi*ep0)./(len_P(ia))*(log(2*len_P(ia)/re_P(ia))-1);
end

    Cmtx = inv(Pmtx);
Cmtx=full(Cmtx);
% end


