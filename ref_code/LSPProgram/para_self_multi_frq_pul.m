function [Rself,Lself] = para_self_multi_frq_pul(shape, dim1,dim2, R_pul, sig,mur, frq)
%  Function:       para_self_multi_frq_pul
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
%  Input:          re        --  equivalent radius (N*1)
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

len_pul = 1;

if Nf > 16
    error('Frequency point should less than 16.');
end

%% 1. calculate the self R and L of all conductors.
for ik = 1:Nc
    %         idrec = shape == 1002;
    %         idagi = shape == 1003;
    %         idcir = ~(idrec|idagi);
    
    % L and R calculate at various frerquncies to generate vector fitting
    % network
    switch shape(ik)
        case 1002
            for ig = 1:Nf
                mur_tmp = max(mur(ik,ig), 1);
                Rself(ik,ig) = resis_bar_ac(dim1(ik), dim2(ik), ...
                    R_pul(ik), sig(ik),  mur_tmp, len_pul, frq(ig));
                
                Lself(ik,ig) = induct_bar_ac(dim1(ik), dim2(ik), ...
                    sig(ik),  mur_tmp, len_pul, frq(ig));
            end
            
            
        case 1003
            for ig = 1:Nf
                mur_tmp = max(mur(ik,ig), 1);
                Rself(ik,ig) = resis_agi_ac(dim1(ik), dim2(ik), ...
                    R_pul(ik), sig(ik),  mur_tmp, len_pul, frq(ig));
                Rself(ik,ig) = max(Rself(ik,ig), R_pul(ik));
                
                Lself(ik,ig) = induct_agi_ac(dim1(ik), dim2(ik), ...
                    sig(ik), mur_tmp, len_pul, frq(ig));
            end
            
        otherwise
            Lext = induct_cir_ext(dim1(ik),len_pul);
            
            for ig = 1:Nf
                mur_tmp = max(mur(ik,ig), 1);
                [Rself(ik,ig), Lself(ik,ig)]= zin_cir_ac(...
                    dim1(ik), dim2(ik), sig(ik), mur_tmp, len_pul, frq(ig));
                
                Rself(ik,ig) = max(Rself(ik,ig), R_pul(ik));
            end
            
            Lself(ik,1:Nf) = Lself(ik,1:Nf)+Lext;
    end
    
end



end


