% total source = source1 + source2
% us
Tmax = 100;
dt = .2e-1;

%% source 1
sr_amp=13.1e3;
sr_tau1 = 0.22;
sr_tau2 = 88;
n = 2;
cof1 = exp(-(sr_tau1/sr_tau2)*(n*sr_tau2/sr_tau1)^(1/n));

[ist1] = sr_heidler(sr_amp,sr_tau1,sr_tau2,n,Tmax,dt);

ist1 = ist1* cof1;

%% source 2
sr_amp=8.7e3;
sr_tau1 = 0.21;
sr_tau2 = 61;
n = 2;
cof2 = exp(-(sr_tau1/sr_tau2)*(n*sr_tau2/sr_tau1)^(1/n));

[ist2, t0, ts] = sr_heidler(sr_amp,sr_tau1,sr_tau2,n,Tmax,dt);

ist2 = cof2*ist2;

%% total source
ist = ist1 + ist2;

ist = -22.7e3/max(ist)*ist;

figure(22);
hold on;
plot(t0,ist1,'--');
plot(t0,ist2,'-.');
plot(t0,ist,'k');
hold off
grid on

save_path_name = [];


fpath=[];
fname=['tl_lightning'];
write_source_txt(ts, ist, fpath, fname)
