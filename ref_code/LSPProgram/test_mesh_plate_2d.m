
clear


frq = [1 20 50 100 200 500 1e3 2e3 5e3 10e3 20e3 50e3 100e3 200e3 500e3];
Nf = length(frq);


dlx = [20;];
dly = [10;];
dl_type = 2;


pt_mid = [0 0 0;];  % center of the plate - multi-plate is supported

Nc = size(pt_mid,1);
shape = 1001*ones(Nc,1);
dim1 = 0.05*ones(Nc,1);
dim2 = 0.01*ones(Nc,1);

dim1 = 2*ones(Nc,1);
dim2 = 1*ones(Nc,1);

R_pul = 1e-4*ones(Nc,1);
mur = 1*ones(Nc,1);
sig = R_pul.*(dim1.*dim2);
dv = ones(Nc,1)*[0 0 1];



p_flag = 1;

[pt_sta_L, pt_end_L, pt_mid_L, dv_L, shape_L,d1_L,d2_L, R_L, sig_L,mur_L, ...
    pt_P, pt_mid_P, dv_P, shape_P,d1_P,d2_P, Nx,Ny] = mesh_plate_2d ...
    (pt_mid, dv, shape, dim1, dim2, R_pul, sig, mur, dlx, dly, dl_type, p_flag);
Nb = size(pt_mid_L,1);

figure(55)
hold on
Nx_tmp = dlx*(dly+1);

plot(pt_mid_L(1:Nx_tmp,1),pt_mid_L(1:Nx_tmp,2),'x','MarkerSize',4)
plot(pt_mid_L(Nx_tmp+1:end,1),pt_mid_L(Nx_tmp+1:end,2),'+','MarkerSize',4)

for ik=1:Nb
    arrowPlot([pt_sta_L(ik,1),pt_end_L(ik,1)],[pt_sta_L(ik,2),pt_end_L(ik,2)], 'number', 1)
end

hold on
plot(pt_sta_L(1:Nx_tmp,1),pt_sta_L(1:Nx_tmp,2),'s','MarkerSize',6)
plot(pt_end_L(1:Nx_tmp,1),pt_end_L(1:Nx_tmp,2),'s','MarkerSize',6)

posL = [pt_mid_L(:,1)-d1_L/2, pt_mid_L(:,2)-d2_L/2, d1_L,d2_L];
for ik=1:Nx_tmp
    rectangle('Position', posL(ik,:),'LineStyle','--','EdgeColor','b');
end
for ik=Nx_tmp+1:size(posL,1)
    rectangle('Position', posL(ik,:),'LineStyle','--','EdgeColor','g');
end

if p_flag>0
    plot(pt_mid_P(:,1),pt_mid_P(:,2),'o','MarkerSize',6)
    posP = [pt_mid_P(:,1)-d1_P/2, pt_mid_P(:,2)-d2_P/2, d1_P,d2_P];
    for ik=1:size(posP,1)
        rectangle('Position', posP(ik,:),'LineStyle','-.','EdgeColor','r');
    end
    pos = [pt_mid(1)-dim1/2,pt_mid(2)-dim2/2, dim1, dim2];
    rectangle('Position', pos, 'LineWidth',1)
end
% grid on
% hold off
axis equal


[Rpeec, Lpeec, Ppeec] = para_main_plate_2d(pt_mid_L, dv_L, d1_L, d2_L, R_L,...
    pt_mid_P, dv_P, d1_P, d2_P, p_flag);
Lpeec=full(Lpeec);
Ppeec=full(Ppeec);


%% source
sr_type = 0;
dlavg = max(max(d1_P), max(d2_P));
dt =10*dlavg/3e8;
tus = 0:dt:10e-6;
sr_t = 5e9.*tus;
Nt = length(tus);

dt = 0.0001e-7;
Tmax = 0.01e-6;
[sr_t, tus, ts] = sr_heidler(1000, 0.0001e-6, 1e-6, 5, Tmax*1.2, dt, 0e-6);
Nt = length(sr_t);
% tus = (0:dt:Tmax);

figure(66)
plot(tus, sr_t)
%
% Sig_pul_cs2D=5e8;
% S = pi*(dim1.^2-dim2.^2);
% R_pul = 1./( Sig_pul_cs2D.*S);
% [Rs1, Ls1] = para_main_self_multi_frq(shape_Lseg(1), d1_Lseg(1), d2_Lseg(1), ...
%     l_Lseg(1), R_pul, Sig_pul_cs2D, mur*ones(1,Nf), frq);
%
% Nseg = size(p_mid_Lseg,1);
% [Rs, Ls, Rn, Ln] = vectfit_main_Z(Rs1(1,1:Nf),Ls1(1,1:Nf),frq, 2);
% Lpeec = Lpeec+diag(Ls*ones(Nseg,1)-diag(Lpeec));
% Rpeec = diag(Rs*ones(Nseg,1));
% Rfit = ones(Nseg,1)*Rn;
% Lfit = ones(Nseg,1)*Ln;

[nod_start, nod_end, nod_P] = sol_peec_brn_nod_create( ...
    pt_sta_L,pt_end_L, pt_P, [],[], [],[], p_flag);

% nod_start = 5000+[(1:Nb)]';
% nod_end   = 5000+[(2:Nb+1)]';

nod_sr = [nod_start(1)  nod_end(end)];
nod_gnd =  [nod_end(end)];

Rfit = [];
Lfit = [];


[~, id_TdL] = sol_td(pt_mid_L, dt);
[~, id_TdP] = sol_td(pt_mid_P, dt);

Rec=[];Lec=[]; Cec=[]; nod_ec=[];
[Rec,Lec,Cec, Pnew, Cs,Ss, Amtx, id_sv, id_si, id_TdP] = sol_net_mna_td(Lpeec,Ppeec, ...
    Rfit,Lfit, Rec,Lec,Cec, nod_P,id_TdP, nod_start,nod_end, nod_gnd, nod_sr, nod_ec, sr_type);
Amtx=full(Amtx);

disp('Start Calculationg');
tic
[Ibt, Vnt] = sol_peec_time_mna_td( Rpeec,Lpeec, Cs,Ss, ...
    Rec,Lec,Cec, id_TdL,id_TdP, Amtx, id_sv,id_si, sr_t, dt, Nt, p_flag );

% [Ibt, Vnt] = sol_peec_time_mna_tr( Rpeec, Lpeec, Cpeec,  ...
%     Amtx, id_sv, id_si, sr_t, dt, Nt );

% [Ibt, Vnt] = sol_peec_time_mna_bu( Rpeec, Lpeec, Cpeec,  ...
%     Amtx, id_sv, id_si, sr_t, dt, Nt );
toc
disp('End Calculationg');

figure(61)
hold on
plot(tus(1:Nt-1), Ibt(1,1:Nt-1));
plot(tus(1:Nt-1), Ibt(15,1:Nt-1));
plot(tus(1:Nt-1), Ibt(30,1:Nt-1));
plot(tus(1:Nt-1), Ibt(45,1:Nt-1));



%plot3( p_sta_L(:,1),p_sta_L(:,2), Ibt(:,20),'.')

%
% h=figure;
% M=moviein(length(1:5:Nt));
% cnt=1;
% for jj=1:5:Nt
%
%     Z = griddata(p_mid_L(:,1),p_mid_L(:,2),Ibt(:,jj),p_mid_L(:,1),p_mid_L(:,2),'v4');%²åÖµ
%     set(h,'zdata',Z);
%     axis equal;
%     drawnow
%     %pause(0.02)
%     M(cnt) = getframe;
%     cnt=cnt+1;
% end
%
% movie(M);

