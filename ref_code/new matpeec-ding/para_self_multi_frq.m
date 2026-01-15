function [Rself, Lself] = para_self_multi_frq(shape, dim1, dim2, ...
    len, R_pul, sig, mur, frq)
%  Function:       para_main_self_vf
%  Description:    Calculate R and length matrix of all conductors using
%                  frequency dependent fitting model. Cable group which
%                  calculated using meshing method with update the matrix
%                  outside this function.
%
%  Calls:          cal_L_filament
%                  resis_bar_ac
%                  induct_bar_ac
%                  resis_induct_cir_ac
%
%  Input:          pt_start  --  start point of conductors (N*3) (m)
%                  pt_end    --  end point of conductors (N*3) (m)
%                  dv        --  direction vector of conductors (N*3)
%                  re        --  equivalent radius (N*1)
%                  Rin_pul   --  resistance of conductors (N*1) (ohm/m)
%                  Lin_pul   --  internal L of conductors (N*1) (H/m)
%                  len       --  length of conductors (N*1)
%                  Ndat_3D   --  num. of conductors (N*1)
%  Output:         Rmtx    --  R matrix
%                  Lmtx    --  L matrix
%                  Rself   --  self R matrix of different frequency (Nc*Nf)
%                  Lself   --  self L matrix of different frequency (Nc*Nf)
%  Others:
%  Author:         Chen Hongcai
%  Email :         hc.chen@live.com
%  Date:           2014-12-13
%
%  History:        modified to frequency dependent self matrix version.
%                  2015-06-25


Nf = length(frq);
Nc = size(dim1,1);

Rself = zeros(Nc,Nf);
Lself = zeros(Nc,Nf);

dim1 = abs(dim1);
dim2 = abs(dim2);

% if Nf > 16
%     error('Frequency point should less than 16.');
% end

%% 1. calculate the self R and L of all conductors.
for ik = 1:Nc
    %         idrec = shape == 1002;
    %         idagi = shape == 1003;
    %         idcir = ~(idrec|idagi);
    %         10001 % plate
    % L and R calculate at various frerquncies to generate vector fitting
    % network
    if size(mur,2)==1
        mur_frq=mur(ik)*ones(1,Nf);
    else
        mur_frq = mur(ik,:);
    end
    
    switch shape(ik)
        case 1002
            for ig = 1:Nf
                mur_tmp = max(mur_frq(ig), 1);
                Rself(ik,ig) = resis_bar_ac(dim1(ik), dim2(ik), ...
                    R_pul(ik), sig(ik), mur_tmp, len(ik), frq(ig));
                Rself(ik,ig) = max(Rself(ik,ig), R_pul(ik)*len(ik));
                
                Lself(ik,ig) = induct_bar_ac(dim1(ik), dim2(ik), ...
                    sig(ik), mur_tmp, len(ik), frq(ig));
            end
            
            
        case 1003
            for ig = 1:Nf
                mur_tmp = max(mur_frq(ig), 1);
                Rself(ik,ig) = resis_agi_ac(dim1(ik), dim2(ik), ...
                    R_pul(ik), sig(ik), mur_tmp, len(ik), frq(ig));
                Rself(ik,ig) = max(Rself(ik,ig), R_pul(ik)*len(ik));
                
                Lself(ik,ig) = induct_agi_ac(dim1(ik), dim2(ik), ...
                    sig(ik), mur_tmp, len(ik), frq(ig));
            end
            
        case 10001 % plate
            Lext = induct_tape(dim1(ik), len(ik));

            for ig = 1:Nf
                mur_tmp = max(mur_frq(ig), 1);
                [Rself(ik,ig), Lself(ik,ig)] = zs_plate_sonnet(...
                    len(ik), sig(ik), mur_tmp, frq(ig));
                     
                Rself(ik,ig) = max(Rself(ik,ig), R_pul(ik)*len(ik));
            end
            
            Lself(ik,1:Nf) = Lself(ik,1:Nf)+Lext;
            
        otherwise
            Lext = induct_cir_ext(dim1(ik), len(ik));
            
            for ig = 1:Nf
                mur_tmp = max(mur_frq(ig), 1);
                [Rself(ik,ig), Lself(ik,ig)]= zin_cir_ac(...
                    dim1(ik), dim2(ik), sig(ik), mur_tmp, len(ik), frq(ig));
                
                Rself(ik,ig) = max(Rself(ik,ig), R_pul(ik)*len(ik));
            end
            
            Lself(ik,1:Nf) = Lself(ik,1:Nf)+Lext;
    end
    
end



end


