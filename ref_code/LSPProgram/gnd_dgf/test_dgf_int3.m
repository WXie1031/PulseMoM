
err_tol = 1e-10;

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

% kp_max = 1.2*k0;
% kp_max = 1e5;

Nkp = 1e4;
J0_zero = J0_zero(1:Nkp);
J1_zero = J1_zero(1:Nkp);


kp_im = zeros(1,Nkp);
% Elliptical integration path
kp_max = min(max( 1.2*k0 ),max(abs(k0))+k0*0.2);
a = kp_max/2;
b = 1e-3*a;




p = xf./k0;
% p = xf;
Nf = length(p);
aa = (1+1j)/sqrt(2);
ww = 1;
F1=zeros(1,Nf);F2=zeros(1,Nf);F3=zeros(1,Nf);F4=zeros(1,Nf);F5=zeros(1,Nf);
F6=zeros(1,Nf);F7=zeros(1,Nf);F8=zeros(1,Nf);F9=zeros(1,Nf);F10=zeros(1,Nf);

for ik = 1:Nf
    ik
    kp_J0 = J0_zero/p(ik);
    kp_J1 = J1_zero/p(ik);
    
    Nim_J0 = sum(kp_J0<min(kp_max,kp_max))+1;
    kp_im = b .* sin(pi*(0:Nim_J0-1)/Nim_J0);
    kp_J0(1:Nim_J0) = kp_J0(1:Nim_J0)+1j*kp_im;
    
    Nim_J1 = sum(kp_J1<min(kp_max,kp_max))+1;
    kp_im = b .* sin(pi*(0:Nim_J1-1)/Nim_J1);
    kp_J1(1:Nim_J1) = kp_J1(1:Nim_J1)+1j*kp_im;
    
    
    for ig = 1:Nkp-1
        
        
        Nint_J0 = min(max(ceil(abs(kp_J0(ig+1)-kp_J0(ig))/dkp),3),127);
        Nint_J1 = min(max(ceil(abs(kp_J1(ig+1)-kp_J1(ig))/dkp),3),127);
        dkp_J0 = kp_J0(ig+1)-kp_J0(ig);
        dkp_J1 = kp_J1(ig+1)-kp_J1(ig);
        
        [gau_nod_J0, gau_wei_J0] = gauss_int_coef(Nint_J0);
        [gau_nod_J1, gau_wei_J1] = gauss_int_coef(Nint_J1);
        
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
        
        % rapidly convergent forms
        F1(ik) = F1(ik) + sum( kp_J0_int.*exp(-aa*kp_J0_int.^2).*J0_tmp.*gau_wei_J0 ,2)*dkp_J0;
        F2(ik) = F2(ik) + sum( exp(-kp_J1_int).*J1_tmp.*gau_wei_J1 ,2)*dkp_J1;
        % slowly convergent forms
        F3(ik) = F3(ik) + sum( J0_tmp.*gau_wei_J0 ,2)*dkp_J0;
        F4(ik) = F4(ik) + sum( kp_J0_int./sqrt(kp_J0_int.^2+aa.*aa).*J0_tmp.*gau_wei_J0 ,2)*dkp_J0;
        % algebracically divergent forms
        F5(ik) = F5(ik) + sum( (kp_J0_int.*J0_tmp) .*gau_wei_J0 ,2)*dkp_J0;
        F6(ik) = F6(ik) + sum( (kp_J0_int.*sqrt(kp_J0_int.^2+aa.*aa).*J0_tmp) .*gau_wei_J0 ,2)*dkp_J0;
        % oscillatory kernel forms
        F9(ik) = F9(ik) + sum( (cos(kp_J1_int).*J1_tmp) .*gau_wei_J1 ,2)*dkp_J1;
        F10(ik) = F10(ik) + sum( (cos(kp_J1_int)./kp_J1_int.*J1_tmp) .*gau_wei_J1 ,2)*dkp_J1;
        
        % Sommerfeld identity
        F7tmp = sum( exp(-kz_int_J0*ww)./kz_int_J0.*kp_J0_int.*J0_tmp.*gau_wei_J0 ,2)*dkp_J0;
        F7(ik) = F7(ik) + F7tmp;
        F8tmp = sum( exp(-kz_int_J1*ww)./kz_int_J1.*kp_J1_int.^2.*J1_tmp.*gau_wei_J1 ,2)*dkp_J1;
        F8(ik) = F8(ik) + F8tmp;

        
    end
end

p2 = p.^2;
rr = sqrt(p2+ww.^2);



F11 = exp(-p2./(4.*aa))./(2*aa);
F22 = (sqrt(p2+1)-1)./(p.*sqrt(p2+1));
F33 = 1./p;
F44 = exp(-aa.*p)./p;
F55 = zeros(Nf,1);
F66 = exp(-aa.*p)./(p.^3).*(aa.*p+1);

F99 = zeros(Nf,1);
id1 = p>=1;
id0 = p<1;
F99(id0) = 1./p(id0);
F99(id1) = (sqrt(1-p2(id1))-1)./(p(id1).*sqrt(1-p2(id1)));

F100 = zeros(Nf,1);
id1 = p>1;
id0 = p<=1;
F100(id0) = sqrt(p2(id0)-1)./p(id0);
% F100(id1) =

F77 = exp(1j*k0*rr)./rr;
F88 = exp(1j*k0*rr)./(rr.^3).*(1-1j*k0*rr).*p;
figure(10)
semilogx(xf,abs(F1),'r');
hold on
loglog(xf,abs(F11),'k');
legend('numerical','analytical')

figure(11)
loglog(xf,abs(F2),'r');
hold on
plot(xf,abs(F22),'k');
legend('numerical','analytical')

figure(12)
loglog(xf,abs(F3),'r');
hold on
loglog(xf,abs(F33),'k');
legend('numerical','analytical')

figure(13)
loglog(xf,abs(F4),'r');
hold on
loglog(xf,abs(F44),'k');
legend('numerical','analytical')

figure(14)
plot(xf,abs(F5),'r');
hold on
plot(xf,abs(F55),'k');
legend('numerical','analytical')

figure(15)
loglog(xf,abs(F6),'r');
hold on
loglog(xf,abs(F66),'k');
legend('numerical','analytical')

figure(16)
loglog(xf,abs(F7),'r');
hold on
loglog(xf,abs(F77),'k');
legend('numerical','analytical')

figure(17)
loglog(xf,abs(F8),'r');
hold on
loglog(xf,abs(F88),'k');
legend('numerical','analytical')

figure(18)
loglog(xf,abs(F9),'r');
hold on
loglog(xf,abs(F99),'k');
legend('numerical','analytical')

figure(19)
loglog(xf,abs(F10),'r');
hold on
loglog(xf,abs(F100),'k');
legend('numerical','analytical')
