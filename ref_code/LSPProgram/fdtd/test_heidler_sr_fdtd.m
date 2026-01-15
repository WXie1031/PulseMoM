

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
% 
% sr_type=0;
% sr_amp=100e3;
% sr_tau1 = 19;
% sr_tau2 = 485;
% n = 10;

% us
Tmax = 1000;
dt = 0.1;
[ist0, t0, ts] = sr_heidler(sr_amp,sr_tau1,sr_tau2,n,Tmax,dt);
%[ist1, t1] = sr_double_exp(sr_amp, sr_tau1, sr_tau2, Tmax, dt);
ist_max = max(ist0);

ist0 = ist0* sr_amp/ist_max;

figure(22);
hold on;
plot(t0,ist0,'b');
%plot(t1*1e6,ist1,'r');
hold off
xlabel('Time(us)')



save_path_name='D:\Cloud Data\Dropbox\Work\Journal\4-SystemSim\Data\SysSim\Tower_Case\20kA_10_350us';
save_path_name = [];

% % create_heidler_source(0, sr_amp, sr_tau1, sr_tau2, n, save_path_name,1)
% dt=Exp4_I(2,1)-Exp4_I(1,1);
% Nt = length(Exp4_I(:,1));
% Tsta = max(dt);
% win = window_cos(Tsta, Nt, dt, twin1, twin2);
dt = ts(2)-ts(1);
fs = 1/dt;
df = 1/max(ts);
NFFT = ceil(fs/df);
if mod(NFFT,2)~=0
    NFFT=NFFT+1;
end
Nt = length(ts);
Ifft = fft(ist0);
Ifft = Ifft(1:NFFT/2+1);
f = fs/2*linspace(0,1,NFFT/2+1);
%f(1)=1;
figure(23);
semilogx(f(f<=2e6), abs(Ifft(f<=2e6)));
xlabel('Frequency')

abs(Ifft(f==1e6))/abs(sum(Ifft))*100

fpath=[];
fname=['sr_025_100'];
write_source_txt(ts, ist0, fpath, fname)


