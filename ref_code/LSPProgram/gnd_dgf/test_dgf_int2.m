
mu0 = 4e-7*pi;
ep0 = 8.854187817e-12;
vc = 3e8;

%% configuration of the multi-layer structure
w0= 2*pi*30e9;
k0 = w0*sqrt(ep0.*mu0);

Nint = 512*8;  % order of the Guassion integration
[gau_nod, gau_wei] = gauss_int_coef(Nint);

T = textread('GA.txt');
xf = T(:,1);




Nkp_imp = Nint;
kp_im = zeros(1,Nkp_imp);
% Elliptical integration path
kp_max = min(max( 1.2*k0 ),max(abs(k0))+k0*0.2);
% Nkp_imp = size(1:dkp:min(kp_end,kp_max),2);

kp1 = linspace(0,kp_max,Nint);
a = kp_max/2;
b = 1e-3*a;
%kp_im(1:Nkp_imp) = b .* sqrt(1-((1:dkp:min(kp_end,kp_max))-kp_max/2).^2./a.^2);
kp_im(1:Nkp_imp) = b .* sin(pi*(0:Nkp_imp-1)/Nkp_imp);
kp1 = kp1+1j*kp_im;

kz1 = sqrt(k0.^2 - kp1.^2);
id_kz = angle(kz1)>0 | angle(kz1)<=-pi;
kz1(id_kz) = -kz1(id_kz);


p = xf./k0;
% p = xf;
Nf = length(p);
a = (1+1j)/sqrt(2);
b = 1;
F1=0;F2=0;F3=0;F4=0;F5=0;F6=0;F7=0;F8=0;F9=0;F10=0;
%% part 1:
ptmp = repmat(p,1,Nint);
kptmp = repmat(kp1,Nf,1);
kztmp = repmat(kz1,Nf,1);
weitmp = repmat(gau_wei,Nf,1);

J0_tmp = besselj(0,kptmp.*ptmp);
J1_tmp = besselj(1,kptmp.*ptmp);

dkp1 = repmat([kp1(2:end)-kp1(1:end-1),kp_max-kp1(end)],Nf,1);

% rapidly convergent forms
F1 = F1 + sum( kptmp.*exp(-a*kptmp.^2).*J0_tmp.*dkp1.*weitmp ,2);
F2 = F2 + sum( exp(-kptmp).*J1_tmp.*dkp1.*weitmp ,2);
% slowly convergent forms
F3 = F3 + sum( J0_tmp.*dkp1.*weitmp ,2);
F4 = F4 + sum( kptmp./sqrt(kptmp.^2+a.*a).*J0_tmp.*dkp1.*weitmp ,2);
% algebracically divergent forms
F5 = F5 + sum( (kptmp.*J0_tmp).*dkp1 .*weitmp ,2);
F6 = F6 + sum( (kptmp.*sqrt(kptmp.^2+a.*a).*J0_tmp).*dkp1 .*weitmp ,2);
% oscillatory kernel forms
F9 = F9 + sum( (cos(kptmp).*J1_tmp).*dkp1 .*weitmp ,2);
F10 = F10 + sum( (cos(kptmp)./kptmp.*J1_tmp).*dkp1 .*weitmp ,2);

% Sommerfeld identity
F7 = F7 + sum( exp(-kztmp*b)./kztmp.*kptmp.*J0_tmp.*dkp1.*weitmp ,2);
F8 = F8 + sum( exp(-kztmp*b)./kztmp.*kptmp.^2.*J1_tmp.*dkp1.*weitmp ,2);



%% part 2:
for ig = 1:Nf

    
    kp2 = linspace(kp_max,400/p(ig),Nint);
    
    J0_tmp = besselj(0,kp2.*p(ig));
    J1_tmp = besselj(1,kp2.*p(ig));
    
    dkp2 = 400/p(ig)-kp_max;
    dkp2 = [kp2(2)-kp2(1)]*ones(1,Nint);
    
    kz_t2 = sqrt(k0.^2 - kp2.^2);
    id_kz = angle(kz_t2)>0 | angle(kz_t2)<=-pi;
    kz_t2(id_kz) = -kz_t2(id_kz);
    

    % rapidly convergent forms
    F1(ig) = F1(ig) + sum( kp2.*exp(-a*kp2.^2).*J0_tmp.*dkp2.*gau_wei ,2);
    F2(ig) = F2(ig) + sum( exp(-kp2).*J1_tmp.*dkp2.*gau_wei ,2);
    % slowly convergent forms
    F3(ig) = F3(ig) + sum( J0_tmp.*dkp2.*gau_wei ,2);
    F4(ig) = F4(ig) + sum( kp2./sqrt(kp2.^2+a.*a).*J0_tmp.*dkp2.*gau_wei ,2);
    % algebracically divergent forms
    F5(ig) = F5(ig) + sum( (kp2.*J0_tmp).*dkp2 .*gau_wei ,2);
    F6(ig) = F6(ig) + sum( (kp2.*sqrt(kp2.^2+a.*a).*J0_tmp).*dkp2 .*gau_wei ,2);
    % oscillatory kernel forms
    F9(ig) = F9(ig) + sum( (cos(kp2).*J1_tmp).*dkp2 .*gau_wei ,2);
    F10(ig) = F10(ig) + sum( (cos(kp2)./kp2.*J1_tmp).*dkp2 .*gau_wei ,2);
    
    % Sommerfeld identity
    F7(ig) = F7(ig) + sum( exp(-kz_t2*b)./kz_t2.*kp2.*J0_tmp.*dkp2.*gau_wei ,2);
    F8(ig) = F8(ig) + sum( exp(-kz_t2*b)./kz_t2.*kp2.^2.*J1_tmp.*dkp2.*gau_wei ,2);
end

p2 = p.^2;
rr = sqrt(p2+b.^2);



F11 = exp(-p2./(4.*a))./(2*a);
F22 = (sqrt(p2+1)-1)./(p.*sqrt(p2+1));
F33 = 1./p;
F44 = exp(-a.*p)./p;
F55 = zeros(Nf,1);
F66 = exp(-a.*p)./(p.^3).*(a.*p+1);

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
