function [Rgird,Lgrid, Rgself,Lgself] = main_ground_grid(pt_start,pt_end, dv, re, len, ...
    shape, dim1,dim2, Rdc, sig, mur, frq, gnd_type, sig_soil, epr_soil)
%  Function:       main_ground_grid
%  Description:    Calculate R and L matrix of grid conductors using
%                  filament model.
%
%  Calls:          para_grid_fila
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

%% 1. mesh the ground grid 
p_flag = 0;
dl = 5;
dl_type = 1;

[pt_sta_m, pt_end_m, dv_m, re_m, len_m, shape_m,dim1_m,dim2_m, Rdc_m, sig_m, mur_m] ...
    = mesh_line_3d(pt_start, pt_end, dv, re, len, shape, dim1, dim2, ...
    Rdc, sig, mur, dl, dl_type, p_flag);

%% 2. add new nodes and branches for new segments after meshing 


%% 3. calculate parameters
% mutual R,L matrix
[Rgird, Lgrid] = para_grid_fila(pt_sta_m, pt_end_m, dv_m, re_m, len_m);

% Zc conductor R and L
[Rgself, Lgself] = para_self_multi_frq(shape_m, dim1_m, dim2_m, ...
    len_m, Rdc_m, sig_m, mur_m*ones(1,length(frq)), frq);

% Zg 

if gnd_type == 1
    [Rg,Lg, Rgself,Lgself] = grid_cmplx_plane(pt_sta_m,pt_end_m, dv_m, re_m, len_m,...
    sig_soil, epr_soil, frq);
    
end

Rgird = Rgird+Rg;
Lgrid = Lgrid+Lg;


end


