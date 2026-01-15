
pt_start_grid = [0 0 -0.5];
pt_end_grid = [1 0  -0.5];

dv_grid = [1 0  0];

re_grid = [20e-3];
len_grid = 1;
sig_soil = 0.001;
epr_soil = 10;


frq = [50 100 500 1000 5e3 10e3 50e3 100e3 500e3 1e6 2e6 5e6 10e6];

[R1, L1] = grid_self_Sunde_h([], [], pt_start_grid, pt_end_grid, ...
    dv_grid, re_grid, len_grid, sig_soil, epr_soil, frq);

[R4, L4] = grid_self_Sunde_log_h([], [], pt_start_grid, pt_end_grid, ...
    dv_grid, re_grid, len_grid, sig_soil, epr_soil, frq);


[R5, L5] = grid_self_LogExp_h([], [], pt_start_grid, pt_end_grid, ...
    dv_grid, re_grid, len_grid, sig_soil, epr_soil, frq);

mu0 = 4*pi*1e-7;
ep0 = 8.85e-12;
w = 2*pi*frq;
ep_soil = epr_soil*ep0;

rg = sqrt( 1j*w*mu0.*(sig_soil + 1j*w*ep_soil) );
cplx_depth = 1./rg;   % complex skin depth


Z1 = R1+1j*w.*L1;
Z4 = R4+1j*w.*L4;
Z5 = R5+1j*w.*L5;
Y1 = rg.^2./Z1;
Y4 = rg.^2./Z4;
Y5 = rg.^2./Z5;

figure;
semilogx(frq, abs(Z1./(1j*w)),'k');
hold on;
semilogx(frq, abs(Z4./(1j*w)),'k--s')
semilogx(frq, abs(Z5./(1j*w)),'r--v')

hold off
grid on
xlabel('Frquency (Hz)');
ylabel('|Z| (ohm/m)');
legend('Sunde','Sunde log','LogExp');

figure;
semilogx(frq, abs(Y1),'k');
hold on;
semilogx(frq, abs(Y4),'k--s')
semilogx(frq, abs(Y5),'r--v')

hold off
grid on
xlabel('Frquency (Hz)');
ylabel('|Y| (ohm/m)');
legend('Sunde','Sunde log','LogExp');


