
clear


dl = [60,120,120]*5;
dl_type = 2;


% load('./cirsolver/data_pulsetest4ns.mat');
% sr_t = I(:,end)';
% tus = T;
% Nt = size(tus,1);
% dt = tus(2)*1e-6;
% dt = 8e-9;
% tus = 0:dt:0.8e-6;
% sr_t = 5e9.*tus;
% Nt = length(tus);

pt_start = [0 0 0;
            0 0 60.1;
            0 0 180.1];
pt_end   = [0 0 60;
            0 0 180.1; 
            0 0  300.1;];

Nc = size(pt_start,1);
shape = 1001*ones(Nc,1);
dim1 = [5e-3;5e-3; 20e-3];
dim2 = 0*ones(Nc,1);
R_pul = 0*ones(Nc,1);
mur = 1*ones(Nc,1);
re = dim1;
sig = 1./(R_pul.*pi.*re.^2);


[dv, len] = line_dv(pt_start, pt_end);

frq = [1 20 50 100 200 500 1e3 2e3 5e3 10e3 20e3 50e3 100e3 200e3 500e3];
Nf = length(frq);


p_flag = 1;

[pt_sta_L, pt_end_L, dv_L,re_L l_L, shape_L,d1_L,d2_L, R_L, sig_L,mur_L, ...
    pt_sta_P, pt_end_P, pt_P, re_P, dv_P, l_P, shape_P,d1_P,d2_P] ...
    = mesh_line_3d(pt_start,pt_end,dv,re,len,shape,dim1,dim2,R_pul,sig, mur, dl, dl_type, p_flag);
Nb = size(pt_sta_L,1);

[Rpeec, Lpeec, Ppeec] = para_main_fila_rlp(pt_sta_L, pt_end_L, dv_L, d1_L, l_L, ...
    pt_sta_P, pt_end_P, dv_P, d1_P, l_P, p_flag);


%% source
sr_type = 0;
dlavg = max(l_L);
dt = 2.01*dlavg/3e8;
tus = 0:dt:5e-6;
sr_t = 5e9.*tus;
Nt = length(tus);

% Tmax = 4e-6;
% [sr_t, ~] = sr_heidler(1000, 0.06e-6, 95e-6, 5, Tmax*1.2, dt, 0e-6);
% Nt = ceil(Tmax/dt);
% tus = (0:dt:Tmax);



Sig_pul = 5e8;
S = pi*(dim1.^2-dim2.^2);
R_pul = 1./( Sig_pul.*S);
[Rs1, Ls1] = para_self_multi_frq(shape_L(1), d1_L(1), d2_L(1), ...
    l_L(1), R_pul, Sig_pul, mur*ones(1,Nf), frq);

Nseg = size(pt_sta_L,1);
[Rs, Ls, Rn, Ln] = main_vectfit_z(Rs1(1,1:Nf),Ls1(1,1:Nf),frq, 2, Lpeec);
Lpeec = Lpeec+diag(Ls*ones(Nseg,1)-diag(Lpeec));
Rpeec = diag(Rs*ones(Nseg,1));
Rfit = ones(Nseg,1)*Rn;
Lfit = ones(Nseg,1)*Ln;


[nod_start, nod_end, nod_P] = sol_peec_brn_nod_create( ...
    pt_sta_L,pt_end_L, pt_P, [],[], [],[], p_flag);

% nod_start = 5000+[(1:dl(1))  ( (dl(1)+2):(dl(1)+dl(2)+1)  )  ( (dl(1)+dl(2)+2) : (dl(1)+dl(2)+dl(3)+1) )]';
% nod_end   = 5000+[(2:dl(1)+1)  ( dl(1)+3:(dl(1)+dl(2)+2) )    ( (dl(1)+dl(2)+2) : (dl(1)+dl(2)+dl(3)+2) )]';

nod_sr = [dl(1)+1  dl(1)+2];
nod_gnd = [nod_start(1);  nod_end(end)];

Rfit = [];
Lfit = [];
Rpeec = diag(0*ones(Nseg,1));



[~, id_TdL] = sol_peec_td(pt_sta_L, pt_end_L, dt);
[~, id_TdP] = sol_peec_td(pt_sta_P, pt_end_P, dt);

Rec=[];Lec=[]; Cec=[]; 
[Rec,Lec,Cec,Ctmp, Cpeec,Speec, Amtx, id_sv, id_si, id_TdP] = sol_net_mna_td(Lpeec,Ppeec, ...
    Rfit,Lfit, Rec,Lec,Cec, nod_P,id_TdP, nod_start,nod_end, nod_gnd, nod_sr, sr_type);
disp('Start Calculationg');
tic
[Ibt, Vnt] = sol_peec_time_mna_td( Rpeec, Lpeec, Cpeec, Speec, ...
    Rec, Lec, Cec, id_TdL, id_TdP, Amtx, id_sv, id_si, sr_t, dt, Nt );

toc
disp('End Calculationg');

% figure(55)
% plot3(p_sta_Lseg(:,1),p_end_Lseg(:,2),p_end_Lseg(:,3),p_end_Lseg(:,1),p_end_Lseg(:,2),p_end_Lseg(:,3),'-x')
% grid on
% 
% figure(56)
% plot3(p_sta_Pseg(:,1),p_sta_Pseg(:,2),p_sta_Pseg(:,3),p_end_Pseg(:,1),p_end_Pseg(:,2),p_end_Pseg(:,3),'-o')
% grid on

figure(61)
hold on
plot(tus(1:Nt-1), Ibt(dl(1)+floor(30/120*dl(2)),1:Nt-1));
plot(tus(1:Nt-1), Ibt(dl(1)+floor(60/120*dl(2)),1:Nt-1));
grid on

fname = '3line.mat';
save('3line', 'tus', 'dt', 'Ibt');

% dN = 10;
% figure(62)
% hold on
% plot(tus(1:Nt-1-dN), Ibt(1,dN+1:Nt-1));
% plot(tus(1:Nt-1-dN*2), Ibt(10,dN*2+1:Nt-1));
% plot(tus(1:Nt-1-dN*3), Ibt(20,dN*3+1:Nt-1));
% plot(tus(1:Nt-1-dN*4), Ibt(30,dN*4+1:Nt-1));
% plot(tus(1:Nt-1-dN*4), Ibt(30,dN*4+1:Nt-1));
% plot(tus(1:Nt-1-dN*4), Ibt(180,dN*4+1:Nt-1));

