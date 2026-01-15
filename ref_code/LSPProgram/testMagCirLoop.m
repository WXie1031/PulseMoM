
%clear
addpath('./resis');
addpath('./induct');
addpath('./intpeec');
addpath('./meshpeec');
addpath('./vectorfit');


re = 20e-3;
%mur = 20;
Rsurge = 7.65e-3;
fmur = 10e3;

fplt = [1 50 100 200 500 1e3 2e3  5e3  10e3 20e3 50e3  100e3 200e3 500e3];%];%


Nplt = length(fplt);

Nfit=2;

w0 = 2*pi*fplt;

shape = [1000;];
pt2D = [0  0 ]*1e-3;
dim1 = [5;]*1e-3;
dim2 = [0;]*1e-3;

Rin_pul = [3.6250]*1e-3;

Lin_pul = zeros(length(shape),1);
len = 6.6*ones(length(shape),1);
len=1000000;
%Sig_pul_cs2D = 1./(Rin_pul_cs2D.*dim1_cs2D.*dim2_cs2D);
Sig_pul = 3.5124e+06;
Sig_pul = 59.6*1e6;
Rin_pul = 1./(Sig_pul.* pi*(dim1*dim1-dim2*dim2));

murtmp = mur_fit_rec(dim1, dim2, Rin_pul, Sig_pul, Rsurge, fmur);
murtmp = 1;

mur = ones(1,Nplt)*murtmp;



% mutual inductance of the square loop
Lm = induct_gmd(len,len);

[~,~,Rmesh1, Lmesh1] = main_mesh2d_cmplt( ...
    shape, pt2D, dim1, dim2, Rin_pul, Lin_pul,mur, len, fplt);
Lmesh1=(Lmesh1-Lm);
Rmesh1=Rmesh1;
% [~,~,Rmesh1, Lmesh1] = mesh2d_main_complete_pul( ...
%     shape_cs2D, pt_cs2D, dim1_cs2D, dim2_cs2D, Rin_pul_cs2D, Lin_pul_cs2D,mur, fplt);


Rs1 = zeros(1,Nplt);
Ls1 = zeros(1,Nplt);

[ Rs1, Ls1] = para_main_self_multi_frq(shape, dim1, dim2, ...
    len, Rin_pul, Sig_pul, mur, fplt);
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




figure(3);
hold on
semilogx(fplt/1e3,Rmesh1*1e3/len,'displayname','Segment Model');
semilogx(fplt/1e3,(Rs1)*1e3/len);
% semilogx(fplt/1e3,Rfcur*1e3/len_cs2D,'k.-');
xlabel('f(kHz)');
grid on
hold off

figure(4);
hold on
semilogx(fplt/1e3,Lmesh1*1e6/len,'displayname','Segment Model');
semilogx(fplt/1e3,(Ls1)*1e6/len);
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

