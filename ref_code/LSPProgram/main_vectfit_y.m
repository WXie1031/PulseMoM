function [G0fit,C0fit,Gvfit,Cvfit,Yvfit] = main_vectfit_y(Rself,Lself, frq, Nfit)
%  Function:       vectfit_main_Y
%  Description:    Add the ground effect to the parameter matrix. Vector
%                  fitting will be employed to generate the final
%                  parameters.
%                  Ground effect matrix Rg and Lg is calculated on a
%                  specified frequency to correct the mutual parameters.
%                  Self parameter part will be calculated based on a
%                  separate frequency dependent Rgself and Lgself.
%                  Vector fitting will be employed after parameter update.
%  Calls:          vecfit_main_Y
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
Nc = size(Rself,1);
Nf = length(frq);

G0fit = zeros(Nc,1);
C0fit = zeros(Nc,1);
Gvfit = zeros(Nc,12);
Cvfit = zeros(Nc,12);

Yvfit = zeros(Nc,Nf);

if Nfit>10
    disp('Number of vector fitting points should not exceed 10.');
    return;
end


for ik = 1:Nc
    
    if isnan(Rself(ik,2)) || isinf(Rself(ik,2))
        G0fit(ik) = Rself(ik,1);
        C0fit(ik) = Lself(ik,1);
        Yvfit(ik,:) =  1./(Rself(ik,1) + 1j*2*pi*frq.*Lself(ik,1));
    
    elseif Rself(ik,2)~=0
        
        if frq(end)<50e3
            Rtmp = Rself(ik,Nf);%spline linear
        else
            Rtmp = interp1(frq,Rself(ik,1:Nf),50e3,'linear');%spline linear
        end
        
        if abs(Rtmp-Rself(ik,1)/Rself(ik,1))>=0.4  %  >40%
            
            Ys = zeros(1,1,Nf);
            Ys(1,1,1:Nf) = 1./(Rself(ik,1:Nf) + 1j*2*pi*frq.*Lself(ik,1:Nf));
            
            [G0, C0, Gn, Cn, Ytmp] = vecfit_kernel_Y(Ys, frq, Nfit);
%             G0fit(ik) = G0-sum(Gn);
            G0fit(ik) = G0;
            C0fit(ik) = C0;
            Gvfit(ik,1:Nfit) = Gn;
            Cvfit(ik,1:Nfit) = Cn;
            Yvfit(ik,:) = Ytmp(1,1,:);
            
        elseif abs(Rtmp-Rself(ik,1)/Rself(ik,1))<0.4 && ...
                abs(Rtmp-Rself(ik,1)/Rself(ik,1))>=0.1 %  >40%
            
            Ys = zeros(1,1,Nf);
            Ys(1,1,1:Nf) = 1./(Rself(ik,1:Nf) + 1j*2*pi*frq.*Lself(ik,1:Nf));
            
            [G0, C0, Gn, Cn, Ytmp] = vecfit_kernel_Y(Ys, frq, 1);
            G0fit(ik) = G0;
            C0fit(ik) = C0;
            Gvfit(ik,1) = Gn;
            Cvfit(ik,1) = Cn;
            Yvfit(ik,:) = Ytmp(1,1,:);
            
        elseif abs(Rtmp-Rself(ik,1)/Rself(ik,1))<0.1
            G0fit(ik) = Rself(ik,1);
            C0fit(ik) = Lself(ik,1);
            Yvfit(ik,:) =  1./(Rself(ik,1) + 1j*2*pi*frq.*Lself(ik,1));
        end
        
    else
        G0fit(ik) = Rself(ik,1);
        C0fit(ik) = Lself(ik,1);
        Yvfit(ik,:) =  1./(Rself(ik,1) + 1j*2*pi*frq.*Lself(ik,1));
    end
    
end


end
