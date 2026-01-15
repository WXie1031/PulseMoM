clear
% 
pt_start = [0,0,10];
pt_end = [100,0,10];

% pt_start = [200.1,0,10;  400.1,0,10;  200.1,0,6;  400.1,0,6];
% pt_end = [199.9,0,10;  399.9,0,10; 199.9,0,6;  399.9,0,6];

dpt = 0.01;
pt_start = [100-dpt,0-dpt,0;
    100-dpt,0-dpt,6-dpt;
    100-dpt,0-dpt,10-dpt;
    200-dpt,0-dpt,6-dpt;
    500-dpt,0-dpt,6-dpt;
    1500-dpt,0-dpt,6-dpt;];
pt_end = [100+dpt,0+dpt,0+2*dpt;
    100-dpt,0-dpt,6+dpt;
    100+dpt,0+dpt,10+dpt;
    200+dpt,0+dpt,6+dpt;
    500+dpt,0+dpt,6+dpt;
    1500+dpt,0+dpt,6+dpt;];

pt_nod = (pt_start+pt_end)/2;
Imax = 1e4;

Tmax = 30; % us
dt = 0.1; % us

tus = (0:dt:Tmax);
t_sr = tus*1e-6;

alpha1=3e4;
alpha2=1e7;
i_sr=Imax*(exp(-alpha1*t_sr)-exp(-alpha2*t_sr)); 


Nt = length(tus);

pt_hit = [0 0];
Ns_ch = 4000;
%h_ch = 5e3;
h_ch = 7e3;

soil_sig = 0.01;
soil_epr = 10;
flag_type = 1; % TL 

tic
%[Uout1,Er_T,Ez_T] = sr_induced_e_num(pt_hit,h_ch,Ns_ch, pt_start,pt_end, i_sr, t_sr,2);
[Ur1,Uz1,Er_T,Ez_T,Ha0_T] = sr_induced_e_num_lossy(pt_hit,h_ch,Ns_ch, pt_start,pt_end, pt_nod,i_sr,t_sr,soil_sig,soil_epr,flag_type);
toc

tic
%[U_rs,Er_rs,Ez_rs] = sr_induced_v_anal_test(pt_hit,h_ch,Ns_ch, pt_start,pt_end, i_sr,t_sr, 1);
%[U_rs,Er_rs,Ez_rs] = sr_induced_v_anal(pt_hit,h_ch, pt_start,pt_end, i_sr,t_sr);
[U_rs,Er_rs,Ez_rs] = sr_induced_v_anal_lossy(pt_hit,h_ch, pt_start,pt_end, i_sr,t_sr,soil_sig,soil_epr);
toc
% [U_rs,Er_rs,Ez_rs, U_l,Er_l,Ez_l, U_t,Er_t,Ez_t] = ...
%     sr_induced_v_anal_L_RS(pt_hit,h_ch, pt_start,pt_end, i_sr,t_sr);

Nc = size(pt_start,1);
figure(10)
subplot(2,2,1)
hold on
for ik=1:Nc
    plot(tus,Er_T(ik,:));
end
title('Er num')
xlabel('Time (us)')
subplot(2,2,2)
hold on
for ik=1:Nc
    plot(tus,Ez_T(ik,:)+27.8e3);
end
title('Ez num')
xlabel('Time (us)')
subplot(2,2,3)
hold on
for ik=1:Nc
    plot(tus,Ur1(ik,:));
end
title('Ur num')
xlabel('Time (us)')
subplot(2,2,4)
hold on
for ik=1:Nc
    plot(tus,Uz1(ik,:));
end
title('Uz num')
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

% figure(22)
% hold on
% plot(tus,Ha0_T)
% title('H field')
% xlabel('Time (us)')


% figure(12)
% subplot(1,3,1)
% hold on
% for ik=1:Nc
%     plot(Er_l(ik,:));
% end
% title('Er anal L')
% subplot(1,3,2)
% hold on
% for ik=1:Nc
%     plot(Ez_l(ik,:));
% end
% title('Ez anal L')
% subplot(1,3,3)
% hold on
% for ik=1:Nc
%     plot(U_l(ik,:));
% end
% title('U anal L')
% 
% figure(13)
% subplot(1,3,1)
% hold on
% for ik=1:Nc
%     plot(Er_t(ik,:));
% end
% title('Er anal L+RS')
% subplot(1,3,2)
% hold on
% for ik=1:Nc
%     plot(Ez_t(ik,:));
% end
% title('Ez anal L+RS')
% subplot(1,3,3)
% hold on
% for ik=1:Nc
%     plot(U_t(ik,:));
% end
% title('U anal L+RS')