function [R0fit3,L0fit3,Rvfit3,Lvfit3,Zvfit3] = main_vectfit_z_mtx3(Rf3,Lf3, frq, Nfit)
%  Function:       main_vectfit_z_mtx3
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
if nargin == 3 % input Z not R,L
    Nfit = frq;
    frq = Lf3;
end

Nc = size(Rf3,1);
Nf = length(frq);

R0fit3 = zeros(Nc,Nc,1);
L0fit3 = zeros(Nc,Nc,1);
Rvfit3 = zeros(Nc,Nc,12);
Lvfit3 = zeros(Nc,Nc,12);
Zvfit3 = zeros(Nc,Nc,Nf);

if Nfit>10
    disp('Number of vector fitting points should not exceed 10.');
    
    if nargin == 3 % input Z not R,L
        R0fit3 = real(Rf3(:,:,5));
        L0fit3 = imag(Rf3(:,:,5))/(2*pi*frq(5));
        Zvfit3 =  R0fit3*ones(1,1,Nf) + 1j*2*pi*ones(Nc,1)*frq.*(L0fit3*ones(1,Nf));
        return;
    else
        R0fit3 = Rf3(:,:,5);
        L0fit3 = Lf3(:,:,5);
        Zvfit3 =  R0fit3*ones(1,1,Nf) + 1j*2*pi*ones(Nc,1)*frq.*(L0fit3*ones(1,Nf));
        return;
    end
end


if nargin == 3 % input Z not R,L

%    Ztmp = Rf3;
%     Rf3 = zeros(Nc,Nc,Nf);
%     Lf3 = zeros(Nc,Nc,Nf);
%     for ik = 1:Nf
%         Rf3(:,:,ik) = real(Ztmp(:,:,ik));
%         Lf3(:,:,ik) = imag(Ztmp(:,:,ik))/(2*pi*frq(ik));
%     end
    
    Zsf3 = zeros(Nc,Nc,Nf);
    %Zmf3 = zeros(Nc,Nc,Nf);
    for ik = 1:Nf
        Zsf3(:,:,ik) = diag(diag(Rf3(:,:,ik))); 
    end
    Zmf3 = Rf3-Zsf3;
    
else
    Zsf3 = zeros(Nc,Nc,Nf);
    Zmf3 = zeros(Nc,Nc,Nf);
    
    for ik = 1:Nf
        Zsf3(:,:,ik) = diag(diag(Rf3(:,:,ik))) + 1j*2*pi*frq(ik)*diag(diag(Lf3(:,:,ik)));
    end
    
    for ik = 1:Nf
        Zmf3(:,:,ik) = Rf3(:,:,ik) + 1j*2*pi*frq(ik)*Lf3(:,:,ik);
    end
    %Zmf3 = Zmf3-Zsf3;
    
end


% [R0sfit3,L0sfit3,Rsfit3,Lsfit3,Zsfit3] = vecfit_kernel_Z_mtx(Zsf3, frq, Nfit);
% 
% Nfit_off = 2;
% Nfit_off = min(Nfit_off, Nfit);
% vf_mod = 3;
% [R0mfit3,L0mfit3,Rmfit3,Lmfit3,Zmfit3] = vecfit_kernel_Z_mtx(Zmf3, frq, Nfit_off, vf_mod);
% 
% 
% R0fit3 = R0sfit3 + R0mfit3;
% L0fit3 = L0sfit3 + L0mfit3;
% 
% Rvfit3(:,:,1:Nfit_off) = Rvfit3(:,:,1:Nfit_off) + Rmfit3;
% Lvfit3(:,:,1:Nfit_off) = Lvfit3(:,:,1:Nfit_off) + Lmfit3;
% Zvfit3 = Zvfit3 + Zmfit3;
% 
% Rvfit3(:,:,1:Nfit) = Rvfit3(:,:,1:Nfit) + Rsfit3;
% Lvfit3(:,:,1:Nfit) = Lvfit3(:,:,1:Nfit) + Lsfit3;
% Zvfit3 = Zvfit3 + Zsfit3;

[R0fit3,L0fit3,Rvfit3,Lvfit3,Zvfit3] = vecfit_kernel_Z_mtx(Zmf3, frq, Nfit);

end

