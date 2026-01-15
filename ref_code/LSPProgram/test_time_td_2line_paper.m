
clear

f0 = 5e6;


dl = [20;20;3];
dl_type = 2;


pt_start = [0     0 1;
            30e-3 0 1;
            0     200e-3 1;];
pt_end   = [0     200e-3 1;
            30e-3 200e-3 1;
            30e-3 200e-3 1;];

Nc = size(pt_start,1);
shape = 1002*ones(Nc,1);
dim1 = 3e-3*ones(Nc,1);
dim2 = 35e-6*ones(Nc,1);
sig = 5e8*ones(Nc,1);
R_pul = 1./(5e8.*dim1.*dim2);
mur = 1*ones(Nc,1);


%% source
sr_type = 1;


dt = 1e-10;
Tmax = 20e-9;
Nt = ceil(Tmax/dt);
t = (0:dt:Tmax);

sr_t = sr_upulse(t,25e-9,100e-12);



[dv, len] = line_dv(pt_start, pt_end);

frq = [1 20 50 100 200 500 1e3 2e3 5e3 10e3 20e3 50e3 100e3 200e3 500e3]*1e4;
Nf = length(frq);


p_flag = 1;

[pt_sta_L, pt_end_L, dv_L, re_L, l_L, shape_L,d1_L,d2_L, R_L, sig_L, mur_L, ...
    pt_sta_P, pt_end_P, pt_P,re_P, dv_P, l_P, shape_P,d1_P,d2_P] ...
    = mesh_line_3d(pt_start, pt_end, dv, dim1, len, shape, dim1, dim2, ...
    R_pul, sig, mur, dl, dl_type, p_flag);
Nb = size(pt_sta_L,1);

[Rpeec, Lpeec, Ppeec] = para_main_fila_rlp(pt_sta_L, pt_end_L, dv_L, d1_L, l_L, ...
    pt_sta_P, pt_end_P, dv_P, d1_P, l_P, p_flag);



[Rs1, Ls1] = para_self_multi_frq(shape_L(1), d1_L(1), d2_L(1), ...
    l_L(1), R_pul(1), sig(1), mur(1)*ones(1,Nf), frq);

Nseg = size(pt_sta_L,1);
[Rs, Ls, Rn, Ln] = vectfit_main_Z(Rs1(1,1:Nf),Ls1(1,1:Nf),frq, 2);
Lpeec = Lpeec+diag(Ls*ones(Nseg,1)-diag(Lpeec));
Rpeec = diag(Rs*ones(Nseg,1));
Rfit = ones(Nseg,1)*Rn;
Lfit = ones(Nseg,1)*Ln;



[nod_start, nod_end, nod_P] = sol_peec_brn_nod_create( ...
    pt_sta_L,pt_end_L, pt_P, [],[], [],[], p_flag);

nod_sr = [1 dl(1)+2];
nod_gnd =  [dl(1)+2 ];

Rfit = [];
Lfit = [];
% Rpeec = diag(0*ones(Nseg,1));

[~, id_TdL] = sol_peec_td(pt_sta_L, pt_end_L, dt);

[~, id_TdP] = sol_peec_td(pt_sta_P, pt_end_P, dt);

Rec=[];Lec=[]; Cec=[]; 
[Rec,Lec,Cec,  Cpeec, Cs,Speec, Amtx, id_sv, id_si, id_TdP] = sol_net_mna_td(Lpeec,Ppeec, ...
    Rfit,Lfit, Rec,Lec,Cec, nod_P,id_TdP, nod_start,nod_end, nod_gnd, nod_sr, sr_type);

tic
[Ibt, Vnt] = sol_peec_time_mna_td( Rpeec, Lpeec, Cs, Speec, ...
    Rec, Lec, Cec, id_TdL, id_TdP, Amtx, id_sv, id_si, sr_t, dt, Nt );

toc


% figure(55)
% plot3(p_sta_Lseg(:,1),p_end_Lseg(:,2),p_end_Lseg(:,3),p_end_Lseg(:,1),p_end_Lseg(:,2),p_end_Lseg(:,3),'-x')
% grid on
% 
% figure(56)
% plot3(p_sta_Pseg(:,1),p_sta_Pseg(:,2),p_sta_Pseg(:,3),p_end_Pseg(:,1),p_end_Pseg(:,2),p_end_Pseg(:,3),'-o')
% grid on

figure(61)
hold on
plot(t(1:Nt-1), Ibt(1,1:Nt-1));
plot(t(1:Nt-1), Ibt(15,1:Nt-1));
plot(t(1:Nt-1), Ibt(30,1:Nt-1));
% plot(t(1:Nt-1), Ibt(45,1:Nt-1));

% dN = 10;
% figure(62)
% hold on
% plot(tus(1:Nt-1-dN), Ibt(1,dN+1:Nt-1));
% plot(tus(1:Nt-1-dN*2), Ibt(10,dN*2+1:Nt-1));
% plot(tus(1:Nt-1-dN*3), Ibt(20,dN*3+1:Nt-1));
% plot(tus(1:Nt-1-dN*4), Ibt(30,dN*4+1:Nt-1));
% plot(tus(1:Nt-1-dN*4), Ibt(30,dN*4+1:Nt-1));
% plot(tus(1:Nt-1-dN*4), Ibt(180,dN*4+1:Nt-1));

