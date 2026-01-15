clear
% 
a=0.5;
pt_start = [110,0,a-0.001;
    50,0,a-0.001;
   ];
pt_end = [110,0,a+0.001;
    50,0,a-0.001;
    ];


% total source = source1 + source2
% us
Tmax = 20;
dt = .5e-1;

%% source 1
sr_amp=13.1e3;
sr_tau1 = 0.22;
sr_tau2 = 88;
n = 2;
cof1 = exp(-(sr_tau1/sr_tau2)*(n*sr_tau2/sr_tau1)^(1/n));

[ist1] = sr_heidler(sr_amp,sr_tau1,sr_tau2,n,Tmax,dt);

ist1 = ist1*cof1;

%% source 2
sr_amp=8.7e3;
sr_tau1 = 0.21;
sr_tau2 = 61;
n = 2;
cof2 = exp(-(sr_tau1/sr_tau2)*(n*sr_tau2/sr_tau1)^(1/n));

[ist2, tus, ts] = sr_heidler(sr_amp,sr_tau1,sr_tau2,n,Tmax,dt);

ist2 = cof2*ist2;

%% total source
i_sr = ist1 + ist2;
i_sr = 22.7e3/max(i_sr)*i_sr;

soil_sig=3.5e-3;
soil_epr=10;

t_sr = tus*1e-6;
Nt = length(tus);
% i_sr = ones(1,Nt);
% i_sr(1)=0;
pt_hit = [0 0];
Ns_ch = 2000;
%h_ch = 5e3;
h_ch = 2e3;

flag_type = 2;

% iout = sr_l_ch_i(h_ch,Ns_ch, i_sr,t_sr, flag_type);

tic
%[Uout1,Er_T,Ez_T] = sr_induced_e_num(pt_hit,h_ch,Ns_ch, pt_start,pt_end, i_sr, t_sr,flag_type);
[Uout1,Er_T,Ez_T,Ha0_T] = sr_induced_e_num_lossy(pt_hit,h_ch,Ns_ch, pt_start,pt_end, i_sr,t_sr,soil_sig,soil_epr,flag_type);
toc

tic
%[U_rs,Er_rs,Ez_rs] = sr_induced_v_anal_test(pt_hit,h_ch,Ns_ch, pt_start,pt_end, i_sr,t_sr, flag_type);

%[U_rs,Er_rs,Ez_rs] = sr_induced_v_anal(pt_hit,h_ch, pt_start,pt_end, i_sr,t_sr);

[U_rs,Er_rs,Ez_rs] = sr_induced_v_anal_lossy(pt_hit,h_ch, pt_start,pt_end, i_sr,t_sr,soil_sig,soil_epr);
toc
% [U_rs,Er_rs,Ez_rs, U_l,Er_l,Ez_l, U_t,Er_t,Ez_t] = ...
%     sr_induced_v_anal_L_RS(pt_hit,h_ch, pt_start,pt_end, i_sr,t_sr);

Nc = size(pt_start,1);
figure(10)
subplot(1,3,1)
hold on
for ik=1:Nc
    plot(tus,Er_T(ik,:));
end
title('Er num')
xlabel('Time (us)')
subplot(1,3,2)
hold on
for ik=1:Nc
    plot(tus,Ez_T(ik,:));
end
title('Ez num')
xlabel('Time (us)')
subplot(1,3,3)
hold on
for ik=1:Nc
    plot(tus,Uout1(ik,:));
end
title('U num')
xlabel('Time (us)')

figure(11)
subplot(1,3,1)
hold on
for ik=1:Nc
    plot(tus,Er_rs(ik,:));
end
title('Er anal')
xlabel('Time (us)')
subplot(1,3,2)
hold on
for ik=1:Nc
    plot(tus,Ez_rs(ik,:));
end
title('Ez anal')
xlabel('Time (us)')
subplot(1,3,3)
hold on
for ik=1:Nc
    plot(tus,U_rs(ik,:));
end
title('U anal')
xlabel('Time (us)')

figure(22)
hold on
plot(tus,Ha0_T)
title('H field')
xlabel('Time (us)')

% figure(12)
% subplot(1,3,1)
% hold on
% for ik=1:Nc
%     plot(tus,Er_l(ik,:));
% end
% title('Er anal L')
% xlabel('Time (us)')
% subplot(1,3,2)
% hold on
% for ik=1:Nc
%     plot(tus,Ez_l(ik,:));
% end
% title('Ez anal L')
% xlabel('Time (us)')
% subplot(1,3,3)
% hold on
% for ik=1:Nc
%     plot(t_sr,U_l(ik,:));
% end
% title('U anal L')
% xlabel('Time (us)')

% figure(13)
% subplot(1,3,1)
% hold on
% for ik=1:Nc
%     plot(tus,Er_t(ik,:));
% end
% title('Er anal L+RS')
% xlabel('Time (us)')
% subplot(1,3,2)
% hold on
% for ik=1:Nc
%     plot(tus,Ez_t(ik,:));
% end
% title('Ez anal L+RS')
% xlabel('Time (us)')
% subplot(1,3,3)
% hold on
% for ik=1:Nc
%     plot(tus,U_t(ik,:));
% end
% title('U anal L+RS')
% xlabel('Time (us)')