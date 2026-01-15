clear
% 
a=0.5;
pt_start = [5000,0,a-0.001;
   ];
pt_end = [5000,0,a+0.001;
    ];


% total source = source1 + source2
% us
Tmax = 1000;
dt = 0.1;

%% source 1
sr_amp = 100e3;
sr_tau1 = 10;
sr_tau2 = 30;
n = 2;
cof1 = exp(-(sr_tau1/sr_tau2)*(n*sr_tau2/sr_tau1)^(1/n));

[ist1, tus] = sr_heidler(sr_amp,sr_tau1,sr_tau2,n,Tmax,dt);

i_sr = ist1*cof1;




soil_sig=3.5e-3;
soil_epr=10;

t_sr = tus*1e-6;
Nt = length(tus);
% i_sr = ones(1,Nt);
% i_sr(1)=0;
pt_hit = [0 0];
Ns_ch = 200;
%h_ch = 5e3;
h_ch = 2e3;

flag_type = 3;

% iout = sr_l_ch_i(h_ch,Ns_ch, i_sr,t_sr, flag_type);

tic


[Uout1,Er_T,Ez_T] = sr_induced_e_num_backup(pt_hit,h_ch,Ns_ch, pt_start,pt_end, i_sr, t_sr,flag_type);
% [Uout1,Er_T,Ez_T] = sr_induced_e_num(pt_hit,h_ch,Ns_ch, pt_start,pt_end, i_sr, t_sr,flag_type);
%[Uout1,Er_T,Ez_T,Ha0_T] = sr_induced_e_num_lossy(pt_hit,h_ch,Ns_ch, pt_start,pt_end, i_sr,t_sr,soil_sig,soil_epr,flag_type);
toc


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

