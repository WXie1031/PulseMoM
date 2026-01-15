function [R0fit,L0fit, Rvfit,Lvfit, Zvfit] = vectfit_main_Zin(Rself, Lself, ...
    frq, Nfit)
%  Function:       vectfit_main_Zin
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


% = 1 --> D=0,  E=0
% = 2 --> D~=0, E=0
% = 3 --> D~=0, E~=0
VFmode = 2;

%% 1. self parameter update with vector fitting
Nc = size(Rself,1);
Nf = length(frq);

R0fit = zeros(Nc,1);
L0fit = zeros(Nc,1);
    
Rvfit = zeros(Nc,12);
Lvfit = zeros(Nc,12);

Zvfit = zeros(Nc,Nf);

if Nfit>10
    disp('Number of vector fitting points should not exceed 10.');
    
    R0fit = Rself(:,5);
    L0fit = Lself(:,5);

    Zvfit =  R0fit*ones(1,Nf) + 1j*2*pi*ones(Nc,1)*frq.*(L0fit*ones(1,Nf));
    return;
end


for ik = 1:Nc
    
    if isnan(Rself(ik,2)) || isinf(Rself(ik,2))
        R0fit(ik) = Rself(ik,1);
        L0fit(ik) = Lself(ik,1);
        Zvfit(ik,:) = R0fit(ik) + 1j*2*pi*frq.*L0fit(ik);
    
    elseif Rself(ik,2)~=0
        
        if frq(end)<50e3
            Rtmp = Rself(ik,Nf);%spline linear
        else
            Rtmp = interp1(frq,Rself(ik,1:Nf),50e3,'linear');%spline linear
        end
        
        if (Rtmp-Rself(ik,1))/Rself(ik,1)>0.4  %  >40%
            
            Zs = zeros(1,1,Nf);
            Zs(1,1,1:Nf) = Rself(ik,1:Nf) + 1j*2*pi*frq.*(Lself(ik,1:Nf)-Lself(ik,Nf)*0.8);
            
            [R0, ~, Rn,Ln, Ntmp,Ztmp] = vecfit_kernel_Zin(Zs, frq, Nfit, VFmode);
            R0fit(ik) = R0-sum(Rn);
            L0fit(ik) = Lself(ik,Nf)*0.8;
            Rvfit(ik,1:Ntmp) = Rn;
            Lvfit(ik,1:Ntmp) = Ln;
            Zvfit(ik,:) = squeeze(Ztmp(1,1,:))+1j*2*pi*frq'*L0fit(ik);

        elseif (Rtmp-Rself(ik,1))/Rself(ik,1)<=0.4 && ...
                (Rtmp-Rself(ik,1))/Rself(ik,1)>0.2 %  >40%
            
            Zs = zeros(1,1,Nf);
            Zs(1,1,1:Nf) = Rself(ik,1:Nf) + 1j*2*pi*frq.*(Lself(ik,1:Nf)-Lself(ik,Nf)*0.8);

            [R0, ~, Rn, Ln,Ntmp,Ztmp] = vecfit_kernel_Zin(Zs, frq, 1, VFmode);
            R0fit(ik) = R0-sum(Rn);
            L0fit(ik) = Lself(ik,Nf)*0.8;
            Rvfit(ik,1:Ntmp) = Rn;
            Lvfit(ik,1:Ntmp) = Ln;
            Zvfit(ik,:) = squeeze(Ztmp(1,1,:))+1j*2*pi*frq'*L0fit(ik);
        elseif (Rtmp-Rself(ik,1))/Rself(ik,1)<=0.2
            R0fit(ik) = Rself(ik,5);
            L0fit(ik) = Lself(ik,5);
            Zvfit(ik,:) = R0fit(ik) + 1j*2*pi*frq.*L0fit(ik);
        end
        
    else
        R0fit(ik) = Rself(ik,5);
        L0fit(ik) = Lself(ik,5);
        Zvfit(ik,:) = R0fit(ik) + 1j*2*pi*frq.*L0fit(ik);
    end
    
end


end
