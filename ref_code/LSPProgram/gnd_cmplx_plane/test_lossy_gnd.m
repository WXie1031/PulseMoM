clc

pt_start = [0 0 0;
    0 0.1 5
    0 0 0.2;
    0 0 0;];
pt_end = [0 0 5;
    0 0.1 0
    5 0 0.2;
    2 2 10;];

re = [5e-3; ]*ones(size(pt_start,1),1);

[dv, len] = line_dv(pt_start, pt_end);

sig_soil = 1000;
epr_soil = 10;
frq = [1 100 200 500 1e3 2e3 5e3 10e3 50e3 100e3 500e3 1e6];


[Rg1,Lg1, Rgself1,Lgself1] = grid_cmplx_plane(pt_start,pt_end, dv, re, len,...
    sig_soil, epr_soil, frq);
[Rmtx, Lmtx] = para_main_fila(pt_start, pt_end, dv, re, len);

% [Rg1, Lg1, Rgself1, Lgself1] = ground_cmplx_plane(pt_start, pt_end, dv, re, len,...
%     sig_soil, frq, 0);

%  [Rg2, Lg2, Rgself2, Lgself2] = ground_pec(pt_start, pt_end, dv, ...
%     re, len, frq, 0); 
% 
% [Rg3,Lg3, Rgself3,Lgself3] = grid_cmplx_plane_new(pt_start,pt_end, dv, re, len,...
%     sig_soil, epr_soil, frq);

figure;
subplot(1,2,1);
semilogx(frq/1e3,Rgself1(1,:)*1e3)
hold on
semilogx(frq/1e3,Rgself1(2,:)*1e3)
semilogx(frq/1e3,Rgself1(3,:)*1e3)
semilogx(frq/1e3,Rgself1(4,:)*1e3)
ylabel('R(m\Omega)')
xlabel('Frequency(kHz)')

subplot(1,2,2);
semilogx(frq/1e3,Lgself1(1,:)*1e6)
hold on
semilogx(frq/1e3,Lgself1(2,:)*1e6)
semilogx(frq/1e3,Lgself1(3,:)*1e6)
semilogx(frq/1e3,Lgself1(4,:)*1e6)
ylabel('L(\muH)')
xlabel('Frequency(kHz)')



pf1 = [0 0 0;
    0 0 0;
    0 0 0;
    0 0 0;];
pf2 = [0 0 -1;
    0 0 1;
    2 2 10;
    2 2 -10];
% 
% pf1 = [0 0 1;
%     0 0 1;];
% pf2 = [0 0 0;
%     0 0 2;];
% 
% 
r2 = [5e-3;]*ones(size(pf1,1),1);
[dv2, l2] = line_dv(pf1, pf2);

ps1=pf1(1,:);
ps2=pf2(1,:);
dv1 = dv2(1,:);
r1 = r2(1);
l1=l2(1);
cal_L_fila(ps1,ps2,dv1,l1,r1, pf1,pf2,dv2,l2,r2)

