
sr_type=0;
sr_amp=9.7;
sr_tau1 = 6;
sr_tau2 = 80;
n = 2;


% us
Tmax = 1000;
dt = 1e-1;
[ist, t0, ts] = sr_heidler(sr_amp,sr_tau1,sr_tau2,n,Tmax,dt);
%[ist1, t1] = sr_double_exp(sr_amp, sr_tau1, sr_tau2, Tmax, dt);
ist_max = max(ist);

ist = ist* sr_amp/ist_max;

figure(22);
hold on;
plot(t0,ist,'b');
%plot(t1*1e6,ist1,'r');
hold off
grid on



fpath=[''];
fname=['exp10'];
write_source_txt(ts, ist, fpath, fname)
