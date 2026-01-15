function [dfit3,efit3,rfit3,pfit3,Yfit3] = main_vectfit_y_mtx3(Yf3, frq, Nfit, flag_mod)
%  Function:       main_vectfit_y_mtx3
%  Description:    Add the ground effect to the parameter matrix. Vector
%                  fitting will be employed to generate the final
%                  parameters.
%                  Ground effect matrix Rg and Lg is calculated on a
%                  specified frequency to correct the mutual parameters.
%                  Self parameter part will be calculated based on a
%                  separate frequency dependent Rgself and Lgself.
%                  Vector fitting will be employed after parameter update.
%  Calls:          vecfit_main_Z
%  Input:          Rf3    --  R matrix (Nc*Nc*Nf)
%                  Lf3    --  L matrix (Nc*Nc*Nf)
%                  frq    --  frequency (1*Nf)
%                  Nfit   --  num. of fitting points
%  Output:         dfit3  --  final D matrix
%                  efit3  --  final e matrix
%                  rfit3  --  matrix of fitted residues (Nc*Nc*Nfit)
%                  pfit3  --  matrix of fitted poles (Nc*Nc*Nfit)
%                  Yvfit3 --  matrix of fitted admittance matrix (Nc*Nc*Nf)
%  Others:
%  Author:         Chen Hongcai
%  Email :         hc.chen@live.com
%  Date:           2016-10-30
%  ps:

Nc = size(Yf3,1);
% flag_mod = 0;
if Nc > 80
    flag_mod = 0;
end

Nf = length(frq);
Nfmax = 12;

%% 1. self parameter update with vector fitting
if Nfit>16 || Nfit<=0
    disp('Number of vector fitting points should not between (0, 16).');
    
    rfit3 = zeros(Nc,Nc,Nfmax);
    pfit3 = zeros(Nc,Nc,Nfmax);
    Yfit3 = zeros(Nc,Nc,Nf);
    
    Ytmp = Yf3(:,:,5);
    dfit3 = real(Ytmp);
    efit3 = imag(Ytmp)/(2*pi*frq(5));
    for ik = 1:Nf
        Yfit3(:,:,ik) =  dfit3 + 1j*2*pi*frq(ik).*efit3;
    end
    
    return;
end


%% vector fitting
if flag_mod == 0  % mod 0: Y matrix is fitted simultaneously
    
    [dfit3,efit3,rfit3,pfit3,Yfit3] = vecfit_kernel_y_mtx(Yf3, frq, Nfit);
    
elseif flag_mod == 1 % mod 1: half Y matrix is fitted one by one
    dfit3 = zeros(Nc,Nc);
    efit3 = zeros(Nc,Nc);
    rfit3 = zeros(Nc,Nc,Nfmax);
    pfit3 = zeros(Nc,Nc,Nfmax);
    Yfit3 = zeros(Nc,Nc,Nfmax);
    for ik = 1:Nc
        for ig = ik:Nc
            [yd,ye,yrn,ypn, Ntmp,Ytmp] = vecfit_kernel_Y(Yf3(ik,ig,:), frq, Nfit);
            if isempty(Ntmp)
                disp('empty!');
            elseif Ntmp == 0
                disp('0')
            end
            
            dfit3(ik,ig) = yd;
            efit3(ik,ig) = ye;
            rfit3(ik,ig,1:Ntmp) = yrn(1:Ntmp);
            pfit3(ik,ig,1:Ntmp) = ypn(1:Ntmp);
            Yfit3(ik,ig,1:Ntmp) = Ytmp(1,1,1:Ntmp);
        end
    end
    
    dfit3 = dfit3+dfit3'-diag(diag(dfit3));
    efit3 = efit3+efit3'-diag(diag(efit3));
    for ik = 1:Ntmp
        rfit3(:,:,ik) = rfit3(:,:,ik)+rfit3(:,:,ik)'-diag(diag(rfit3(:,:,ik)));
        pfit3(:,:,ik) = pfit3(:,:,ik)+pfit3(:,:,ik)'-diag(diag(pfit3(:,:,ik)));
        Yfit3(:,:,ik) = Yfit3(:,:,ik)+Yfit3(:,:,ik)'-diag(diag(Yfit3(:,:,ik)));
    end
    
    
end


end

