
fpath = 'D:\ProjectFiles\Lightning_Far_Field\Trans_Line1_usr\pulse';
%fpath='F:\Chen\Dropbox\Duke\FDTD_Project\Trans_Line1\Trans_Line1_usr\pulse';
%fpath='D:\Cloud Data\Dropbox\Duke\FDTD_Project\Trans_Line1\Trans_Line1_usr\pulse';

flag_type = 2;
h_ch = 2000;
Ns_ch = 200;

%% total source = source1 + source2
% us
Tmax = 200;
dt = 0.2e-1;
td = 0.01;

% source 1
sr_amp=13.1e3;
sr_tau1 = 0.1;
sr_tau2 = 88;
n = 2;
cof1 = exp(-(sr_tau1/sr_tau2)*(n*sr_tau2/sr_tau1)^(1/n));
[ist1] = sr_heidler(sr_amp,sr_tau1,sr_tau2,n,Tmax,dt,td);
ist1 = ist1*cof1;

% source 2
sr_amp=8.7e3;
sr_tau1 = 0.21;
sr_tau2 = 61;
n = 2;
cof2 = exp(-(sr_tau1/sr_tau2)*(n*sr_tau2/sr_tau1)^(1/n));
[ist2, tus, ts] = sr_heidler(sr_amp,sr_tau1,sr_tau2,n,Tmax,dt,td);
ist2 = cof2*ist2;

% total source
i_sr = ist1 + ist2;
i_sr = 22.7e3/max(i_sr)*i_sr;

figure(1)
plot(tus,i_sr/1e3);
xlabel('Time(us)')
ylabel('Current(kA)')

%%
iout = sr_l_ch_i(h_ch,Ns_ch, i_sr,ts, flag_type);

% write_ch_sr(fpath, fname, pt_name, sr_name);

%% lightning channel
% channel
for ik = 1:Ns_ch
    % TL_Source_user_lport_pulse_1_c02
    % Trans_Line1_user_lport_pulse_1_c06
    sr_name = sprintf('Trans_Line1_user_lport_pulse_1_c06(1,%.3d)',ik);
    spice_i_ch(ts, iout(ik,:), fpath, sr_name);
end

% channel imag
for ik = 1:Ns_ch
    % TL_Source_user_lport_pulse_1_c02
    % Trans_Line1_user_lport_pulse_1_c06
    sr_name = sprintf('Trans_Line1_user_lport_pulse_201_c18(1,%.3d)',ik);
    spice_i_ch(ts, iout(ik,:), fpath, sr_name);
end


