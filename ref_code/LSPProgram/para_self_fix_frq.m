function [Rself,Lself] = para_self_fix_frq(shape,dim1,dim2, len, R_pul,sig,mur)
%  Function:       para_self_fix_frq
%  Description:    Calculate self R and L and update R and L matrix of all
%                  conductors using filament model. Cable group which
%                  calculated using meshing method with update the matrix
%                  outside this function.
%                  For filament model, R is the internal resistance of
%                  specified frequency.
%
%  Calls:          cal_L_filament
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
%  Date:           2014-12-13


Nc = size(dim1,1);

dim1 = abs(dim1);
dim2 = abs(dim2);

Rself = zeros(Nc,1);
Lself = zeros(Nc,1);

%% 1. calculate the self R and L of all conductors.
for ik = 1:Nc

    %         idrec = shape == 1002;
    %         idagi = shape == 1003;
    %         idcir = ~(idrec|idagi);
    
    % L calcualte at 10kHz, R calculate at 500Hz
    
    mur_tmp = max(mur(ik), 1);
    
    switch shape(ik)
        case 1002
            
            f0 = 500;
            Rin = resis_bar_ac(dim1(ik), dim2(ik), ...
                R_pul(ik), sig(ik), mur_tmp, len(ik), f0);
            Rself(ik) = max(Rin, R_pul(ik)*len(ik));
            
%             if Lin_pul(ik)==0
                f0 = 5e3;
                Lself(ik) = induct_bar_ac(dim1(ik), dim2(ik), ...
                    sig(ik), mur_tmp, len(ik), f0);
%             else
%                 f0 = 5e6;
%                 Lself(ik) = induct_bar_ac(dim1(ik), dim2(ik), ...
%                     sig(ik), 1, len(ik), f0) + Lin_pul(ik).*len(ik);
%             end
            
        case 1003
            
            f0 = 500;
            Rin = resis_agi_ac(dim1(ik), dim2(ik), ...
                R_pul(ik), sig(ik), mur_tmp, len(ik), f0);
            Rself(ik) = max(Rin, R_pul(ik)*len(ik));
            
%             if Lin_pul(ik)==0
                f0 = 5e3;
                Lself(ik) = induct_agi_ac(dim1(ik), dim2(ik), ...
                    sig(ik), mur_tmp, len(ik), f0);
%             else
%                 f0 = 5e6;
%                 Lself(ik) = induct_agi_ac(dim1(ik), dim2(ik), ...
%                     sig(ik), 1, len(ik), f0) + Lin_pul(ik).*len(ik);
%             end
            
        otherwise
            
            f0 = 500;
            Rin = zin_cir_ac(dim1(ik), dim2(ik), ...
                sig(ik), mur_tmp, len(ik), f0);
            
            Rself(ik) = max(Rin, R_pul(ik)*len(ik));
            
%             if Lin_pul(ik)==0
                f0 = 5e3;
                [~, Lin] = zin_cir_ac(dim1(ik), dim2(ik),...
                    sig(ik), mur_tmp, len(ik), f0);
%             else
%                 Lin = Lin_pul(ik).*len(ik);
%             end
            
            Lself(ik) = induct_cir_ext(dim1(ik), len(ik)) + Lin;
            
    end
    
%     sign_tmp = sign(Lmtx(ik,1:Nc-1));
%     Lmtx(ik,1:Nc-1) = sign_tmp.*min(abs(Lmtx(ik,1:Nc-1)),0.99*Lmtx(ik,ik));
%     Lmtx(1:Nc-1,ik) = sign_tmp'.*min(abs(Lmtx(1:Nc-1,ik)),0.99*Lmtx(ik,ik));
    
end


end


