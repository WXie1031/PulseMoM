
mu0 = 4e-7*pi;
ep0 = 8.854187817e-12;
vc = 3e8;

%% configuration of the multi-layer structure
w0= 2*pi*30e9;
k0 = w0*sqrt(ep0.*mu0);

Nint = 32;  % order of the Guassion integration
[gau_nod, gau_wei] = gauss_int_coef(Nint);

T = textread('GA.txt');
xf = T(:,1);

%% kp setting and integration path
% ppw = 40;
ppw = 20;
lumda = 2*pi*vc/w0;
dkp = pi/(lumda*ppw);

kp_end = 1e6;

kp = 0:dkp:kp_end;
Nkp = length(kp);

kp_im = zeros(1,Nkp);
% Elliptical integration path
id_kn = sig_lyr<1e4;
kp_max = max( 1.2*k0 );
%kp_max = max(abs(kn(id_kn)))+k0*0.2 ;
Nkp_imp = size(1:dkp:min(kp_end,kp_max),2);
a = kp_max/2;
b = 10e-3*a;
kp_im(1:Nkp_imp) = b .* sqrt(1-((1:dkp:min(kp_end,kp_max))-kp_max/2).^2./a.^2);
%kp_im(1:Nkp_imp) = b .* sin(pi*(1:Nkp_imp)/Nkp_imp);
kp = kp+1j*kp_im;


Nkp_tot = (Nkp-1)*Nint;
kp_tot = zeros(1,Nkp_tot);
% calculate the kz
for ig = 1:Nkp-1
    dkp_int = (kp(ig+1)-kp(ig))/2*gau_nod;
    kp_tot((ig-1)*Nint+(1:Nint)) = dkp_int + (kp(ig)+kp(ig+1))/2;
end

p = 1e-3;
% p = 1;

J0 = zeros(1,Nkp-1);
J1 = zeros(1,Nkp-1);
for ig = 1:Nkp-1
    ind_int = (ig-1)*Nint+(1:Nint);
    
    kp_int = kp_tot(ind_int);
    
    
    J0_tmp = besselj(0,kp_int.*p);
    J1_tmp = besselj(1,kp_int.*p);
    
    J0(ig) = sum(J0_tmp.*gau_wei ,2);
    J1(ig) = sum(J1_tmp.*gau_wei ,2);
    
end

figure(10)
hold on
plot(kp(1:end-1),J0);
plot(kp(1:end-1),J1);



