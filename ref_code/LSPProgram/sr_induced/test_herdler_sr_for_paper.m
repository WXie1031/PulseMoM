

%[FileName,PathName] = uiputfile({'*.cir', 'Circuit Files (*.cir)'});
%save_path_name = [PathName, FileName];


sr_type=0;
sr_amp=35;
sr_tau1 = 0.16;
sr_tau2 = 16;
n = 3;

sr_type=0;
sr_amp=9.7;
sr_tau1 = 6;
sr_tau2 = 80;
n = 2;

sr_type=0;
sr_amp=50e3;
sr_tau1 = 0.454;
sr_tau2 = 143;
n = 10;

% sr_type=0;
% sr_amp=20e3;
% sr_tau1 = 19;
% sr_tau2 = 485;
% n = 10;

sr_type=0;
sr_amp=13.1e3;
sr_tau1 = 0.22;
sr_tau2 = 88;
n = 2;

% us
Tmax = 100;
dt = .2e-1;
[ist0, t0, ts] = sr_heidler(sr_amp,sr_tau1,sr_tau2,n,Tmax,dt);
%[ist1, t1] = sr_double_exp(sr_amp, sr_tau1, sr_tau2, Tmax, dt);
ist_max = max(ist0);

ist0 = ist0* sr_amp/ist_max;

figure(22);
hold on;
plot(t0,ist0,'b');
%plot(t1*1e6,ist1,'r');
hold off
grid on


save_path_name='D:\Cloud Data\Dropbox\Work\Journal\4-SystemSim\Data\SysSim\Tower_Case\20kA_10_350us';
save_path_name = [];


fpath=[];
fname=['tl_lightning'];
write_source_txt(ts, ist0, fpath, fname)
