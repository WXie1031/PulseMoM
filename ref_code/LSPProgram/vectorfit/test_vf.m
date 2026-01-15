
Nc = 1;

Nfit = 9;


f0 = [1 50  100  200  500  1e3  2e3  5e3  10e3  50e3  100e3 500e3 1e6 ...
    2e6 5e6 10e6 20e6 50e6 100e6 200e6 500e6 1000e6];

dim1=40e-3;
dim2=4e-3;
R_pul=1.28e-3;
sig = 4.8828e+06;


Nf = length(f0);
Rs = zeros(Nc,Nf);
Lin = zeros(Nc,Nf);
Ls = zeros(Nc,Nf);

for ik = 1:Nc
    for ig = 1:Nf
        Rs(ik,ig) = resis_bar_ac(dim1(ik), dim2(ik), R_pul(ik), sig(ik), 1, 1, f0(ig));
        [Ls(ik,ig),Lin(ik,ig),Lext] = induct_bar_ac(dim1(ik), dim2(ik), sig(ik), 1,1, f0(ig));
    end
end

Zs = Rs+1j*2*pi*f0.*Ls;

[R02, L02, Rn2, Ln2, Zv2] = main_vectfit_z(Rs,Ls,f0,Nfit, Lext);


%[R03, C03, Rn3, Ln3, Yv3] = vectfit_main_Y(Rs,Ls,f0,Nfit);

% Rmf = zeros(1,1,Nf);
% Lmf = Rmf;
% Rmf(1,1,:)=Rs(1:Nf);
% Lmf(1:1,:)=Ls(1:Nf);
% L0min = max(max( abs(Lmtx-diag(diag(Lmtx))) ))
% [Rm0fit, Lm0fit, Rmvfit, Lmvfit, Zmfit] = main_vectfit_Zmtx(...
%     Rmf, Lmf, f0, Nfit, Lmtx);

figure;
hold on;
plot(f0,abs(Zs),'k')
% plot(f0,abs(Zv1),'b')
plot(f0,abs(Zv2),'r')
%plot(frq,squeeze(abs(1./Yv3)),'g')

figure;
hold on;
plot(f0,(Rs),'k')
% plot(f0,real(Zv1),'b')
plot(f0,real(Zv2),'r')

figure;
hold on;
plot(f0,(Ls),'k')
% plot(f0,imag(Zv1)./f0/2/pi,'b')
plot(f0,imag(Zv2)./f0/2/pi,'r')

