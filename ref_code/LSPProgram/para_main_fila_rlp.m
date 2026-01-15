function [Rmtx,Lmtx,Pmtx] = para_main_fila_rlp(p_sta_L,p_end_L,dv_L,re_L,l_L, ...
    p_sta_P,p_end_P,dv_P,re_P,l_P,p_flag)
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
%                  Pmtx --  P matrix
%  Others:
%  Author:         Chen Hongcai
%  Email :         hc.chen@live.com
%  Date:           2016-09-13


%% use filament model to calculate mutual inductance
Nc = size(p_sta_L,1);
Rmtx = zeros(Nc, Nc);
Lmtx = zeros(Nc, Nc);

for ik = 1:Nc

    Lmtx(1:ik,ik) = cal_L_fila ...
        (p_sta_L(ik,1:3), p_end_L(ik,1:3), dv_L(ik,1:3), l_L(ik), re_L(ik), ...
        p_sta_L(1:ik,1:3), p_end_L(1:ik,1:3), dv_L(1:ik,1:3), l_L(1:ik), re_L(1:ik));

end

Ls = diag(Lmtx);
Lmtx = sparse(Lmtx+Lmtx'-diag(Ls));


%% use filament model to calculate mutual inductance
if p_flag>0
    Np = size(p_sta_P,1);
    Pmtx = zeros(Np, Np);
    
    for ik = 1:Np 
        
        Pmtx(1:ik,ik) = cal_P_fila ...
            (p_sta_P(ik,1:3), p_end_P(ik,1:3), dv_P(ik,1:3), l_P(ik), re_P(ik), ...
            p_sta_P(1:ik,1:3), p_end_P(1:ik,1:3), dv_P(1:ik,1:3), l_P(1:ik), re_P(1:ik));
    
    end
    
    Ps = diag(Pmtx);
    Pmtx = sparse(Pmtx+Pmtx'-diag(Ps));
else
    Pmtx = [];
end


end


