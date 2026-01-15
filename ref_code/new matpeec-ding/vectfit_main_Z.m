function [R0fit,L0fit,Rvfit,Lvfit,Zvfit] = vectfit_main_Z(Rself,Lself, frq, Nfit, Lmtx)
%  Function:       vectfit_main_Z
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
%  ps:
%  plot R and L using these formulas
%     Rfcur(ik) = Rdc+sum( (wplt(ik).^2.*Rn(:,1:Nfit).*Ln(:,1:Nfit).^2)./(Rn(:,1:Nfit).^2+wplt(ik).^2.*Ln(:,1:Nfit).^2));
%     Lfcur(ik) = L0+sum( (Rn(:,1:Nfit).^2.*Ln(:,1:Nfit))./(Rn(:,1:Nfit).^2+wplt(ik).^2.*Ln(:,1:Nfit).^2));

% Lmin is the max value of the mutual inductance. Limit the L0fit value to
% ensure the stability

%% 2. self parameter update with vector fitting
Nc = size(Rself,1);
Nf = length(frq);

fbrk = 50e3;

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

    ia=0;
for ik = 1:Nc
    

    %L0min = max(Lmtx(ik+1:end,ik).^2./Ldiag(ik+1:end));
    L0min = max(Lmtx(ik+1:end,ik));
    if isempty(L0min)
        L0min = 0;
    end
        
    if isnan(Rself(ik,2)) || isinf(Rself(ik,2))
        R0fit(ik) = Rself(ik,1);
        L0fit(ik) = Lself(ik,1);
        Zvfit(ik,:) = R0fit(ik) + 1j*2*pi*frq.*L0fit(ik);
    
    elseif Rself(ik,2)~=0
        
        if frq(end)<fbrk
            Rtmp = Rself(ik,Nf);%spline linear
        else
            Rtmp = interp1(frq,Rself(ik,1:Nf),fbrk,'linear');%spline linear
        end
        
%         if (abs((Rtmp-Rself(ik,1))/Rself(ik,1))>5 && Nfit<3)
%             
%             Ltmp = Rself(ik,Nf)./(2*pi*sqrt(frq*frq(Nf)));
%             Lmax = Rself(ik,Nf)^2/Rself(ik,1)/(2*pi*frq(Nf));
%             Ltmp(Ltmp>Lmax)=Lmax;
% 
%             Zs = zeros(1,1,Nf);
%             Zs(1,1,1:Nf) = Rself(ik,1:Nf) + 1j*2*pi*frq.*Ltmp;
%             
%             [R0, L0, Rn, Ln, Ntmp, Ztmp] = vecfit_kernel_Z(Zs, frq, 3);
%             
%             R0fit(ik) = R0-sum(Rn);
%             L0fit(ik) = L0 - (Ltmp(end)- Lself(ik,end));
%             Rvfit(ik,1:Ntmp) = Rn;
%             Lvfit(ik,1:Ntmp) = Ln;
%             Zvfit(ik,:) = Ztmp(1,1,:);
        if abs((Rtmp-Rself(ik,1))/Rself(ik,1))>4  % >100%
            Zs = zeros(1,1,Nf);
            Zs(1,1,1:Nf) = Rself(ik,1:Nf) + 1j*2*pi*frq.*Lself(ik,1:Nf);
            
            [R0, L0, Rn, Ln, Ntmp,Ztmp] = vecfit_kernel_Z(Zs, frq, Nfit, L0min);
            R0fit(ik) = R0;
            L0fit(ik) = L0;
            Rvfit(ik,1:Ntmp) = Rn;
            Lvfit(ik,1:Ntmp) = Ln;
            Zvfit(ik,:) = Ztmp(1,1,:);
            
        elseif abs((Rtmp-Rself(ik,1))/Rself(ik,1))<=4  && ...
            abs((Rtmp-Rself(ik,1))/Rself(ik,1))>0.4 %  40%~400%
        
            Zs = zeros(1,1,Nf);
            Zs(1,1,1:Nf) = Rself(ik,1:Nf) + 1j*2*pi*frq.*Lself(ik,1:Nf);
            
            ia=ia+1
            if ia==163
                disp('end');
            end
            [R0, L0, Rn, Ln, Ntmp,Ztmp] = vecfit_kernel_Z(Zs, frq, 2, L0min);
            
            R0fit(ik) = R0;
            L0fit(ik) = L0;
            Rvfit(ik,1:Ntmp) = Rn;
            Lvfit(ik,1:Ntmp) = Ln;
            Zvfit(ik,:) = Ztmp(1,1,:);
            
        elseif abs((Rtmp-Rself(ik,1))/Rself(ik,1))<=0.4 && ...
                 abs((Rtmp-Rself(ik,1))/Rself(ik,1))>0.2 %  20%~40%
            
            Zs = zeros(1,1,Nf);
            Zs(1,1,1:Nf) = Rself(ik,1:Nf) + 1j*2*pi*frq.*Lself(ik,1:Nf);
            
            [R0, L0, Rn, Ln, ~, Ztmp] = vecfit_kernel_Z(Zs, frq, 1, L0min);
            R0fit(ik) = R0;
            L0fit(ik) = L0;
            Rvfit(ik,1) = Rn;
            Lvfit(ik,1) = Ln;
            Zvfit(ik,:) = Ztmp(1,1,:);
        elseif abs((Rtmp-Rself(ik,1))/Rself(ik,1))<=0.2
            R0fit(ik) = Rself(ik,5);
            L0fit(ik) = Lself(ik,5);
            Zvfit(ik,:) = R0fit(ik) + 1j*2*pi*frq.*L0fit(ik);
        end
        
        if R0fit(ik)<=0 
%         if R0fit(ik)<=0 || L0fit(ik)<=0     
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

%
end
