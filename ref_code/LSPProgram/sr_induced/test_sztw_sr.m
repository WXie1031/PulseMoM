
fpath = 'F:\Chen\Dropbox\Duke\FDTD_Project\ShenzhenTW\ShenzhenTW_usr\pulse';
%fpath='D:\Cloud Data\Dropbox\Duke\FDTD_Project\Trans_Line1\Trans_Line1_usr\pulse';

%% near xdir case
fpath = 'F:\Chen\Dropbox\Duke\FDTD_Project\ShenzhenTW_near_xdir\ShenzhenTW_near_1_usr\pulse';

flag_type = 2;
h_ch = 3600;
Ns_ch = 180;

%% total source = source1 + source2
% us
Tmax = 500;
dt = 0.05;
td = 5;

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

for ik = 1:Ns_ch
    if ik<=25
        sr_name = sprintf('ShenzhenTW_user_lport_pulse_1_c08(1,%.2d)',ik);
        sr_name = sprintf('ShenzhenTW_near_1_user_lport_pulse_S1_c11(1,%.2d)',ik);
    elseif ik>25 && ik<=55
        sr_name = sprintf('ShenzhenTW_user_lport_pulse_14_c09(1,%.2d)',ik-12);
        sr_name = sprintf('ShenzhenTW_near_1_user_lport_pulse_S2_c13(1,01)(1,%.2d)',ik-12);
    else
        sr_name = sprintf('ShenzhenTW_user_lport_pulse_65_c10(1,%.3d)',ik-62);
        sr_name = sprintf('ShenzhenTW_near_1_user_lport_pulse_S3_c14(1,%.3d)',ik-62);
    end

    spice_i_ch(ts, iout(ik,:), fpath, sr_name);
end




