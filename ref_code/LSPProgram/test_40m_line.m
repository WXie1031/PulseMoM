
clear


dl = [20;]; % number of segment
dt = 1e-8;



dl_type = 2;
%    case 1  Ns = ceil(len./dl); % input the interval length
%    case 2  % input the number of the segment
        
        
pt_start = [0   0 0;];
pt_end   = [0   0 100;];


Nc = size(pt_start,1);
shape = 2000*ones(Nc,1);
dim1 = 5e-3*ones(Nc,1);
dim2 = 0*ones(Nc,1);
sig = 5.8e7*ones(Nc,1);
epr = 1*ones(Nc,1);
R_pul = 1./(5.8e7.*pi*dim1.^2);
mur = 1*ones(Nc,1);
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


%% source
sr_type = 0;   %  0 - current source
 

Tmax = 10e-6;

t = (0:dt:Tmax);
Nt = length(t);

% First negative impulse 1/200us k=0.986  T1=1.82  T2=285  100kA
% Subsequent negative impulse 0.25/100us k=0.993  T1=0.454  T2=143  50kA
amp = 50e3;
tau1 = 1.82e-6;
tau2 = 285e-6;
% 
% tau1 =0.454e-6;
% tau2 = 143e-6;
[sr_t, tus] = sr_heidler(amp, tau1, tau2, 5, Tmax, dt, 0);
Nt = ceil(Tmax/dt);


flag_p = 1;
[pt_sta_L,pt_end_L,dv_L,re_L,l_L, shape_L,d1_L,d2_L, R_L, sig_L,mur_L,epr_L, ...
    pt_sta_P,pt_end_P,pt_P,re_P,dv_P,l_P, shape_P,d1_P,d2_P, ...
    g_id_L,g_type_L,p_2d_L, bname,nd_start,nd_end, nd_P] = main_mesh_line_3d ...
    (pt_start,pt_end,dv,dim1,len, shape,dim1,dim2, R_pul,sig,mur, epr,...
    grp_id,grp_type,pt_2d, bran_name,nod_start,nod_end, dl,dl_type, flag_p);

Nb = size(pt_sta_L,1);

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
[~, id_TdL] = sol_td(pt_sta_L, pt_end_L, dt);
[~, id_TdP] = sol_td(pt_sta_P, pt_end_P, dt);
[~, id_TdGL] = sol_td_img(pt_sta_L,pt_end_L, pt_sta_L_img,pt_end_L_img, dt);
[~, id_TdGP] = sol_td_img(pt_sta_P,pt_end_P, pt_sta_P_img,pt_end_P_img, dt);

[Rpeec, Lpeec, Ppeec] = para_main_fila_rlp(pt_sta_L, pt_end_L, dv_L, d1_L, l_L, ...
    pt_sta_P, pt_end_P, dv_P, d1_P, l_P, flag_p);
offset = 0;
frq = 100e3;
[Rg,Lg,Pg, Rgself,Lgself] = ground_pec(pt_sta_L,pt_end_L,dv_L,re_L,l_L, ...
    pt_sta_P,pt_end_P,dv_P,re_P,l_P, frq, offset, flag_p); 


Nseg = size(pt_sta_L,1);
Rpeec = diag(0*ones(Nseg,1));
Rfit = [];
Lfit = [];

% Rpeec = diag(0*ones(Nseg,1));
% Rfit = [];
% Lfit = [];


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

[Rec,Lec,Cec, Pnew,Pgnew, Amtx, id_sv,id_si, id_TdP,id_TdGP] = sol_net_mna_td_gnd(Lpeec,Ppeec, Pg,...
    Rfit,Lfit, Rec,Lec,Cec, nod_P,id_TdP,id_TdGP, nod_start,nod_end, nod_gnd, nod_sr, nod_ec, sr_type);


tic
[Ibt, Vnt] = sol_peec_time_mna_td_gnd( Rpeec,Lpeec,Pnew, ...
    Rec,Lec,Cec, -Lg,-Pgnew, id_TdL,id_TdP, id_TdGL,id_TdGP, ...
    Amtx, id_sv,id_si, sr_t, dt, Nt );
toc

% figure(55)
% plot3(pt_sta_L(:,1),pt_sta_L(:,2),pt_sta_L(:,3),pt_end_L(:,1),pt_end_L(:,2),pt_end_L(:,3),'-x')
% grid on
% 

figure(56)
plot3(pt_sta_P(:,1),pt_sta_P(:,2),pt_sta_P(:,3),pt_end_P(:,1),pt_end_P(:,2),pt_end_P(:,3),'-o')
grid on
axis equal


figure(61)
hold on
plot(t(1:Nt-1)*1e6, Ibt(dl(1),1:Nt-1)/1e3,'k-','LineWidth',1);
plot(t(1:Nt-1)*1e6, Ibt(dl(1)-ceil(dl(1)/2),1:Nt-1)/1e3,'k--','LineWidth',1);
plot(t(1:Nt-1)*1e6, Ibt(1,1:Nt-1)/1e3,'k-.','LineWidth',1);
ylabel('Current(kA)')
xlabel('Time(us)')
legend('Top','Mid','Bottom')




