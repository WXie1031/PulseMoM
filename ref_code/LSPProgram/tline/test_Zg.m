
pt_start_grid = [0 0 -1];
pt_end_grid = [5 0  -1];

dv_grid = [1 0  0];

re_grid = [4e-3];
len_grid = 5;
sig_soil = 1/150;
epr_soil = 10;

frq = [1 100 200 500 1e3 2e3 5e3 10e3 20e3 50e3 100e3 200e3 500e3 1e6];
frq = [50 100 500 1000 5e3 10e3 50e3 100e3 500e3 1e6 2e6 5e6 10e6];

[R1, L1] = grid_self_Sunde_h([], [], pt_start_grid, pt_end_grid, ...
    dv_grid, re_grid, len_grid, sig_soil, epr_soil, frq);

[R4, L4] = grid_self_Sunde_log_h([], [], pt_start_grid, pt_end_grid, ...
    dv_grid, re_grid, len_grid, sig_soil, epr_soil, frq);

offset = 2*abs(pt_start_grid(:,3));
[~,~,R2, L2] = ground_cmplx_plane_b1(pt_start_grid, pt_end_grid, dv_grid, re_grid, len_grid,...
    sig_soil, epr_soil, frq, 0);

[~,~,R2p, L2p] = grid_cmplx_plane(pt_start_grid, pt_end_grid, dv_grid, re_grid, len_grid,...
    sig_soil, epr_soil, frq);

 [R5, L5] = grid_self_LogExp_h([], [], pt_start_grid, pt_end_grid, ...
    dv_grid, re_grid, len_grid, sig_soil, epr_soil, frq);

mu0 = 4*pi*1e-7;
ep0 = 8.85e-12;
w = 2*pi*frq;
ep_soil = epr_soil*ep0;

rg = sqrt( 1j*w*mu0.*(sig_soil + 1j*w*ep_soil) );
cplx_depth = 1./rg;   % complex skin depth

Ztl = 1j*w.*mu0./(2*pi).*( log( (2*cplx_depth)./re_grid ) ).*len_grid;
Rg = real(Ztl);
Lg = imag(Ztl)./w;


Z3 = induct_cir_ext(cplx_depth*2, len_grid);
R3d = imag(Z3).*w;
L3d = real(Z3);

figure;
semilogx(frq, abs(R1+1j*w.*L1),'k');
hold on;
semilogx(frq, abs(R4+1j*w.*L4),'k--s')
semilogx(frq, abs(Rg+1j*w.*Lg),'k--x')
%semilogx(frq, abs(R3d+1j*w.*L3d),'k--o')
semilogx(frq, abs(R2+1j*w.*L2),'k--v')
%semilogx(frq, abs(R2p+1j*w.*L2p),'k--^')
semilogx(frq, abs(R5+1j*w.*L5),'r--v')

hold off
grid on
xlabel('Frquency (Hz)');
ylabel('|Z| (ohm/m)');
legend('Sunde','Sunde log','r=cplx depth','cplx plane','LogExp');


figure;
semilogx(frq, (R1),'b');
hold on;
semilogx(frq, (R4),'b--s')
semilogx(frq, (Rg),'b--x')
semilogx(frq, (R2),'b--v')
semilogx(frq, R5,'r--^')

hold off
grid on
xlabel('Frquency (Hz)');
ylabel('R (ohm/m)');
legend('Sunde','Sunde log','r=cplx depth','cplx plane','LogExp');

figure;
semilogx(frq, (L1)*1e6,'b');
hold on;
semilogx(frq, (L4)*1e6,'b--s')
semilogx(frq, (Lg)*1e6,'b--x')
semilogx(frq, (L2)*1e6,'b--v')
semilogx(frq, L5*1e6,'r--^')

hold off
grid on
xlabel('Frquency (Hz)');
ylabel('L (uH/m)');
legend('Sunde','Sunde log','r=cplx depth','cplx plane','LogExp');