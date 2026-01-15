function [Rmtx, Lmtx] = update_vectfit_to_main(Rmtx, Lmtx, R0fit, L0fit)
%  Function:       update_ground_to_main
%  Description:    Add the ground effect to the parameter matrix. Vector
%                  fitting will be employed to generate the final
%                  parameters.
%                  Ground effect matrix Rg and Lg is calculated on a
%                  specified frequency to correct the mutual parameters.
%                  Self parameter part will be calculated based on a
%                  separate frequency dependent Rgself and Lgself.
%                  Vector fitting will be employed after parameter update.
%  Calls:          vecfit_main_Z
%  Input:          Rmtx    --  R matrix
%                  Lmtx    --  L matrix
%                  Rself   --  self R matrix of different frequency (Nc*Nf)
%                  Lself   --  self L matrix of different frequency (Nc*Nf)
%                  Rg      --  ground effect R matrix
%                  Lg      --  ground effect L matrix
%                  Rself   --  self R matrix of different frequency (Nc*Nf)
%                  Lself   --  self L matrix of different frequency (Nc*Nf)
%                  Nfit    --  num. of fitting points
%  Output:         Rfinal  --  final R matrix
%                  Lmtx    --  final L matrix
%                  Rfit    --  self R matrix of fitted result (Nc*Nfit)
%                  Lself   --  self L matrix of fitted result (Nc*Nfit)
%  Others:
%  Author:         Chen Hongcai
%  Email :         hc.chen@live.com
%  Date:           2014-06-25



%% 2. self parameter update with vector fitting
Nc = size(Rmtx,1);

for ik = 1:Nc
    
    if R0fit(ik)>0
        Rmtx(ik,ik) = R0fit(ik);
    end
    if L0fit(ik)>0
        Lmtx(ik,ik) = L0fit(ik);
    end
    
%     sign_tmp = sign(Lmtx(ik,1:Nc-1));
%     Lmtx(ik,1:Nc-1) = sign_tmp.*min(abs(Lmtx(ik,1:Nc-1)),0.99*Lmtx(ik,ik));
%     Lmtx(1:Nc-1,ik) = sign_tmp'.*min(abs(Lmtx(1:Nc-1,ik)),0.99*Lmtx(ik,ik));
    
end



