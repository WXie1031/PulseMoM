
err_tol = 1e-12;


mu0 = 4e-7*pi;
ep0 = 8.854187817e-12;
vc = 3e8;

%% configuration of the multi-layer structure
w0= 2*pi*30e9;
k0 = w0*sqrt(ep0.*mu0);


T = textread('GA.txt');
xf = T(:,1);

% J0_zero = [0, besselzero(0,kp_max,1)];
% J1_zero = [0, besselzero(1,kp_max,1)];

load('bessel_zero.mat');
%% kp setting and integration path
% ppw = 40;
ppw = 20;
lumda = 2*pi*vc/w0;
dkp = pi/(lumda*ppw);
dkp = pi./p;
% kp_max = 1.2*k0;
% kp_max = 1e5;

Nkp = 1e5;
J0_zero = J0_zero(1:Nkp);
J1_zero = J1_zero(1:Nkp);


kp_im = zeros(1,Nkp);
% Elliptical integration path
kp_max = min(max( 1.2*k0 ),max(abs(k0))+k0*0.2);
a = kp_max/2;
b = 10e-3*a;




p = xf./k0;
% p = xf;
Nf = length(p);
aa = (1+1j)/sqrt(2);
ww = 1;
F7=zeros(1,Nf);F8=zeros(1,Nf);

for ik = 1:Nf
    ik
    kp_J0 = J0_zero/p(ik);
    kp_J1 = J1_zero/p(ik);
    
    Nim_J0 = sum(kp_J0<min(kp_max,kp_max))+1;
    kp_im = b .* sin(pi*(0:Nim_J0-1)/Nim_J0);
    kp_J0(1:Nim_J0) = kp_J0(1:Nim_J0)-1j*kp_im;
    
    Nim_J1 = sum(kp_J1<min(kp_max,kp_max))+1;
    kp_im = b .* sin(pi*(0:Nim_J1-1)/Nim_J1);
    kp_J1(1:Nim_J1) = kp_J1(1:Nim_J1)-1j*kp_im;
    
    
    for ig = 1:Nkp-1
        
        dkp_J0 = kp_J0(ig+1)-kp_J0(ig);
        dkp_J1 = kp_J1(ig+1)-kp_J1(ig);
        

        kz0 = @(x0) -1j*sqrt(k0^2 - x0.^2);

%         signkz0 = @(x0) (angle(kz0(x0))>0 | angle(kz0(x0))<=-pi);
%         kz0 = @(x0) -signkz0(x0).*kz0(x0);
%         
        kz1 = @(x1) -1j*sqrt(k0^2 - x1.^2);
%         signkz1 = @(x1) (angle(kz1(x1))>0 | angle(kz1(x1))<=-pi);
%         kz1 = @(x1) -signkz1(x1).*kz1(x1);

%         J0_tmp = besselj(0,x0.*p(ik));
%         J1_tmp = besselj(1,x1.*p(ik));
        
        % Sommerfeld identity
        F7tmp = @(x0) (exp(-kz0(x0)*ww)./kz0(x0).*x0.*besselj(0,x0.*p(ik)));
        F7new = integral(F7tmp,kp_J0(ig),kp_J0(ig+1))*dkp_J0;
%         F7new = quadgk(kz0,kp_J0(ig),kp_J0(ig+1)).*dkp_J0;
        
        F8tmp = @(x1) (exp(-kz1(x1)*ww)./kz1(x1).*x1.^2.*besselj(1,x1.*p(ik)));
        F8new = integral(F8tmp,kp_J1(ig),kp_J1(ig+1))*dkp_J1;
%         F8new = quadgk(kz1,kp_J1(ig),kp_J1(ig+1));
            
        if ((abs(F7new)<(abs(F7(ik))*err_tol)) || F7new==0) && ...
                ((abs(F8new)<(abs(F8(ik))*err_tol)) || F8new==0)
            disp(['Converge step : ',num2str(ig)])
            break;
        else
            F7(ik) = F7(ik)+F7new;
            F8(ik) = F7(ik)+F8new;
        end
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

