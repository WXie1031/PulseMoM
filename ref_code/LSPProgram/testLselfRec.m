
%clear

re = 10.8;
%mur = 20;
Rsurge = 7.65e-3;
fmur = 10e3;


frq = [1 20 50 100 200 500 1e3 2e3  5e3  10e3 20e3 50e3 100e3  200e3 500e3 1e6];
%frq = [1e6 2e6 5e6 10e6 100e6];
%f0 = [ 1 5 (10:10:100) (150:50:500) 750 (1e3:1e3:5e3) (10e3:10e3:100e3) 200e3 300e3 500e3 ];

Nf = length(frq);

Nfit=2;

w0 = 2*pi*frq;

shape_cs2D = [1002;];
pt_cs2D = [0  0 ]*1e-3;
dim1_cs2D = [40;]*1e-3;
dim2_cs2D = [4;]*1e-3;
re_cs2D = [10.8]*1e-3;
Rin_pul_cs2D = [1.28]*1e-3;

Lin_pul_cs2D = zeros(length(shape_cs2D),1);
len_cs2D = 5*ones(length(shape_cs2D),1);
len_cs2D = 1.65;
%Sig_pul_cs2D = 1./(Rin_pul_cs2D.*dim1_cs2D.*dim2_cs2D);
Sig_pul_cs2D = 4.8828e+06;
% Sig_pul_cs2D = 59.6e4;
Rin_pul_cs2D = 1./(Sig_pul_cs2D.* (dim1_cs2D.*dim2_cs2D));

murtmp = mur_fit_rec(dim1_cs2D, dim2_cs2D, Rin_pul_cs2D, Sig_pul_cs2D, Rsurge, fmur);
 %murtmp = 20;
%  murtmp=1;

% mur(1) = max(0.1*murtmp,1);
% mur(2) = max(0.2*murtmp,1);
% mur(3) = max(0.5*murtmp,1);
% mur(4) = max(0.5*murtmp,1);
% mur(5) = max(0.5*murtmp,1);
% mur(6) = murtmp;
% mur(7) = 1.5*murtmp;
% mur(8) = 2*murtmp;
% mur(9) = 1.5*murtmp;

mur = ones(1,Nf)*murtmp;

mur = ones(1,Nf);

% mutual inductance of the square loop
% Lm = -3.7376*1e-7*len_cs2D/4;
%  Lm = -induct_cir_ext(2, len_cs2D);
Lm = 0;

%mur=1;
% [~,~,Rmesh1, Lmesh1] = mesh2d_main_partial( ...
%     shape_cs2D, pt_cs2D, dim1_cs2D, dim2_cs2D, Rin_pul_cs2D, Lin_pul_cs2D,Sig_pul_cs2D, mur, len_cs2D, frq);

% [~,~,Rmesh1, Lmesh1] = mesh2d_main_partial_pul( ...
%     shape_cs2D, pt_cs2D, dim1_cs2D, dim2_cs2D, Rin_pul_cs2D, Lin_pul_cs2D,Sig_pul_cs2D, mur, fplt);


[~,~,Rmesh1, Lmesh1] = main_mesh2d_cmplt( ...
   shape_cs2D, pt_cs2D, dim1_cs2D, dim2_cs2D, Rin_pul_cs2D, Sig_pul_cs2D,mur, len_cs2D, frq);
%Lmesh1=Lmesh1+Lm;
% [~,~,Rmesh1, Lmesh1] = mesh2d_main_complete_pul( ...
%     shape_cs2D, pt_cs2D, dim1_cs2D, dim2_cs2D, Rin_pul_cs2D, Lin_pul_cs2D,mur, fplt);


Rs1 = zeros(1,Nf);
Ls1 = zeros(1,Nf);

[ Rs1, Ls1] = para_self_multi_frq(shape_cs2D, dim1_cs2D, dim2_cs2D, ...
    len_cs2D, Rin_pul_cs2D, Sig_pul_cs2D, 1, frq);
Ls1=Ls1+Lm;
% for ig = 1:Nf
%     Rs1(ig) = resis_bar_ac(dim1_cs2D, dim2_cs2D, Rin_pul_cs2D, Sig_pul_cs2D, 1, f0(ig));
%     Ls1(ig) = induct_bar_ac(dim1_cs2D, dim2_cs2D, Sig_pul_cs2D, 1, f0(ig));
% end

% Rmag1 = resis_add_mag(Rs1, mur, fbr, f0)*len_cs2D;
% Rs1 = Rmag1;
% Rs1 = Rs1*len_cs2D;
% Ls1 = Ls1*len_cs2D;
% Rs1 = Rs1-Rin_pul_cs2D.*len_cs2D;
% L0 = Ls1(1);
% Ls1 = -Ls1+L0;

% fi = [ 1 5 (10:10:100) (150:50:500) (1e3:1e3:5e3) (10e3:10e3:100e3) 1e6 2e6];
% fi = fi(fi<f0(end));
% Nfi = length(fi);
% Rsi = zeros(1,Nfi);
% Lsi = zeros(1,Nfi);
%
% for ig = 1:1
%     Rsi(ig,:) = interp1(f0,squeeze(Rs1(ig,:)),fi,'linear');%spline linear
%     Lsi(ig,:) = interp1(f0,squeeze(Ls1(ig,:)),fi,'linear');
% end

wplt = 2*pi*frq;
ffit = [1:10:max(frq)];



% Rtmp = interp1(f0, squeeze(Rs1(1,1:Nf)), ffit,'linear');
% Ltmp = interp1(f0, squeeze(Ls1(1,1:Nf)), ffit,'linear');
[Rdc, L0, Rn, Ln, Zfit] = main_vectfit_z( Rs1(1,1:Nf),  Ls1(1,1:Nf), frq, Nfit,0);
%[Rdc, L0, Rn, Ln] = vectfit_main_Z( Rtmp, Ltmp, ffit, Nfit);


%     Rdc = Rin_pul_cs2D.*len_cs2D;
Rfcur = zeros(1,Nf);
Lfcur = zeros(1,Nf);
for ik = 1:Nf
    Rfcur(ik) = Rdc+sum( (wplt(ik).^2.*Rn(:,1:Nfit).*Ln(:,1:Nfit).^2)./(Rn(:,1:Nfit).^2+wplt(ik).^2.*Ln(:,1:Nfit).^2));
    Lfcur(ik) = L0+sum( (Rn(:,1:Nfit).^2.*Ln(:,1:Nfit))./(Rn(:,1:Nfit).^2+wplt(ik).^2.*Ln(:,1:Nfit).^2));
end

[R0imag, L0imag, R1imag, C1imag] = main_vectfit_zin(Rs1(1,1:Nf), Ls1(1,1:Nf), frq, Nfit);
Rimag = zeros(1,Nf);
Limag = zeros(1,Nf);
for ik = 1:Nf
    Rimag(ik) = Rfcur(ik)-Rdc+R0imag+sum( (R1imag(:,1:Nfit))./(1+wplt(ik).^2.*R1imag(:,1:Nfit).^2.*C1imag(:,1:Nfit).^2) );
    Limag(ik) = Lfcur(ik)+L0imag-sum( (R1imag(:,1:Nfit).^2.*C1imag(:,1:Nfit))./(1+wplt(ik).^2.*R1imag(:,1:Nfit).^2.*C1imag(:,1:Nfit).^2));
    
end
% Zs = zeros(1,1,Nf);
% Zs(1,1,:) = Rmag1(1,:);
% 
% [R0imag, L0imag, R1imag, C1imag] = vecfit_main_Zin(Zs, f0, 2);
% [R0zmag, L0zmag, R1zmag, L1zmag] = vecfit_main_Z(Zs, f0, 2);
% Rzdc = R0zmag-sum(R1zmag);
% 
% Rimag = zeros(1,Nf);
% Limag = zeros(1,Nf);
% Rzmag = zeros(1,Nf);
% Lzmag = zeros(1,Nf);
% for ik = 1:Nf
%     Rimag(ik) = Rfcur(ik)-Rdc+R0imag+sum( (R1imag)./(1+w0(ik).^2.*R1imag.^2.*C1imag.^2) );
%     Limag(ik) = Lfcur(ik)+L0imag-sum( (R1imag.^2.*C1imag)./(1+w0(ik).^2.*R1imag.^2.*C1imag.^2));
%     
%     Rzmag(ik) = Rfcur(ik)-Rdc+Rzdc+sum( (w0(ik).^2.*R1zmag.*L1zmag.^2)./(R1zmag.^2+w0(ik).^2.*L1zmag.^2));
%     Lzmag(ik) = Lfcur(ik)+L0zmag+sum( (R1zmag.^2.*L1zmag)./(R1zmag.^2+w0(ik).^2.*L1zmag.^2));
% end


figure(4);
semilogx(frq/1e3,Rmesh1*1e3,'k-o','LineWidth',1);
hold on
semilogx(frq/1e3,(Rs1)*1e3,'k--v','LineWidth',1);
% semilogx(f0,Rmag1*1e3);
% semilogx(frq/1e3,(Rimag)*1e3);
% semilogx(f0,(Rzmag)*1e3);
% semilogx(frq/1e3,Rfcur*1e3/len_cs2D,'k.-');
xlabel('Frequency(Hz)')
ylabel('Resistance(m\Omega)')
hold off
legend('Proposed','Analytic')

figure(5);
semilogx(frq/1e3,Lmesh1*1e6,'k-o','LineWidth',1);
hold on
semilogx(frq/1e3,(Ls1)*1e6,'k--v','LineWidth',1);
% semilogx(frq/1e3,(Limag)*1e6);
% semilogx(f0,(Lzmag)*1e6);
% semilogx(frq/1e3,Lfcur*1e6,'k.-');
xlabel('Frequency(Hz)')
ylabel('Inductance(uH)')
hold off
legend('Proposed','Analytic')




% figure(6);
% 
% hold on
% err_r = Rs1./Rmesh1;
% err_analy = sqrt(mur);
% 
% semilogx(frq/1e3,err_r,'--');
% semilogx(frq/1e3,err_analy);
% 
% xlabel('f(kHz)');
% grid on
% hold off

% semilogx(frq/1e3,(Rfcur + 1j*2*pi*frq.*Lfcur)/len_cs2D,'k.-');

% figure(5);
% hold on
% 
% plotyy(fplt/1e3,(Rs1)*1e3,fplt/1e3,(Ls1)*1e6);
% 
% xlabel('f(kHz)');
% hold off
% grid on

