function Hcov_T = sr_lossy_h_conv(Ha0_T, t_sr, soil_sig,soil_epr, flag_type)

mu0 = 4*pi*1e-7;
ep0 = 8.85*1e-12;

Nt = length(t_sr);
dt = t_sr(2)-t_sr(1);


Nc = size(Ha0_T,1);
Hcov_T = zeros(Nc,Nt);

ita = sqrt(mu0/(soil_epr*ep0));

if flag_type == 1 % direct from paper 
    
    tau = soil_sig/(2*soil_epr*ep0);
    
    % be careful of the time sequence used. From t2=1*dt to tn=max(t) !!
    % the first one equals to 0 and not calculated here
    tau_n = tau*(t_sr(2:Nt));
    taun_1 = tau*(t_sr(1:Nt-1));
    
    Zcof = zeros(Nt-1,1);
    Zcof(1:Nt-1) = ( -besseli(0,tau_n).*exp(-tau_n) ...
        +besseli(0,taun_1).*exp(-taun_1)  );
    
    idnan = find(isinf(Zcof)|isnan(Zcof));
    if ~isempty(idnan)
        Zcof(idnan) = ( -besseli_exp(0,tau_n(idnan)) + besseli_exp(0,taun_1(idnan)) );
    end

    for ik = 1:Nc
        
        Hmtx = zeros(Nt-1,Nt-1);
        for ig = 1:Nt-1
            Hmtx(ig:Nt-1,ig) = Ha0_T(ik,1:Nt-ig);
        end
        
        Hcov_T(ik,2:Nt) = Hmtx*Zcof;
    end
    
    Hcov_T = ita * ( Ha0_T - Hcov_T );
 
elseif flag_type == 2  % use vector fitting from paper
    
    Nfit = 12;

%     frq = [0.1 0.2 0.5 1 2 5 10 15 20 30 50 100 200 500 1e3 2e3 5e3 10e3 20e3 50e3 ...
%         100e3 200e3 500e3 1e6 2e6 5e6 1e7 2e7 5e7];
    frq = logspace(-3,log10(5e7),10000);
    Nf = length(frq);
    Zin = zeros(1,1,Nf);
    
    tau = (soil_epr*ep0)/soil_sig;
    %Zin(1,1,1:Nf) = ita*sqrt( (1j*2*pi*frq*tau)./(1j*2*pi*frq*tau+1) );
    Zin(1,1,1:Nf) = ita*sqrt( (1j*2*pi*frq)./(1j*2*pi*frq+1./tau) );
    vf_mod = 2;
    [R0,L0, Rn,Ln,Nfit,~] = vecfit_kernel_Z(Zin, frq, Nfit, 0, vf_mod);
    
    pn = zeros(Nfit,1);
    rn = zeros(Nfit,1);
    if ndims(Rn) == 3
        pn(1:Nfit) = -Rn(1,1,1:Nfit).^2./Ln(1,1,1:Nfit);
        rn(1:Nfit) = -Rn(1,1,1:Nfit)./Ln(1,1,1:Nfit);
    elseif ndims(Rn) == 2
        pn(1:Nfit) = -Rn(1,1:Nfit).^2./Ln(1,1:Nfit);
        rn(1:Nfit) = -Rn(1,1:Nfit)./Ln(1,1:Nfit);
    end
    KcofA = exp(pn./tau*dt);
    KcofB = 1./pn.*(exp(pn./tau*dt)-1);
    
    
    for ik = 1:Nc
        Kn = zeros(Nfit,Nt);
        for ig = 2:Nt
            Kn(1:Nfit,ig) = KcofA.*Kn(1:Nfit,ig-1) + KcofB.*Ha0_T(ik,ig-1);
        end
        
        Hcov_T(ik,1:Nt) = R0*Ha0_T(ik,1:Nt) + sum(rn*ones(1,Nt).*Kn,1);
    end
        
elseif flag_type == 3  % use vector fitting from Ding
    
    Nfit = 12;

    
%     frq = [0.1 0.2 0.5 1 2 5 10 15 20 30 50 100 200 500 1e3 2e3 5e3 10e3 20e3 50e3 ...
%         100e3 200e3 500e3 1e6 2e6 5e6 1e7 2e7 5e7];
    frq = logspace(-3,log10(5e7),10000);
    Nf = length(frq);
    Zin = zeros(1,1,Nf);
    
    tau = (soil_epr*ep0)/soil_sig;
    Zin(1,1,1:Nf) = ita*sqrt( (1j*2*pi*frq*tau)./(1j*2*pi*frq*tau+1) );

    vf_mod = 2;
    [R0,L0, Rn,Ln,Nfit,~] = vecfit_kernel_Z(Zin, frq, Nfit, 0, vf_mod);
    
    Hcov_cof=zeros(Nt,Nfit);
    if ndims(Rn) == 3
        for ih=1:Nfit
            Hcov_cof(:,ih)= -Rn(1,1,ih)^2/Ln(1,1,ih)*exp(-Rn(1,1,ih)/Ln(1,1,ih).*(t_sr));
        end
    elseif ndims(Rn) == 2
        for ih=1:Nfit
            Hcov_cof(:,ih) = -Rn(1,ih)^2/Ln(1,ih)*exp(-Rn(1,ih)/Ln(1,ih).*(t_sr));
        end
    end
    
    
    for ik = 1:Nc
        Hcov_vf = zeros(2*Nt-1,Nfit);
        for ig = 1:Nfit
            Hcov_vf(:,ig) = dt*conv(Hcov_cof(1:Nt,ig),Ha0_T(ik,1:Nt));
        end
        Hcov_3 = sum(Hcov_vf,2)';
        
        Hcov_T(ik,1:Nt) = R0*Ha0_T(ik,1:Nt) + Hcov_3(1:Nt);
    end
 
elseif flag_type == 4  % direct convolution / not stable because complex number appears
    frq = logspace(-3,log10(5e7),2^15);
    Nf = length(frq);
    Zin = zeros(1,Nf);
    
    tau = (soil_epr*ep0)/soil_sig;
    %Zin(1,1,1:Nf) = ita*sqrt( (1j*2*pi*frq*tau)./(1j*2*pi*frq*tau+1) );
    Zin(1,1:Nf) = ita*sqrt( (1j*2*pi*frq)./(1j*2*pi*frq+1./tau) );
    Zt = ifft(Zin);
    
    for ik = 1:Nc
        Hcov_tmp = conv(Ha0_T(ik,1:Nt), Zt);
        Hcov_T(ik,1:Nt) = real(Hcov_tmp(1:Nt));
    end
    
end

% figure(99)
% hold on
% plot(t_sr*1e6,Hcov_T)


end

