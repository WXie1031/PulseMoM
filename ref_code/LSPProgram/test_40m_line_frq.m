
clear

%% source
sr_type = 0;   %  0 - current source

dt = 1e-7;
Tmax = 1000e-6;
t = (0:dt:Tmax-dt);

% First negative impulse 1/200us k=0.986  T1=1.82  T2=285  100kA
% Subsequent negative impulse 0.25/100us k=0.993  T1=0.454  T2=143  50kA
amp = 50e3;
tau1 = 1.82e-6;
tau2 = 285e-6;
[sr_t, ts] = sr_heidler(amp, tau1, tau2, 5, Tmax, dt, 0);
Nt = ceil(Tmax/dt);

Nf = length(sr_t);
df = 1/Tmax;
sr_f = fft(sr_t);

frq = (0:Nf-1)*df; 
if frq(1) == 0
 	frq(1) = 0.1;
end


dl = [10;];
dl_type = 2;
%    case 1  Ns = ceil(len./dl); % input the interval length
%    case 2  % input the number of the segment
        
        
pt_start = [0   0  0;];
pt_end   = [0   0  40;];

Nc = size(pt_start,1);
shape = 2000*ones(Nc,1);
dim1 = 5e-3*ones(Nc,1);
dim2 = 0*ones(Nc,1);
sig = 5.8e7*ones(Nc,1);
R_pul = 1./(5.8e7.*pi*dim1.^2);
mur = 1*ones(Nc,1);
epr  = 1*ones(Nc,1);
[dv, len] = line_dv(pt_start, pt_end);

grp_id = zeros(Nc,1);
grp_type = zeros(Nc,1);
pt_2d = zeros(Nc,2);
bran_name = [];
nod_start = [];
nod_end = [];
cnt = 0;
for ik = 1:Nc
    bran_name(ik,:) = ['B', num2str(ik, '%.5d')];
    cnt = cnt+1;
    nod_start(ik,:) = ['N', num2str(cnt, '%.5d')];
    cnt = cnt+1;
    nod_end(ik,:) = ['N', num2str(cnt, '%.5d')];
end


flag_p = 1;
[pt_sta_L,pt_end_L,dv_L,re_L,l_L, shape_L,d1_L,d2_L, R_L, sig_L,mur_L,epr_L, ...
    pt_sta_P,pt_end_P,pt_P,re_P,dv_P,l_P, shape_P,d1_P,d2_P, ...
    g_id_L,g_type_L,p_2d_L, bname,nd_start,nd_end, nd_P] = main_mesh_line_3d ...
    (pt_start,pt_end,dv,dim1,len, shape,dim1,dim2, R_pul,sig,mur,epr, ...
    grp_id,grp_type,pt_2d, bran_name,nod_start,nod_end, dl,dl_type, flag_p);

Nb_seg = size(pt_sta_L,1);
Nn_seg = size(pt_sta_P,1);


pt_sta_L_img = pt_sta_L;
pt_sta_L_img(:,3) = -pt_sta_L_img(:,3);
pt_end_L_img = pt_end_L;
pt_end_L_img(:,3) = -pt_end_L_img(:,3);
if flag_p>0
    pt_sta_P_img = pt_sta_P;
    pt_sta_P_img(:,3) = -pt_sta_P_img(:,3);
    pt_end_P_img = pt_end_P;
    pt_end_P_img(:,3) = -pt_end_P_img(:,3);
else
    pt_sta_P_img = [];
    pt_end_P_img = [];
end

dt_td = 1e-12;
[TdL, ~] = sol_td(pt_sta_L, pt_end_L, dt_td);
[TdP, ~] = sol_td(pt_sta_P, pt_end_P, dt_td);
[TdGL, ~] = sol_td_img(pt_sta_L,pt_end_L, pt_sta_L_img,pt_end_L_img, dt_td);
[TdGP, ~] = sol_td_img(pt_sta_P,pt_end_P, pt_sta_P_img,pt_end_P_img, dt_td);

[Rpeec, Lpeec, Ppeec] = para_main_fila_rlp(pt_sta_L, pt_end_L, dv_L, d1_L, l_L, ...
    pt_sta_P, pt_end_P, dv_P, d1_P, l_P, flag_p);
Rpeec = diag(0*ones(Nb_seg,1));

offset = 0;
ftmp = 100e3;
[Rg,Lg,Pg, Rgself,Lgself] = ground_pec(pt_sta_L,pt_end_L,dv_L,re_L,l_L, ...
    pt_sta_P,pt_end_P,dv_P,re_P,l_P, ftmp, offset, flag_p); 


[nod_start, nod_end, nod_P] = sol_peec_brn_nod_create( ...
    pt_sta_L,pt_end_L, pt_P, [],[], [],[], flag_p);


%% set source pt
Rec=[]; Lec=[]; Cec=[]; 
Rec = [5;]; % add a ground resistance


ngnd = max([nod_start; nod_end]) + 1;
% nod_start = [nod_start; nod_end(dl(1),:); nod_end(dl(1)+dl(2),:); nod_end(dl(1)+dl(2)+dl(3),:)];
% nod_end = [nod_end; ngnd; ngnd; ngnd];
nod_start = [nod_start; nod_start(1,:);];
nod_end = [nod_end; ngnd];
% nod_P = [nod_P; ngnd];

% nod_sr = [nod_start(1) nod_start(dl(1)+dl(2)+dl(3)+1)];
% nod_gnd = [ngnd;  nod_end(dl(1)+dl(2)+dl(3)+dl(4))];

nod_ec = [ ngnd;];

nod_sr = [nod_end(dl(1)) ngnd];
nod_gnd =  ngnd;

[Rec,Lec,Cec, Pnew,Pgnew, Amtx, id_sv,id_si, TdPnew,TdGPnew, nod_new] = sol_net_mna_frq_gnd(Lpeec,Ppeec, Pg,...
    Rec,Lec,Cec, nod_P,TdP,TdGP, nod_start,nod_end, nod_gnd, nod_sr, nod_ec, sr_type);


tic

% [Ibf, Vnf] = sol_peec_freq_mna_gnd( Rpeec,Lpeec,Pnew, Rec,Lec,Cec, ...
%     Lg,Pgnew, TdL,TdPnew,TdGL,TdGPnew, Amtx, id_sv, id_si, sr_f, frq );

Ibf = sol_peec_freq_mla_gnd( Rpeec,Lpeec,Pnew, Rec, Lec, Cec, ...
    Lg,Pgnew, TdL,TdPnew,TdGL,TdGPnew, Amtx, id_sv,id_si, sr_f, frq);
% 
% [Ibf, Vnf] = sol_peec_freq_na( Rpeec,Lpeec,Pnew, TdL,TdP, Rec,Lec,Cec, ...
%     Amtx, id_sv,id_si, sr_f, frq );

% Ibt = real(ifft(Ibf,[],2));
Ibt = zeros(Nb_seg,Nt);
for ik = 1:Nb_seg
    Ibt(ik,:) = real(ifft(Ibf(ik,:)));
end

toc
% % correction of the DC current
% idc = -Ibt(:,1);
% Ibt(:,1) = Ibt(:,1)  + idc;



% figure(55)
% plot3(pt_sta_L(:,1),pt_sta_L(:,2),pt_sta_L(:,3),pt_end_L(:,1),pt_end_L(:,2),pt_end_L(:,3),'-x')
% grid on
% 
% figure(56)
% plot3(pt_sta_P(:,1),pt_sta_P(:,2),pt_sta_P(:,3),pt_end_P(:,1),pt_end_P(:,2),pt_end_P(:,3),'-o')
% grid on
% axis equal


figure(61)
hold on
plot(t(1:Nt-1)*1e6, Ibt(dl(1),1:Nt-1)/1e3,'r-','LineWidth',1);
plot(t(1:Nt-1)*1e6, Ibt(dl(1)-ceil(dl(1)*4/9),1:Nt-1)/1e3,'r--','LineWidth',1);
plot(t(1:Nt-1)*1e6, Ibt(dl(1)-ceil(dl(1)*8/9),1:Nt-1)/1e3,'r-.','LineWidth',1);
ylabel('Current(kA)')
xlabel('Time(us)')
xlim([0 10])




