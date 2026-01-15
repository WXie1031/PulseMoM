function [Rgrid, Lgrid] = para_grid_fila(pt_start, pt_end, dv, re, len)
%  Function:       para_grid_fila
%  Description:    Calculate R and L matrix of grid conductors using
%                  filament model.
%
%  Calls:          cal_L_fila
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
%  Date:           2015-09-13

Nc = size(pt_start,1);
Rgrid = zeros(Nc, Nc);
Lgrid = zeros(Nc, Nc);

% DEVICE_TYPESIGN_CONDUCTOR_BCC = 1001;
% DEVICE_TYPESIGN_CONDUCTOR_BCF = 1002;
% DEVICE_TYPESIGN_CONDUCTOR_BCL = 1003;

% DEVICE_TYPESIGN_CABLE_COX     2100
% DEVICE_TYPESIGN_CABLE_SDC     2200
% DEVICE_TYPESIGN_CABLE_UDC     2300
% DEVICE_TYPESIGN_CABLE_STP     2400
% DEVICE_TYPESIGN_CABLE_UTP     2500
% DEVICE_TYPESIGN_CABLE_OTH     2700
% DEVICE_TYPESIGN_CABLE_SAC     2800
% DEVICE_TYPESIGN_CABLE_UAC     2900
% DEVICE_CABLE_RANGE   3000

%% use filament model to calculate mutual inductance
for ik = 1:Nc
    
%     Rgird(ik,ik) = Rsoil./(2*pi*len(ik)) .* (log(len(ik)./re(ik)+sqrt(1+(len(ik)./re(ik)).^2)) ...
%         + re(ik)./len(ik) - sqrt(1+(re(ik)./len(ik)).^2));
    
    % calculate inductance using filament model
    Lgrid(1:ik,ik) = cal_L_fila ...
        (pt_start(ik,1:3), pt_end(ik,1:3), dv(ik,1:3), len(ik), re(ik), ...
        pt_start(1:ik,1:3), pt_end(1:ik,1:3), dv(1:ik,1:3), len(1:ik), re(1:ik));
    
end

Ls = diag(Lgrid);
Lgrid = Lgrid+Lgrid'-diag(Ls);



end


