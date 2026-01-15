% total source = source1 + source2
% us
Tmax = 200;
dt = .2e-1;

%% source 1
sr_amp = 200e3;
sr_tau1 = 19;
sr_tau2 = 485;
n = 10;

[ist1] = sr_heidler(sr_amp,sr_tau1,sr_tau2,n,Tmax,dt);
ist1_max = max(ist1);

ist1 = ist1* sr_amp/ist1_max;


%% source 2
sr_amp = 100e3;
sr_tau1 = 1.82;
sr_tau2 = 285;
n = 10;

[ist2, t0, ts] = sr_heidler(sr_amp,sr_tau1,sr_tau2,n,Tmax,dt);
ist2_max = max(ist2);

ist2 = ist2* sr_amp/ist2_max;


%% total source

figure(22);
hold on;
plot(t0,ist1/1e3,'k-','LineWidth',1);
plot(t0,ist2/1e3,'k--','LineWidth',1);
hold off
grid off
xlabel('Time(us)');
ylabel('Current(kA)')
legend('First Stroke','Sub. Stroke')

