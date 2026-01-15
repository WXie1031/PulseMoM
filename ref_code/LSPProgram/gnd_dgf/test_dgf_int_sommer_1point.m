
mu0 = 4e-7*pi;
ep0 = 8.854187817e-12;
vc = 3e8;

err_tol = 1e-12;

%% configuration of the multi-layer structure
w0= 2*pi*30e9;
k0 = w0*sqrt(ep0.*mu0);


T = textread('GA.txt');
xf = T(:,1);

p = [xf(1);xf(end)]./k0;

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



Ncal = 1e5;
% p = xf;
Nf = length(p);
aa = (1+1j)/sqrt(2);
ww = 0.1;
F7=zeros(2,Nf);

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
    
    
    for ig = 1:Ncal-1

%         Nint_J0 = min(max(ceil(abs(kp_J0(ig+1)-kp_J0(ig))/dkp(ik)),5),255);
%         Nint_J1 = min(max(ceil(abs(kp_J1(ig+1)-kp_J1(ig))/dkp(ik)),5),255);

        dkp_J0 = kp_J0(ig+1)-kp_J0(ig);

        [gau_nod_J0, gau_wei_J0] = gauss_int_coef(5);
        
        dkp_J0_int = (kp_J0(ig+1)+kp_J0(ig))/2*gau_nod_J0;
        kp_J0_int = dkp_J0_int + (kp_J0(ig)+kp_J0(ig+1))/2;

        
        kz_int_J0 = sqrt(k0.^2 - kp_J0_int.^2);
        id_kz = angle(kz_int_J0)>0 | angle(kz_int_J0)<=-pi;
        kz_int_J0(id_kz) = -kz_int_J0(id_kz);

        J0_tmp = besselj(0,kp_J0_int.*p(ik));
        
        % Sommerfeld identity
        F7(ik,ig+1) = sum( exp(-kz_int_J0*ww)./kz_int_J0.*kp_J0_int.*J0_tmp.*gau_wei_J0 ,2)*dkp_J0;

        if (abs(F7(ik,ig)-F7(ik,ig+1))<(abs(F7(ik,ig))*err_tol)) 
            disp(['Step ', num2str(ig)]);
            break;
        end
    end
end

p2 = p.^2;
rr = sqrt(p2+ww.^2);

F77 = exp(1j*k0*rr)./rr;



figure(16)
loglog(abs(F7(1,:)),'r');
% hold on
% loglog(p*k0,abs(F77),'k');
legend('numerical','analytical')

figure(17)
loglog(abs(F7(2,:)),'r');
% hold on
% loglog(p*k0,abs(F88),'k');
legend('numerical','analytical')

