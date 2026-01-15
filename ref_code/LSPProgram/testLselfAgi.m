
%clear

addpath('./vectorfit');


Rsurge = 1.81e-3;

% f0 = [1 20 50 100  200  500  1e3  2e3  5e3  10e3  20e3  1e6  2e6  5e6  10e6  20e6];
f0 = [1  50 200 500 1e3 2e3  5e3 8e3 10e3 20e3   50e3  100e3 200e3];%
Nf = length(f0);

Nfit=2;

w0 = 2*pi*f0;

shape_cs2D = [1003;];
pt_cs2D = [0  0 ]*1e-3;
dim1_cs2D = [70;]*1e-3;
dim2_cs2D = [5;]*1e-3;

Lin_pul_cs2D = zeros(length(shape_cs2D),1);
len_cs2D = 5*ones(length(shape_cs2D),1);

Rin_pul_cs2D = [0.296]*1e-3;
SS = (dim1_cs2D)^2-(dim1_cs2D-dim2_cs2D)^2;

Sig_pul_cs2D = 1./(Rin_pul_cs2D.*SS);
%Sig_pul_cs2D = 4.8828e+06;
%Rin_pul_cs2D = 1./(Sig_pul_cs2D.* ((dim1_cs2D)^2-(dim1_cs2D-dim2_cs2D)^2));


murtmp = mur_fit_agi(dim1_cs2D, dim2_cs2D, Rin_pul_cs2D, Sig_pul_cs2D, Rsurge, fmur);
mur = ones(1,Nf)*murtmp;
% mur = ones(1,Nf);

[~,~,Rmesh1, Lmesh1] = mesh2d_main_complete( ...
    shape_cs2D, pt_cs2D, dim1_cs2D, dim2_cs2D, Rin_pul_cs2D, Lin_pul_cs2D, mur, len_cs2D, f0);


Rs1 = zeros(1,Nf);
Ls1 = zeros(1,Nf);
Rmag1 = zeros(1,Nf);

[ Rs1, Ls1] = para_main_self_multi_frq(shape_cs2D, dim1_cs2D, dim2_cs2D, ...
    len_cs2D, Rin_pul_cs2D, Sig_pul_cs2D, mur, f0);

% Rmag1 = resis_mag(Rs1, Rsurge, fbr, f0);

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


Zs = zeros(1,1,Nf);
Zs(1,1,:) = Rs1(1,:) + 1j*w0.*Ls1(1,:);

[R0, L0, Rn, Ln] = vecfit_kernel_Z(Zs, f0, Nfit);
Rdc = R0-sum(Rn);


%     Rdc = Rin_pul_cs2D.*len_cs2D;
Rfcur = zeros(1,Nf);
Lfcur = zeros(1,Nf);
for ik = 1:Nf
    Rfcur(ik) = Rdc+sum( (w0(ik).^2.*Rn.*Ln.^2)./(Rn.^2+w0(ik).^2.*Ln.^2));
    Lfcur(ik) = L0+sum( (Rn.^2.*Ln)./(Rn.^2+w0(ik).^2.*Ln.^2));
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
% 

figure(11);
hold on

semilogx(f0,Rmesh1*1e3,'displayname','Segment Model');
semilogx(f0,(Rs1)*1e3);
% semilogx(f0,Rmag1*1e3);
% semilogx(f0,(Rimag)*1e3);
% semilogx(f0,(Rzmag)*1e3);
semilogx(f0,Rfcur*1e3,'k.-');
grid on
hold off

figure(12);
hold on
semilogx(f0,Lmesh1*1e6,'displayname','Segment Model');
semilogx(f0,(Ls1)*1e6);
% semilogx(f0,(Limag)*1e6);
% semilogx(f0,(Lzmag)*1e6);
semilogx(f0,Lfcur*1e6,'k.-');
hold off
grid on


