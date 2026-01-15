
mu0 = 4e-7*pi;
ep0 = 8.854187817e-12;
vc = 3e8;

%% configuration of the multi-layer structure
w0= 2*pi*30e9;
k0 = w0*sqrt(ep0.*mu0);

Nint = 15;  % order of the Guassion integration
[gau_nod, gau_wei] = gauss_int_coef(Nint);

T = textread('GA.txt');
xf = T(:,1);

%% kp setting and integration path
% ppw = 40;
ppw = 20;
lumda = 2*pi*vc/w0;
dkp = pi/(lumda*ppw);

kp_end = 1e4;

kp = 0:dkp:kp_end;
Nkp = length(kp);

kp_im = zeros(1,Nkp);
% Elliptical integration path
kp_max = min(max( 1.2*k0 ),max(abs(k0))+k0*0.2);
Nkp_imp = size(1:dkp:min(kp_end,kp_max),2);
a = kp_max/2;
b = 1e-3*a;
%kp_im(1:Nkp_imp) = b .* sqrt(1-((1:dkp:min(kp_end,kp_max))-kp_max/2).^2./a.^2);
kp_im(1:Nkp_imp) = b .* sin(pi*(0:Nkp_imp-1)/Nkp_imp);
kp = kp+1j*kp_im;

Nkp_tot = (Nkp-1)*Nint;
kp_tot = zeros(1,Nkp_tot);
% calculate the kz
for ig = 1:Nkp-1
    dkp_int = (kp(ig+1)-kp(ig))/2*gau_nod;
    kp_tot((ig-1)*Nint+(1:Nint)) = dkp_int + (kp(ig)+kp(ig+1))/2;
end
kz_tot = sqrt(k0.^2 - kp_tot.^2);
id_kz = angle(kz_tot)>0 | angle(kz_tot)<=-pi;
kz_tot(id_kz) = -kz_tot(id_kz);


p = xf./k0;
% p = xf;
Nf = length(p);
a = (1+1j)/sqrt(2);
b = 1;
F1=zeros(1,Nf);F2=zeros(1,Nf);F3=zeros(1,Nf);F4=zeros(1,Nf);F5=zeros(1,Nf);
F6=zeros(1,Nf);F7=zeros(1,Nf);F8=zeros(1,Nf);F9=zeros(1,Nf);F10=zeros(1,Nf);

for ik = 1:Nf
    ptmp = p(ik);
for ig = 1:Nkp-1


    kztmp = @(kpx) sqrt(k0.^2 - kpx.^2);
    
    J0_tmp = @(kpx) besselj(0,kpx*ptmp);
    J1_tmp = @(kpx) besselj(1,kpx*ptmp);
    
    dkp = kp(ig+1)-kp(ig);
    
    % rapidly convergent forms
    F1tmp = @(kpx) kpx.*exp(-a*kpx.^2).*J0_tmp(kpx);
    F1(ik) = F1(ik) + quadgk(F1tmp,kp(ig),kp(ig+1))*dkp;
    
    F2tmp = @(kpx) exp(-kpx).*J1_tmp(kpx);
    F2(ik) = F2(ik) + quadgk(F2tmp,kp(ig),kp(ig+1))*dkp;
    
    % slowly convergent forms
    F3tmp = @(kpx) J0_tmp(kpx);
    F3(ik) = F3(ik) + quadgk(F3tmp,kp(ig),kp(ig+1))*dkp;
    
    F4tmp = @(kpx) kpx./sqrt(kpx.^2+a.*a).*J0_tmp(kpx);
    F4(ik) = F4(ik) + quadgk(F4tmp,kp(ig),kp(ig+1))*dkp;
    
    % algebracically divergent forms
    F5tmp = @(kpx) kpx.*J0_tmp(kpx);
    F5(ik) = F5(ik) + quadgk(F5tmp,kp(ig),kp(ig+1))*dkp;
    
    F6tmp = @(kpx) (kpx.*sqrt(kpx.^2+a.*a).*J0_tmp(kpx)); 
    F6(ik) = F6(ik) + quadgk(F6tmp,kp(ig),kp(ig+1))*dkp;
    
    % oscillatory kernel forms
    F9tmp = @(kpx) (cos(kpx).*J1_tmp(kpx));
    F9(ik) = F9(ik) + quadgk(F9tmp,kp(ig),kp(ig+1))*dkp;
    
    F10tmp = @(kpx) (cos(kpx)./kpx.*J1_tmp(kpx));
    F10(ik) = F10(ik) + quadgk(F10tmp,kp(ig),kp(ig+1))*dkp;
    
    % Sommerfeld identity
    F7tmp = @(kpx) exp(-kztmp(kpx)*b)./kztmp(kpx).*kpx.*J0_tmp(kpx);
    F7(ik) = F7(ik) + quadgk(F7tmp,kp(ig),kp(ig+1))*dkp;
    
    F8tmp = @(kpx) exp(-kztmp(kpx)*b)./kztmp(kpx).*kpx.^2.*J1_tmp(kpx);
    F8(ik) = F8(ik) + quadgk(F8tmp,kp(ig),kp(ig+1))*dkp;
end
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
