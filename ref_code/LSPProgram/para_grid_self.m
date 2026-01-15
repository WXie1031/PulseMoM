function [Rgrid, Lgrid] = para_grid_self(Rgrid, Lgrid, type_flag, dim1, dim2, ...
    len, R_pul, Lin_pul, mur, sig)
%  Function:       para_grid_self
%  Description:    Calculate self R and L and update R and L matrix of all
%                  conductors using filament model. Cable group which
%                  calculated using meshing method with update the matrix
%                  outside this function.
%                  For filament model, R is the internal resistance of
%                  specified frequency.
%
%  Calls:          cal_L_fila
%
%  Input:          re        --  equivalent radius (N*1)
%                  Rin_pul   --  resistance of conductors (N*1) (ohm/m)
%                  Lin_pul   --  internal L of conductors (N*1) (H/m)
%                  len       --  length of conductors (N*1)
%                  Ndat_3D   --  num. of conductors (N*1)
%  Output:         Rmtx --  R matrix
%                  Lmtx --  L matrix
%  Others:
%  Author:         Chen Hongcai
%  Email :         hc.chen@live.com
%  Date:           2015-111-26

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

Nc = size(dim1,1);
freqR = 500;
freqL = 10e3;

%% 1. calculate the self R and L of all conductors.
for ik = 1:Nc
    
    %         idrec = shape == 1002;
    %         idagi = shape == 1003;
    %         idcir = ~(idrec|idagi);
    
    % L calcualte at 10kHz, R calculate at 500Hz
    switch type_flag(ik)
        case 1001
            
             Rin = zin_cir_ac(dim1(ik), dim2(ik), ...
                sig(ik), mur(ik), len(ik), freqR);
             Rgrid(ik,ik) = max(Rin, R_pul(ik)*len(ik));
             
             if Lin_pul(ik)==0
                f0 = 10e3;
                [~, Lin] = resis_induct_cir_ac(dim1(ik), dim2(ik),...
                    sig(ik), mur(ik), len(ik), f0);
            else
                Lin = Lin_pul(ik).*len(ik);
            end
            
            Lgrid(ik,ik) = induct_cir_ext(dim1(ik),len(ik)) + Lin;
            
        case 1002
            
            Rin = resis_bar_ac(dim1(ik), dim2(ik), ...
                R_pul(ik), sig(ik), mur(ik), len(ik), freqR);
            Rgrid(ik,ik) = max(Rin, R_pul(ik)*len(ik));
            
            if Lin_pul(ik)==0
                f0 = 10e3;
                Lgrid(ik,ik) = induct_bar_ac(dim1(ik), dim2(ik), ...
                    sig(ik), mur(ik), len(ik), f0);
            else
                f0 = 5e6;
                Lgrid(ik,ik) = induct_bar_ac(dim1(ik), dim2(ik), ...
                    sig(ik), 1, len(ik), f0) + Lin_pul(ik).*len(ik);
            end
            
        case 1003
            
            Rin = resis_agi_ac(dim1(ik), dim2(ik), ...
                R_pul(ik), sig(ik), mur(ik), len(ik), freqR);
            Rgrid(ik,ik) = max(Rin, R_pul(ik)*len(ik));
            
            if Lin_pul(ik)==0
                f0 = 10e3;
                Lgrid(ik,ik) = induct_agi_ac(dim1(ik), dim2(ik), ...
                    sig(ik), mur(ik), len(ik), f0);
            else
                f0 = 5e6;
                Lgrid(ik,ik) = induct_agi_ac(dim1(ik), dim2(ik), ...
                    sig(ik), 1, len(ik), f0) + Lin_pul(ik).*len(ik);
            end
            
        otherwise
            
            Rin = zin_cir_ac(dim1(ik), dim2(ik), ...
                sig(ik), mur(ik), len(ik), freqR);
            
            Rgrid(ik,ik) = max(Rin, R_pul(ik)*len(ik));
            
            if Lin_pul(ik)==0
                f0 = 10e3;
                [~, Lin] = resis_induct_cir_ac(dim1(ik), dim2(ik),...
                    sig(ik), mur(ik), len(ik), f0);
            else
                Lin = Lin_pul(ik).*len(ik);
            end
            
            Lgrid(ik,ik) = induct_cir_ext(dim1(ik),len(ik)) + Lin;
            
    end
    
    
%     Lgrid(ik,1:Nc) = min(Lgrid(ik,1:Nc),0.9999*Lgrid(ik,ik));
%     Lgrid(1:Nc,ik) = min(Lgrid(1:Nc,ik),0.9999*Lgrid(ik,ik));
    
end


end


