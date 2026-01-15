
clear


f0 = 5e6;


dlx = [49;];
dly = [49;];
dlz = [1;];
dl_type = 2;


pt_mid = [0 0 0;
            ];
pt_end   = [0 0 10;
            ];

Nc = size(pt_mid,1);
shape = 1001*ones(Nc,1);
dim1 = 0.5*ones(Nc,1);
dim2 = 0.5*ones(Nc,1);
dim3 = 0.002*ones(Nc,1);
R_pul = 1e-4*ones(Nc,1);
mur = 1*ones(Nc,1);
epr = 1*ones(Nc,1);
S = dim1.*dim2.*dim3;
sig = 1/(R_pul.*S);

[dv, len] = line_dv(pt_mid, pt_end);

frq = [1 20 50 100 200 500 1e3 2e3 5e3 10e3 20e3 50e3 100e3 200e3 500e3];
Nf = length(frq);


p_flag = 1;

[pt_sta_L, pt_end_L, pt_mid_L, dv_L, ...
    shpid_L,d1_L,d2_L,d3_L, R_L, sig_L,mur_L,epr_L, ...
    pt_P, pt_mid_P, dv_P, shape_P,d1_P,d2_P] ...
    = mesh_plate_3d(pt_mid, dv, shape, dim1, dim2, dim3, R_pul,  sig,mur,epr, ...
    dlx, dly, dlz, dl_type, p_flag);

% 
x_plot = 0;
y_plot = 0;
z_plot = 0;
p_plot = 1;
mid_plot = 0;

figure(55)
hold on 
Nx_tmp = dlx*(dly+1)*(dlz+1);
Ny_tmp = (dlx+1)*(dly)*(dlz+1);
Nz_tmp = (dlx+1)*(dly+1)*(dlz);

if mid_plot==1
plot3(pt_mid_L(1:Nx_tmp,1),pt_mid_L(1:Nx_tmp,2), pt_mid_L(1:Nx_tmp,3),'x','MarkerSize',4)
plot3(pt_mid_L(Nx_tmp+(1:Ny_tmp),1),pt_mid_L(Nx_tmp+(1:Ny_tmp),2),pt_mid_L(Nx_tmp+(1:Ny_tmp),3),'+','MarkerSize',4)
plot3(pt_mid_L(Nx_tmp+Ny_tmp+(1:Nz_tmp),1),pt_mid_L(Nx_tmp+Ny_tmp+(1:Nz_tmp),2),pt_mid_L(Nx_tmp+Ny_tmp+(1:Nz_tmp),3),'^','MarkerSize',4)
end

if x_plot == 1
plot3(pt_sta_L(1:Nx_tmp,1),pt_sta_L(1:Nx_tmp,2),pt_sta_L(1:Nx_tmp,3),'x','MarkerSize',6)
plot3(pt_end_L(1:Nx_tmp,1),pt_end_L(1:Nx_tmp,2),pt_end_L(1:Nx_tmp,3),'x','MarkerSize',6)
end

if y_plot == 1
plot3(pt_sta_L(Nx_tmp+(1:Ny_tmp),1),pt_sta_L(Nx_tmp+(1:Ny_tmp),2),pt_sta_L(Nx_tmp+(1:Ny_tmp),3),'o','MarkerSize',6)
plot3(pt_end_L(Nx_tmp+(1:Ny_tmp),1),pt_end_L(Nx_tmp+(1:Ny_tmp),2),pt_end_L(Nx_tmp+(1:Ny_tmp),3),'o','MarkerSize',6)
end

if z_plot == 1
plot3(pt_sta_L(Nx_tmp+Ny_tmp+(1:Nz_tmp),1),pt_sta_L(Nx_tmp+Ny_tmp+(1:Nz_tmp),2),pt_sta_L(Nx_tmp+Ny_tmp+(1:Nz_tmp),3),'+','MarkerSize',6)
plot3(pt_end_L(Nx_tmp+Ny_tmp+(1:Nz_tmp),1),pt_end_L(Nx_tmp+Ny_tmp+(1:Nz_tmp),2),pt_end_L(Nx_tmp+Ny_tmp+(1:Nz_tmp),3),'+','MarkerSize',6)
end

if p_plot==1
%plot3(pt_mid_P(:,1),pt_mid_P(:,2),pt_mid_P(:,3),'v','MarkerSize',6)
end

posL = [pt_mid_L(:,1)-d1_L/2, pt_mid_L(:,2)-d2_L/2, d1_L,d2_L];
for ik=1:Nx_tmp
rectangle('Position', posL(ik,:),'LineStyle','--','EdgeColor','b');
end
% for ik=Nx_tmp+1:size(posL,1)
% rectangle('Position', posL(ik,:),'LineStyle','--','EdgeColor','g');
% end
% 
% posP = [pt_mid_P(:,1)-d1_P/2, pt_mid_P(:,2)-d2_P/2, d1_P,d2_P];
% for ik=1:size(posP,1)
% rectangle('Position', posP(ik,:),'LineStyle','-.','EdgeColor','r');
% end
% 
pos = [pt_mid(1)-dim1/2,pt_mid(2)-dim2/2, dim1, dim2];
rectangle('Position', pos, 'LineWidth',1)

X=zeros(8,3);
X([5:8])=dim1;
X([11,12,15,16])=dim2;
X([18,20,22,24])=dim3;
d=[1 2 4 3 1 5 6 8 7 5 6 2 4 8 7 3];
plot3(X(d,1)-dim1/2,X(d,2)-dim2/2,X(d,3)-dim3/2);
view(3); rotate3d;

axis equal
hold off
grid on



Nb = size(pt_mid_L,1);
tic
[Rpeec, Lpeec, Ppeec] = para_main_plate_3d(pt_mid_L, dv_L, d1_L,d2_L,d3_L, ...
    pt_mid_P, dv_P, d1_P,d2_P, p_flag);

toc

%% source
sr_type = 0;
dlavg = max(max(d1_L), max(d2_L));
dt = 1.01*dlavg/3e8;
tus = 0:dt:4e-6;
sr_t = 5e9.*tus;
Nt = length(tus);

dt = 0.01e-7;
Tmax = 2e-6;
[sr_t, ~] = sr_heidler(1000, 0.01e-6, 2e-6, 5, Tmax*1.2, dt, 0e-6);
Nt = ceil(Tmax/dt);
tus = (0:dt:Tmax);


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
% Rpeec = diag(0*ones(Nseg,1));

[~, id_TdL] = sol_td(pt_mid_L, dt);

[~, id_TdP] = sol_td(pt_mid_P, dt);

Rec=[];Lec=[]; Cec=[]; 
[Rec,Lec,Cec, Pnew, Cs,Ss, Amtx, id_sv, id_si, id_TdP] = sol_net_mna_td(Lpeec,Ppeec, ...
    Rfit,Lfit, Rec,Lec,Cec, nod_P,id_TdP, nod_start,nod_end, nod_gnd, nod_sr, nod_ec, sr_type);

disp('Start Calculationg');
tic
% [Ibt, Vnt] = sol_peec_time_mna_td( Rpeec, Lpeec, Cs, Speec, ...
%     Rec, Lec, Cec, id_TdL,id_TdP, Amtx, id_sv,id_si, sr_t, dt, Nt );

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

