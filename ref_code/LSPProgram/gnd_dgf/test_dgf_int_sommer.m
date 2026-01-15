
mu0 = 4e-7*pi;
ep0 = 8.854187817e-12;
vc = 3e8;

err_tol = 1e-12;

%% configuration of the multi-layer structure
w0= 2*pi*30e9;
k0 = w0*sqrt(ep0.*mu0);


T = textread('GA.txt');
xf = T(:,1);

p = xf./k0;

% J0_zero = [0, besselzero(0,kp_max,1)];
% J1_zero = [0, besselzero(1,kp_max,1)];

load('bessel_zero.mat');
%% kp setting and integration path
% ppw = 40;
ppw = 20;
lumda = 2*pi*vc/w0;
dkp = pi/(lumda*ppw);

dkp = 2*pi./p;

% kp_max = 1.2*k0;
% kp_max = 1e5;

Nkp = 1e5;
J0_zero = J0_zero(1:Nkp);
J1_zero = J1_zero(1:Nkp);


kp_im = zeros(1,Nkp);
% Elliptical integration path
kp_max = min(max( 1.2*k0 ),max(abs(k0))+k0*0.2);
a = kp_max/2;
b = 1e-3*a;



Ncal = 1e6;
% p = xf;
Nf = length(p);
aa = (1+1j)/sqrt(2);
ww = 0.1;
F1=zeros(1,Nf);F2=zeros(1,Nf);F3=zeros(1,Nf);F4=zeros(1,Nf);F5=zeros(1,Nf);
F6=zeros(1,Nf);F7=zeros(1,Nf);F8=zeros(1,Nf);F9=zeros(1,Nf);F10=zeros(1,Nf);

for ik = 1:Nf
    
    kp_J0_tmp = J0_zero/p(ik);
    kp_J1_tmp = J1_zero/p(ik);
    
    Nim_J0 = sum(kp_J0_tmp<min(kp_max,kp_max))+1;
    kp_im = b .* sin(pi*(0:Nim_J0-1)/Nim_J0);
    kp_J0_tmp(1:Nim_J0) = kp_J0_tmp(1:Nim_J0)+1j*kp_im;
    

    Nim_J1 = sum(kp_J1_tmp<min(kp_max,kp_max))+1;
    kp_im = b .* sin(pi*(0:Nim_J1-1)/Nim_J1);
    kp_J1_tmp(1:Nim_J1) = kp_J1_tmp(1:Nim_J1)+1j*kp_im;

    kp_J0 = zeros(Ncal,1);
    cnt = 0;
    for ig = 1:Nkp-1 
        dkp_J0_tmp = kp_J0_tmp(ig+1)-kp_J0_tmp(ig);
        Ntmp = min(ceil(real(dkp_J0_tmp)/dkp(ik)),3);
        ind = cnt+1:cnt+Ntmp;
        
        cnt = cnt+Ntmp;
        if cnt>Ncal
            break;
        else
            kp_J0(ind) = dkp_J0_tmp/Ntmp*(0:Ntmp-1) + kp_J0_tmp(ig);
        end
    end
    
    kp_J1 = zeros(Ncal,1);
    cnt = 0;
    for ig = 1:Nkp-1
        dkp_J1_tmp = kp_J1_tmp(ig+1)-kp_J1_tmp(ig);
        Ntmp = min(ceil(real(dkp_J1_tmp)/dkp(ik)),3);
        ind = cnt+1:cnt+Ntmp;
        
        cnt = cnt+Ntmp;
        if cnt>Ncal
            break;
        else
            kp_J1(ind) = dkp_J1_tmp/Ntmp*(0:Ntmp-1) + kp_J1_tmp(ig);
        end
    end
    
    
    
    for ig = 1:Ncal-1

%         Nint_J0 = min(max(ceil(abs(kp_J0(ig+1)-kp_J0(ig))/dkp(ik)),5),255);
%         Nint_J1 = min(max(ceil(abs(kp_J1(ig+1)-kp_J1(ig))/dkp(ik)),5),255);

        dkp_J0 = kp_J0(ig+1)-kp_J0(ig);
        dkp_J1 = kp_J1(ig+1)-kp_J1(ig);
            
            
        [gau_nod_J0, gau_wei_J0] = gauss_int_coef(5);
        [gau_nod_J1, gau_wei_J1] = gauss_int_coef(5);
        
        dkp_J0_int = (kp_J0(ig+1)+kp_J0(ig))/2*gau_nod_J0;
        kp_J0_int = dkp_J0_int + (kp_J0(ig)+kp_J0(ig+1))/2;
        dkp_J1_int = (kp_J1(ig+1)+kp_J1(ig))/2*gau_nod_J1;
        kp_J1_int = dkp_J1_int + (kp_J1(ig)+kp_J1(ig+1))/2;
        
        kz_int_J0 = sqrt(k0.^2 - kp_J0_int.^2);
        id_kz = angle(kz_int_J0)>0 | angle(kz_int_J0)<=-pi;
        kz_int_J0(id_kz) = -kz_int_J0(id_kz);
        
        kz_int_J1 = sqrt(k0.^2 - kp_J1_int.^2);
        id_kz = angle(kz_int_J1)>0 | angle(kz_int_J1)<=-pi;
        kz_int_J1(id_kz) = -kz_int_J1(id_kz);
        
        J0_tmp = besselj(0,kp_J0_int.*p(ik));
        J1_tmp = besselj(1,kp_J1_int.*p(ik));
        
        % Sommerfeld identity
        F7tmp = sum( exp(-kz_int_J0*ww)./kz_int_J0.*kp_J0_int.*J0_tmp.*gau_wei_J0 ,2)*dkp_J0;
        F7new = F7(ik) + F7tmp;
        F8tmp = sum( exp(-kz_int_J1*ww)./kz_int_J1.*kp_J1_int.^2.*J1_tmp.*gau_wei_J1 ,2)*dkp_J1;
        F8new = F8(ik) + F8tmp;

         F7(ik) = F7new;
            F8(ik) = F8new;
%         if (abs(F7new-F7(ik))<(abs(F7(ik))*err_tol)) && ...
%                 (abs(F8new-F8(ik))<(abs(F8(ik))*err_tol))
%             
%             disp(['Step ', num2str(ig)]);
%             break;
%         else
%             F7(ik) = F7new;
%             F8(ik) = F8new;
%         end
    end
end

p2 = p.^2;
rr = sqrt(p2+ww.^2);

F77 = exp(1j*k0*rr)./rr;
F88 = exp(1j*k0*rr)./(rr.^3).*(1-1j*k0*rr).*p;


figure(16)
loglog(p*k0,abs(F7),'r');
hold on
loglog(p*k0,abs(F77),'k');
legend('numerical','analytical')

figure(17)
loglog(p*k0,abs(F8),'r');
hold on
loglog(p*k0,abs(F88),'k');
legend('numerical','analytical')

