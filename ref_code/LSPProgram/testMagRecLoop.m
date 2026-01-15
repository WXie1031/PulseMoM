
%clear
addpath('./vectorfit');


re = 0.0116;
%mur = 20;
Rsurge = 38.8e-3;
fmur = 10e3;

fplt = [1 50 100 200 500 1e3 2e3  5e3  10e3 20e3 50e3 100e3 200e3];%];% 


Nplt = length(fplt);

Nfit=2;

w0 = 2*pi*fplt;

shape_cs2D = [1002;];
pt_cs2D = [0  0 ]*1e-3;
dim1_cs2D = [40;]*1e-3;
dim2_cs2D = [4;]*1e-3;

Rin_pul_cs2D = [10.14]*1e-3;

Lin_pul_cs2D = zeros(length(shape_cs2D),1);
len_cs2D = 1.3*4*ones(length(shape_cs2D),1);

%Sig_pul_cs2D = 1./(Rin_pul_cs2D.*dim1_cs2D.*dim2_cs2D);
Sig_pul_cs2D = 4.8828e+06;
Rin_pul_cs2D = 1./(Sig_pul_cs2D.* (dim1_cs2D.*dim2_cs2D));

murtmp = mur_fit_rec(dim1_cs2D, dim2_cs2D, Rin_pul_cs2D, Sig_pul_cs2D, Rsurge, fmur);
%murtmp = 1;

mur = ones(1,Nplt)*murtmp;



% mutual inductance of the square loop
Lm = induct_gmd(1.3,len_cs2D);
Lm=Lm*4;

%mur=1;
% [~,~,Rmesh1, Lmesh1] = mesh2d_main_partial( ...
%     shape_cs2D, pt_cs2D, dim1_cs2D, dim2_cs2D, Rin_pul_cs2D, Lin_pul_cs2D,Sig_pul_cs2D, mur, len_cs2D, fplt);

% [~,~,Rmesh1, Lmesh1] = mesh2d_main_partial_pul( ...
%     shape_cs2D, pt_cs2D, dim1_cs2D, dim2_cs2D, Rin_pul_cs2D, Lin_pul_cs2D,Sig_pul_cs2D, mur, fplt);


[~,~,Rmesh1, Lmesh1] = mesh2d_main_complete( ...
    shape_cs2D, pt_cs2D, dim1_cs2D, dim2_cs2D, Rin_pul_cs2D, Lin_pul_cs2D,mur, len_cs2D, fplt);
Lmesh1=(Lmesh1-Lm);
Rmesh1=Rmesh1;
% [~,~,Rmesh1, Lmesh1] = mesh2d_main_complete_pul( ...
%     shape_cs2D, pt_cs2D, dim1_cs2D, dim2_cs2D, Rin_pul_cs2D, Lin_pul_cs2D,mur, fplt);


Rs1 = zeros(1,Nplt);
Ls1 = zeros(1,Nplt);

[ Rs1, Ls1] = para_main_self_multi_frq(shape_cs2D, dim1_cs2D, dim2_cs2D, ...
    len_cs2D, Rin_pul_cs2D, Sig_pul_cs2D, mur, fplt);
Ls1=(Ls1-Lm);
Rs1=Rs1;

wplt = 2*pi*fplt;
ffit = [1:10:max(fplt)];

Rtmp = interp1(fplt, Rs1(1,1:Nplt), ffit,'linear');
Ltmp = interp1(fplt, Ls1(1,1:Nplt), ffit,'linear');
%[Rdc, L0, Rn, Ln] = vectfit_main_Z( Rs1(1,1:Nf),  Ls1(1,1:Nf), f0, Nfit);
[Rdc, L0, Rn, Ln] = vectfit_main_Z( Rtmp, Ltmp, ffit, Nfit);




%     Rdc = Rin_pul_cs2D.*len_cs2D;
Rfcur = zeros(1,Nplt);
Lfcur = zeros(1,Nplt);
for ik = 1:Nplt
    Rfcur(ik) = Rdc+sum( (wplt(ik).^2.*Rn(:,1:Nfit).*Ln(:,1:Nfit).^2)./(Rn(:,1:Nfit).^2+wplt(ik).^2.*Ln(:,1:Nfit).^2));
    Lfcur(ik) = L0+sum( (Rn(:,1:Nfit).^2.*Ln(:,1:Nfit))./(Rn(:,1:Nfit).^2+wplt(ik).^2.*Ln(:,1:Nfit).^2));
end

[R0imag, L0imag, R1imag, C1imag] = vectfit_main_Zin(Rs1(1,1:Nplt), Ls1(1,1:Nplt), fplt, Nfit);
Rimag = zeros(1,Nplt);
Limag = zeros(1,Nplt);
for ik = 1:Nplt
    Rimag(ik) = Rfcur(ik)-Rdc+R0imag+sum( (R1imag(:,1:Nfit))./(1+wplt(ik).^2.*R1imag(:,1:Nfit).^2.*C1imag(:,1:Nfit).^2) );
    Limag(ik) = Lfcur(ik)+L0imag-sum( (R1imag(:,1:Nfit).^2.*C1imag(:,1:Nfit))./(1+wplt(ik).^2.*R1imag(:,1:Nfit).^2.*C1imag(:,1:Nfit).^2));
    
end



figure(3);
hold on
semilogx(fplt/1e3,Rmesh1*1e3/len_cs2D,'displayname','Segment Model');
semilogx(fplt/1e3,(Rs1)*1e3/len_cs2D);
% semilogx(fplt/1e3,Rfcur*1e3/len_cs2D,'k.-');
xlabel('f(kHz)');
grid on
hold off

figure(4);
hold on
semilogx(fplt/1e3,Lmesh1*1e6/len_cs2D,'displayname','Segment Model');
semilogx(fplt/1e3,(Ls1)*1e6/len_cs2D);
% semilogx(fplt/1e3,Lfcur*1e6/len_cs2D,'k.-');
xlabel('f(kHz)');
hold off
grid on



% figure(5);
% hold on
% 
% plotyy(fplt/1e3,(Rs1)*1e3,fplt/1e3,(Ls1)*1e6);
% 
% xlabel('f(kHz)');
% hold off
% grid on

