
%clear
%% flat steel

Rsurge = 7.65e-3;
fmur = 10e3;

frq = [1 50 100 200 500 1e3 2e3  5e3  10e3 20e3 50e3 100e3 200e3 500e3 ];

Nf = length(frq);

Nfit=2;

w0 = 2*pi*frq;

shp_id = [1002;];
pt_2d = [0  0 ]*1e-3;
dim1 = [40;]*1e-3;
dim2 = [4;]*1e-3;
re = [10.8]*1e-3;
Rin_pul = [1.28]*1e-3;


len = 1.65;

sig_pul = 4.8828e+06;
Rin_pul = 1./(sig_pul.* (dim1.*dim2));

murtmp = mur_fit_rec(dim1, dim2, Rin_pul, sig_pul, Rsurge, fmur);

mur = ones(1,Nf)*murtmp;


[ Rs1, Ls1] = para_self_multi_frq(shp_id, dim1, dim2, ...
    len, Rin_pul, sig_pul, mur, frq);

LsHF = induct_bar_ac(dim1, dim2, sig_pul, 1, len,10e6);
Ls1 = Ls1-LsHF;


[Rdc, L0, Rn, Ln, Zfit] = vectfit_main_Z( Rs1(1,1:Nf),  Ls1(1,1:Nf), frq, Nfit,0);

Rfcur = zeros(1,Nf);
Lfcur = zeros(1,Nf);
Rfcur(1:Nf) = real(Zfit);
Lfcur(1:Nf) = imag(Zfit)./(2*pi*frq);

figure(4);
hold on
semilogx(frq/1e3,(Rs1)*1e3);
semilogx(frq/1e3,Rfcur*1e3,'k.-');
xlabel('f(kHz)');
grid on
hold off

figure(5);
hold on
semilogx(frq/1e3,(Ls1)*1e6);
semilogx(frq/1e3,Lfcur*1e6,'k.-');
xlabel('f(kHz)');
hold off
grid on

%% round steel

frq = [1 50 100 200 500 1e3 2e3  5e3  10e3 20e3 50e3 100e3 200e3 500e3 ];

Nf = length(frq);

Nfit=2;

w0 = 2*pi*frq;

shp_id = [2001;];
pt_2d = [0  0 ]*1e-3;
dim1 = [5;]*1e-3;
dim2 = [0;]*1e-3;
re = [5]*1e-3;

len = 1;
Rin_pul=3.16e-3;

sig_pul = 1./(Rin_pul.* (pi*dim1.^2));


murtmp = 35;
mur = ones(1,Nf)*murtmp;


[ Rs1, Ls1] = para_self_multi_frq(shp_id, dim1, dim2, ...
    len, Rin_pul, sig_pul, mur, frq);

LsHF = induct_cir_ext(dim1, len);
Ls1 = Ls1-LsHF;


[Rdc, L0, Rn, Ln, Zfit] = vectfit_main_Z( Rs1(1,1:Nf),  Ls1(1,1:Nf), frq, Nfit,0);

Rfcur = zeros(1,Nf);
Lfcur = zeros(1,Nf);
Rfcur(1:Nf) = real(Zfit);
Lfcur(1:Nf) = imag(Zfit)./(2*pi*frq);

figure(7);
hold on
semilogx(frq/1e3,(Rs1)*1e3);
semilogx(frq/1e3,Rfcur*1e3,'k.-');
xlabel('f(kHz)');
ylabel('R(mohm)');
grid on
hold off

figure(8);
hold on
semilogx(frq/1e3,(Ls1)*1e6);
semilogx(frq/1e3,Lfcur*1e6,'k.-');
xlabel('f(kHz)');
ylabel('L(uH)');
hold off
grid on




%% exp 3 vertical rod
%clear

frq = [1 50 100 200 500 1e3 2e3  5e3  10e3 20e3 50e3 100e3 200e3  ];

Nf = length(frq);

Nfit=1;

w0 = 2*pi*frq;

shp_id = [2001;];
pt_2d = [0  0 ]*1e-3;
dim1 = [10;]*1e-3;
dim2 = [0;]*1e-3;
re = [10]*1e-3;

len = 6;

sig_pul = 3.4e6;
Rin_pul = 1./(sig_pul.* (pi*dim1.^2));

murtmp = 400;
mur = ones(1,Nf)*murtmp;


[ Rs1, Ls1] = para_self_multi_frq(shp_id, dim1, dim2, ...
    len, Rin_pul, sig_pul, mur, frq);

LsHF = induct_cir_ext(dim1, len);
Ls1 = Ls1-LsHF;


[Rdc, L0, Rn, Ln, Zfit] = vectfit_main_Z( Rs1(1,1:Nf),  Ls1(1,1:Nf), frq, Nfit,0);

Rfcur = zeros(1,Nf);
Lfcur = zeros(1,Nf);
Rfcur(1:Nf) = real(Zfit);
Lfcur(1:Nf) = imag(Zfit)./(2*pi*frq);

figure(7);
hold on
semilogx(frq/1e3,(Rs1)*1e3);
semilogx(frq/1e3,Rfcur*1e3,'k.-');
xlabel('f(kHz)');
ylabel('R(mohm)');
grid on
hold off

figure(8);
hold on
semilogx(frq/1e3,(Ls1)*1e6);
semilogx(frq/1e3,Lfcur*1e6,'k.-');
xlabel('f(kHz)');
ylabel('L(uH)');
hold off
grid on


%% exp 4 horizontal rod
%clear

frq = [1 50 100 200 500 1e3 2e3  5e3  10e3 20e3 50e3 100e3 200e3 500e3 1e6 2e6 ];

Nf = length(frq);

Nfit=1;

w0 = 2*pi*frq;

shp_id = [2001;];
pt_2d = [0  0 ]*1e-3;
dim1 = [6;]*1e-3;
dim2 = [0;]*1e-3;
re = [6]*1e-3;

len = 15;

sig_pul = 5.8e7;
Rin_pul = 1./(sig_pul.* (pi*dim1.^2));

murtmp = 1;
mur = ones(1,Nf)*murtmp;


[ Rs1, Ls1] = para_self_multi_frq(shp_id, dim1, dim2, ...
    len, Rin_pul, sig_pul, mur, frq);

LsHF = induct_cir_ext(dim1, len);
Ls1 = Ls1-LsHF;

[Rdc, L0, Rn, Ln, Zfit] = vectfit_main_Z( Rs1(1,1:Nf),  Ls1(1,1:Nf), frq, Nfit,0);

Rfcur = zeros(1,Nf);
Lfcur = zeros(1,Nf);
Rfcur(1:Nf) = real(Zfit);
Lfcur(1:Nf) = imag(Zfit)./(2*pi*frq);

figure(7);
hold on
semilogx(frq/1e3,(Rs1)*1e3);
semilogx(frq/1e3,Rfcur*1e3,'k.-');
xlabel('f(kHz)');
ylabel('R(mohm)');
grid on
hold off

figure(8);
hold on
semilogx(frq/1e3,(Ls1)*1e6);
semilogx(frq/1e3,Lfcur*1e6,'k.-');
xlabel('f(kHz)');
ylabel('L(uH)');
hold off
grid on

