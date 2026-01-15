function [dfit3,efit3,rfit3,pfit3,Yvfit3] = main_vectfit_ym_mtx3(Yf3, frq, Nfit)
%  Function:       main_vectfit_ym_mtx3
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


%% 2. self parameter update with vector fitting
Nc = size(Yf3,1);
Nf = length(frq);


if Nfit>10 || Nfit<=0
    disp('Number of vector fitting points should be between (1, 10).');
    
    dfit3 = zeros(Nc,Nc);
    efit3 = zeros(Nc,Nc);
    rfit3 = zeros(Nc,Nc,12);
    pfit3 = zeros(Nc,Nc,12);
    Yvfit3 = zeros(Nc,Nc,Nf);

    fi = 100;
    ind = find(frq == fi, 1);
    if ~isempty(ind)
        dfit3 = real(Yf3(:,:,ind));
    elseif frq(Nf)<fi
        dfit3 = real(Yf3(:,:,Nf));
    elseif max(frq)>fi
        for ik = 1:Nc
            for ig = 1:Nc
                dfit3(ik,ig) = interp1(frq, real(squeeze(Yf3(ik,ig,:))), fi, 'linear');
            end
        end
    end
    
    fi = 10e3;
    ind = find(frq == fi, 1);
    if ~isempty(ind)
        efit3 = imag(Yf3(:,:,ind))./(2*pi*frq(ind));
    elseif frq(Nf)<fi
        efit3 = imag(Yf3(:,:,Nf))./(2*pi*frq(Nf));
    else
        for ik = 1:Nc
            for ig = 1:Nc
                efit3(ik,ig) = interp1(frq, squeeze(imag(Yf3(ik,ig,:))), fi, 'linear');
            end
        end
        efit3 = efit3./(2*pi*fi);
    end
    
    dfit3 = real(dfit3);
    efit3 = imag(efit3);
    
    for ik = 1:Nf
        Yvfit3(:,:,ik) =  dfit3 + 1j*2*pi*frq(ik).*efit3;
    end
    
    return;
end



%% vector fitting on Y
Ysfit3 = zeros(Nc,Nc,Nf);

fi = 500;
ind = find(frq == fi, 1);
if ~isempty(ind)
    Ytmp = ((Yf3(:,:,ind)));
elseif frq(Nf)<fi
    Ytmp = ((Yf3(:,:,Nf)));
else
    Ytmp = zeros(Nc,Nc);
    for ik = 1:Nc
        for ig = 1:Nc
            Ytmp(ik,ig) = interp1(frq, squeeze((Yf3(ik,ig,:))), fi, 'linear');
        end
    end
end

dsfit3 = diag(real(sum(Ytmp,2)));


fi = 10e3;
ind = find(frq == fi, 1);
if ~isempty(ind)
    esfit3 = diag(imag(sum(Yf3(:,:,ind),2))/(2*pi*frq(ind)));
elseif frq(Nf)<fi
    esfit3 = diag(imag(sum(Yf3(:,:,Nf),2))/(2*pi*frq(Nf)));
else
    esfit3 = zeros(Nc,Nc);
    for ik = 1:Nc
        for ig = 1:Nc
            esfit3(ik,ig) = interp1(frq, squeeze(imag(Yf3(ik,ig,:))), fi, 'linear');
        end
    end
    esfit3 = diag(sum(esfit3,2))/(2*pi*fi);
end



for ik = 1:Nf
    Ysfit3(:,:,ik) = dsfit3 + esfit3*(2*pi*frq(ik));
end


[dtmp,etmp,rfit_tmp,pfit_tmp,Yfit_tmp] = vecfit_kernel_y_mtx(Yf3,frq,Nfit);

doff = dtmp-diag(diag(dtmp));
eoff = etmp-diag(diag(etmp));
dfit3 = dsfit3 + doff - diag(sum(doff,1));
efit3 = esfit3 + eoff - diag(sum(eoff,1));

for ik=1:Nc
    rfit_tmp(ik,ik,:)=0;
    pfit_tmp(ik,ik,:)=0;
    Yfit_tmp(ik,ik,:)=0;
end

rfit3(:,:,1:Nfit) = rfit_tmp;
pfit3(:,:,1:Nfit) = pfit_tmp;

Yvfit3 = Ysfit3 + Yfit_tmp;


end

