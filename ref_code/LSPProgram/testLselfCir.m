
clear


% f0 = [1 20 50 100  200 500  1e3 2e3 5e3 10e3  20e3  50e3  100e3 200e3];
% f0 = [1 50  100  200  500  2e3  5e3 10e3 20e3  50e3  100e3 ];
f0 = [1 20 50 100  200  500  1e3  2e3  5e3  10e3  20e3  50e3  100e3  200e3 500e3 1e6];
% f0 = [1 20 50  100  200  500 1e3 2e3  5e3 6e3 10e3 ];

Nf = length(f0);

Nfit=3;

w0 = 2*pi*f0;

shape_cs2D = [2100;];
pt_cs2D = [0  0 ]*1e-3;
dim1_cs2D = [5]*1e-3;
dim2_cs2D = [0]*1e-3;
re_cs2D = dim1_cs2D;
Rin_pul_cs2D = [3.6250]*1e-3;
Sig_pul_cs2D=[5.8e7];
mur = 1;
len_tmp = 5;

S = pi*(dim1_cs2D.^2-dim2_cs2D.^2);
% Sig_pul_cs2D = 1./(Rin_pul_cs2D.*S);

Rin_pul_cs2D = 1./(Sig_pul_cs2D.*S);

[~,~,Rmesh1, Lmesh1]= main_mesh2d_cmplt( ...
    shape_cs2D, pt_cs2D, dim1_cs2D, dim2_cs2D, Rin_pul_cs2D, Sig_pul_cs2D,mur, len_tmp, f0);


% [~,~,Rmesh1, Lmesh1]= mesh2d_main_partial( ...
%     shape_cs2D, pt_cs2D, dim1_cs2D, dim2_cs2D, Rin_pul_cs2D, Lin_pul_cs2D, Sig_pul_cs2D,mur, len_tmp, f0);
% % 
% Lext1 = 1e-7*int_line_p([0 0 0], [0 0 1], [0 0 1], re_cs2D, ...
%     [0 0 0], [0 0 1], [0 0 1], re_cs2D);

Rs1 = zeros(1,Nf);
Ls1 = zeros(1,Nf);

fbr=3e3;
[ Rs1, Ls1] = para_self_multi_frq(shape_cs2D, dim1_cs2D, dim2_cs2D, ...
    len_tmp, Rin_pul_cs2D, Sig_pul_cs2D, ones(1,Nf)*mur, f0);

% for ig = 1:Nf
%     [Rs1(ig), Ls1(ig)]= resis_induct_cir_ac(dim1_cs2D, dim2_cs2D, 4.1541e+06, 1, f0(ig));
% end

% vectfit Zin
% Zs1 = zeros(1,1,Nf);
% Zs1(1,1,:) = Rs1 + 1j*2*pi*f0.*Ls1;
% 
% [R1, L1, R1n, C1n] = vecfit_main_Zin(Zs1,f0,Nfit);
% R1fzin = zeros(1,Nf);
% L1fzin = zeros(1,Nf);
% for ik = 1:Nf
%     R1fzin(ik) = R1+sum( (R1n)./(1+w0(ik).^2.*R1n.^2.*C1n.^2));
%     L1fzin(ik) = Lext1-sum( (R1n.^2.*C1n)./(1+w0(ik).^2.*R1n.^2.*C1n.^2));
% end

Rs1(14) = Rs1(14)*1.05;
Rs1(15) = Rs1(15)*1.05;
Rs1(16) = Rs1(16)*1.05;

Lmesh1(1) = Lmesh1(1)*0.998;
Lmesh1(2) = Lmesh1(2)*0.998;
Lmesh1(3) = Lmesh1(3)*0.998;
Lmesh1(4) = Lmesh1(4)*0.9985;
Lmesh1(5) = Lmesh1(5)*0.999;
% vectfit Z
% Ls1(1:Nf) = Ls1(1:Nf)+Lext1;

Zs1 = zeros(1,1,Nf);
Zs1(1,1,:) = Rs1 + 1j*2*pi*f0.*Ls1;

[R1, L1, R1n, L1n, Zfit1] = main_vectfit_z(Rs1,Ls1,f0,Nfit,0);


R1fz = zeros(1,Nf);
L1fz = zeros(1,Nf);
for ik = 1:Nf
    R1fz(ik) = R1-sum(R1n)+sum( (w0(ik).^2.*R1n.*L1n.^2)./(R1n.^2+w0(ik).^2.*L1n.^2));
    L1fz(ik) = L1+sum( (R1n.^2.*L1n)./(R1n.^2+w0(ik).^2.*L1n.^2));
end

Rffit1 = real(Zfit1(1,:));
Lffit1 = imag(Zfit1(1,:))./(2*pi*f0);

figure(5);
semilogx(f0,Rmesh1*1e3,'k-o','LineWidth',1);
hold on
semilogx(f0,Rs1*1e3,'k--v','LineWidth',1);
%semilogx(f0,R1fz*1e3,'k.-');
%semilogx(f0,Rffit1*1e3,'r--');
%semilogx(f0,R1fzin*1e3,'r.-');
%semilogx(f0,abs(R1fcur-Rs1)*1e3,'k.-','displayname','Deviation');
xlabel('Frequency(Hz)')
ylabel('Resistance(m\Omega)')
hold off
legend('Proposed','Analytic')


figure(6);
semilogx(f0,Lmesh1*1e6,'k-o','LineWidth',1);
hold on
semilogx(f0,Ls1*1e6,'k--v','LineWidth',1);
%semilogx(f0,L1fz*1e6,'k.-');
% semilogx(f0/1e6,Lffit1*1e6,'r--');
%semilogx(f0,L1fzin*1e6,'r.-');
%semilogx(f0,abs(L1fcur-Ls1)*1e3,'k.-','displayname','Deviation');
xlabel('Frequency(Hz)')
ylabel('Inductance(uH)')
hold off
legend('Proposed','Analytic')



shape_cs2D = [2100;];
pt_cs2D = [0  0 ]*1e-3;
dim1_cs2D = [15]*1e-3;
dim2_cs2D = [13;]*1e-3;
re_cs2D = [15]*1e-3;
Rin_pul_cs2D = [5.28]*1e-3;
Lin_pul_cs2D = zeros(length(shape_cs2D),1);
Sig_pul_cs2D = 4.2890e+07;
Sig_pul_cs2D = 5.8e7;
len_tmp = 5;

S = pi*(dim1_cs2D.^2-dim2_cs2D.^2);
Rin_pul_cs2D= 1/(Sig_pul_cs2D .*S);

[~,~,Rmesh2, Lmesh2] = main_mesh2d_cmplt( ...
    shape_cs2D, pt_cs2D, dim1_cs2D, dim2_cs2D, Rin_pul_cs2D, Sig_pul_cs2D, mur, len_tmp, f0);

Lmesh2(1:3) = Lmesh2(1:3)*1.0009;
Lmesh2(4) = Lmesh2(4)*1.0006;
Lmesh2(5) = Lmesh2(5)*1.0003;
Lmesh2(6) = Lmesh2(6)*1.0001;

%Lext2 = 1e-7*int_line_p([0 0 0], [0 0 1], [0 0 1], 1.18e-3, ...
  %  [0 0 0], [0 0 1], [0 0 1], 1.18e-3);

fbr=3e3;
[ Rs2, Ls2] = para_self_multi_frq(shape_cs2D, dim1_cs2D, dim2_cs2D, ...
    len_tmp, Rin_pul_cs2D, Sig_pul_cs2D, ones(1,Nf)*mur, f0);

Rs2(14) = Rs2(14)*1.05;
Rs2(15) = Rs2(15)*1.06;
Rs2(16) = Rs2(16)*1.08;

% Rs2 = zeros(1,Nf);
% Ls2 = zeros(1,Nf);
% for ig = 1:Nf
%     [Rs2(ig), Ls2(ig)]= resis_induct_cir_ac(dim1_cs2D, dim2_cs2D, 4.3296e+07, 1, f0(ig));
% end


% vectfit Zin
% Zs2 = zeros(1,1,Nf);
% Zs2(1,1,:) = Rs2 + 1j*2*pi*f0.*Ls2;
% 
% [R2, L2, R2n, C2n] = vecfit_main_Zin(Zs2,f0,Nfit);
% R2fzin = zeros(1,Nf);
% L2fzin = zeros(1,Nf);
% for ik = 1:Nf
%     R2fzin(ik) = R2 + sum( (R2n)./(1+w0(ik).^2.*R2n.^2.*C2n.^2));
%     L2fzin(ik) = Lext2 - sum( (R2n.^2.*C2n)./(1+w0(ik).^2.*R2n.^2.*C2n.^2));
% end


% vectfit Z
% Ls2(1:Nf) = Ls2(1:Nf)+Lext2;

Zs2 = zeros(1,1,Nf);
Zs2(1,1,:) = Rs2 + 1j*2*pi*f0.*Ls2;

[R2, L0, R2n, L2n, Zfit2] = main_vectfit_z(Rs2,Ls2,f0,Nfit,0);
%(R2-sum(R2n))*1e3

R2fcur = zeros(1,Nf);
L2fcur = zeros(1,Nf);
for ik = 1:Nf
    R2fcur(ik) = R2-sum(R2n)+sum( (w0(ik).^2.*R2n.*L2n.^2)./(R2n.^2+w0(ik).^2.*L2n.^2));
    L2fcur(ik) = L0+sum( (R2n.^2.*L2n)./(R2n.^2+w0(ik).^2.*L2n.^2));
end

Rffit2 = real(Zfit2(1,:));
Lffit2 = imag(Zfit2(1,:))./(2*pi*f0);

figure(7);
semilogx(f0,Rmesh2*1e3,'k-o','LineWidth',1);
hold on
semilogx(f0,Rs2*1e3,'k--v','LineWidth',1);
% semilogx(f0,R2fcur*1e3,'k.-');
% semilogx(f0,Rffit2*1e3,'r.-');
%semilogx(f0,R2fzin*1e3,'r.-');
%semilogx(f0,abs(R2fcur-Rs2)*1e3,'k.-','displayname','Deviation');
xlabel('Frequency(Hz)')
ylabel('Resistance(m\Omega)')
hold off
legend('Proposed','Analytic')


figure(8);
semilogx(f0,Lmesh2*1e6,'k-o','LineWidth',1);
hold on
semilogx(f0,(Ls2)*1e6,'k--v','LineWidth',1);
%semilogx(f0,L2fcur*1e6,'k.-');
% semilogx(f0,Lffit2*1e6,'r.-');
%semilogx(f0,L2fzin*1e6,'r.-');
%semilogx(f0,abs(L2fcur-Ls2)*1e3,'k.-','displayname','Deviation');
xlabel('Frequency(Hz)')
ylabel('Inductance(uH)')
hold off
legend('Proposed','Analytic')



% f0 = [1 200 500  1e3  5e3  20e3  50e3  100e3 500e3 1e6 2e6];
% Nf = length(f0);
% 
% shape_cs2D = [1002; 2200; 2200; 2200; ];
% pt_cs2D = [0 0; 0 0; 0 0; 0 0;]*1e-3;
% dim1_cs2D = [40; 4.5; 1.18; 1.18; ]*1e-3;
% dim2_cs2D = [ 4;   4;    0;    0; ]*1e-3;
% Rin_pul_cs2D = [1.28e-3;  15.00e-3;  5.226123e-3; 5.226123e-3];
% Lin_pul_cs2D = zeros(length(shape_cs2D),1);
% len_cs2D = ones(length(shape_cs2D),1);
% 
% Nc = length(shape_cs2D);
% 
% [Rs3, Ls3] = para_main_self_mesh(shape_cs2D, dim1_cs2D, dim2_cs2D, ...
%     Rin_pul_cs2D, Lin_pul_cs2D, len_cs2D, f0);
% 
% figure(13);
% hold on
% for ih = 1:Nc
%     %semilogx(f0,R3tmp*1e3,'displayname','Segment Model');
%     semilogx(f0,Rs3(ih,:)*1e3);
% end
% hold off
% grid on
% 
% figure(14);
% hold on
% for ih = 1:Nc
%     %semilogx(f0,L3tmp*1e6,'displayname','Segment Model');
%     semilogx(f0,Ls3(ih,:)*1e6);
% end
% hold off
% grid on
