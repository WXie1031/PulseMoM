
clear



dlx = [10;];
dly = [9;];
dl_type = 2;


pt_mid = [0 0 0];

Nc = size(pt_mid,1);
shape = 10001*ones(Nc,1);
dim1 = 0.01*ones(Nc,1);
dim2 = 0.009*ones(Nc,1);

sig = 5e8;
R_pul = 1/sig; % current flow in x,y direction thus, thickness x conductivity 
mur = 1*ones(Nc,1);

dv = ones(Nc,1)*[0 0 1];
frq = [1 20 50 100 200 500 1e3 2e3 5e3 10e3 20e3 50e3 100e3 200e3 500e3];
Nf = length(frq);


p_flag = 1;

[pt_sta_L, pt_end_L, pt_mid_L, dv_L, shape_L,d1_L,d2_L, R_L,sig_L,mur_L, ...
    pt_P, pt_mid_P, dv_P, shape_P,d1_P,d2_P] = mesh_plate_2d ...
    (pt_mid, dv, shape, dim1, dim2, R_pul,sig,mur, dlx, dly, dl_type, p_flag);

% 
figure(55)
hold on 
Nx_tmp = dlx*(dly+1);

% plot(p_mid_Lseg(1:Nx_tmp,1),p_mid_Lseg(1:Nx_tmp,2),'x','MarkerSize',4)
% 
% plot(p_mid_Lseg(Nx_tmp+1:end,1),p_mid_Lseg(Nx_tmp+1:end,2),'+','MarkerSize',4)

% plot(p_sta_Lseg(1:Nx_tmp,1),p_sta_Lseg(1:Nx_tmp,2),'x','MarkerSize',6)
% plot(p_end_Lseg(1:Nx_tmp,1),p_end_Lseg(1:Nx_tmp,2),'x','MarkerSize',6)
% plot(p_mid_Pseg(:,1),p_mid_Pseg(:,2),'o','MarkerSize',6)
% 
% posL = [p_mid_Lseg(:,1)-d1_Lseg/2, p_mid_Lseg(:,2)-d2_Lseg/2, d1_Lseg,d2_Lseg];
% for ik=1:Nx_tmp
% rectangle('Position', posL(ik,:),'LineStyle','--','EdgeColor','b');
% end
% for ik=Nx_tmp+1:size(posL,1)
% rectangle('Position', posL(ik,:),'LineStyle','--','EdgeColor','g');
% end
% 
posP = [pt_mid_P(:,1)-d1_P/2, pt_mid_P(:,2)-d2_P/2, d1_P,d2_P];
for ik=1:size(posP,1)
rectangle('Position', posP(ik,:),'LineStyle','-.','EdgeColor','r');
end
% 
pos = [pt_mid(1)-dim1/2,pt_mid(2)-dim2/2, dim1, dim2];
rectangle('Position', pos, 'LineWidth',1)
% grid on
% hold off
axis equal


Nb = size(pt_mid_L,1);

[Rpeec, Lpeec, Ppeec] = para_main_plate_2d(pt_mid_L, dv_L, d1_L, d2_L, R_L,...
    pt_mid_P, dv_P, d1_P, d2_P, p_flag);


% [Rs1, Ls1] = para_main_self_multi_frq(shape_L, d1_L, d2_L, ...
%     1e-6*ones(Nc,1), R_pul, sig, mur*ones(1,Nf), frq);

%% source
sr_type = 0;
dlavg = max(max(d1_P), max(d2_P));
dt =sqrt(2)*dlavg/3e8;
dt = 2e-11;

%dt = 0.01e-9;
Tmax = 1e-9;
Nt = ceil(Tmax/dt);
t = (0:dt:Tmax);

sr_t = sr_upulse(t,90e-12,30e-12,30e-12);


[nod_start, nod_end, nod_P] = sol_peec_brn_nod_create( ...
    pt_sta_L,pt_end_L, pt_P, [],[], [],[], p_flag);

% nod_start = 5000+[(1:Nb)]';
% nod_end   = 5000+[(2:Nb+1)]';

nod_sr = [nod_start(1)  0];
nod_gnd =  [0];

Rfit = [];
Lfit = [];
% Rpeec = diag(0*ones(Nseg,1));

[~, id_TdL] = sol_peec_td(pt_mid_L, dt);

[~, id_TdP] = sol_peec_td(pt_mid_P, dt);

Rec=[];Lec=[]; Cec=[]; 
[Rec,Lec,Cec, Cpeec,Cs,Speec, Amtx, id_sv, id_si, id_TdP] = sol_net_mna_td(Lpeec,Ppeec, ...
    Rfit,Lfit, Rec,Lec,Cec, nod_P,id_TdP, nod_start,nod_end, nod_gnd, nod_sr, sr_type);

disp('Start Calculationg');
tic
[Ibt, Vnt] = sol_peec_time_mna_td( Rpeec,Lpeec, Cs,Speec, ...
    Rec,Lec,Cec, id_TdL,id_TdP, Amtx, id_sv,id_si, sr_t, dt, Nt );

% [Ibt, Vnt] = sol_peec_time_mna_tr( Rpeec, Lpeec, Cpeec,  ...
%     Amtx, id_sv, id_si, sr_t, dt, Nt );

% [Ibt, Vnt] = sol_peec_time_mna_bu( Rpeec, Lpeec, Cpeec,  ...
%     Amtx, id_sv, id_si, sr_t, dt, Nt );
toc
disp('End Calculationg');

figure(61)
hold on
plot(t(1:Nt-1), Ibt(1,1:Nt-1));
plot(t(1:Nt-1), Ibt(15,1:Nt-1));
plot(t(1:Nt-1), Ibt(30,1:Nt-1));
plot(t(1:Nt-1), Ibt(45,1:Nt-1));



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

