
clear

frq_max = 1e4;
% frq = [1 20 50 100 200 500 1e3 2e3 5e3 10e3 20e3 50e3 100e3 200e3 500e3 1e6 2e6 5e6];
% Nf = length(frq);
Nf = 100;
frq = logspace(0,log10(frq_max),Nf);
  
        
pt_start = [ 0  0  0;
             0  0  5;
             0  0  10;
             0  0  15;
             0  0  20;
             0  0  25;
             0  0  30;
             0  0  35;
             0  0  40;
             0  0  45;
             0  0  50;
             1.  0  0;
             1.  0  0;];

pt_end   = [ 0  0  5;
             0  0  10;
             0  0  15;
             0  0  20;
             0  0  25;
             0  0  30;
             0  0  35;
             0  0  40;
             0  0  45;
             0  0  50;
             0  0  55;   
             0.6  0  50;
             0  0  0;];

dim1 = [1.46;
        1.3725;
        1.282;
        1.195;
        1.107;
        1.020;
        0.932;
        0.845;
        0.756;
        0.6685;
        0.065;
        10e-3;
        10e-3]/2;

       
Nc = size(pt_start,1);
shape = 2000*ones(Nc,1);

dim2 = 0*ones(Nc,1);
sig = 5.8e7*ones(Nc,1);
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



dl = 1*ones(Nc, 1);
dl_type = 1;
%    case 1  Ns = ceil(len./dl); % input the interval length
%    case 2  % input the number of the segment     


%% source
sr_type = 0;   %  0 - current source
 
dt = 2e-8;
Tmax = 10e-6;

t = (0:dt:Tmax);
% Nt = length(t);
% sr_t = sr_upulse(t,25e-9,100e-12);

% First negative impulse 1/200us k=0.986  T1=1.82  T2=285  100kA
% Subsequent negative impulse 0.25/100us k=0.993  T1=0.454  T2=143  50kA
amp = 50e3;
tau1 = 0.454e-6;
tau2 = 143e-6;
[sr_t, tus] = sr_heidler(amp, tau1, tau2, 10, Tmax, dt, 0);
Nt = ceil(Tmax/dt);

tfront = 0.25e-6;
fmax = 1/(4*tfront);

% plot(tus, sr_t)

% sr_tmp = readtable('0.3 us rise head.txt');
% ttmp = (sr_tmp.Var2-1)*2e-6/length(sr_tmp.Var2);
% sr_t = interp1(ttmp, sr_tmp.Var1, t);


flag_p = 1;
[pt_sta_L,pt_end_L,dv_L,re_L,l_L, shape_L,d1_L,d2_L, R_L, sig_L,mur_L, ...
    pt_sta_P,pt_end_P,pt_P,re_P,dv_P,l_P, shape_P,d1_P,d2_P, ...
    g_id_L,g_type_L,p_2d_L, bname,nd_start,nd_end, nd_P, Ns] = main_mesh_line_3d ...
    (pt_start,pt_end,dv,dim1,len, shape,dim1,dim2, R_pul,sig,mur, ...
    grp_id,grp_type,pt_2d, bran_name,nod_start,nod_end, dl,dl_type, flag_p);

Nttw = sum(Ns(1:10));
d_d1 = (dim1(1)-dim1(10))/(Nttw-1);
d1_L(1:Nttw) = dim1(1)-(0:Nttw-1)*d_d1;

Nb_seg = size(pt_sta_L,1);
Nn_seg = size(pt_sta_P,1);


figure(55)
plot3([pt_sta_L(:,1) pt_end_L(:,1)]',[pt_sta_L(:,2) pt_end_L(:,2)]',[pt_sta_L(:,3),pt_end_L(:,3)]','-x')
grid on
axis equal
% % 
% figure(56)
% plot3([pt_sta_P(:,1) pt_end_P(:,1)]',[pt_sta_P(:,2) pt_end_P(:,2)]',[pt_sta_P(:,3) pt_end_P(:,3)]','-o')
% grid on
% axis equal


Nb = size(pt_sta_L,1);

pt_sta_L_img = pt_sta_L;
pt_sta_L_img(:,3) = -pt_sta_L_img(:,3);
pt_end_L_img = pt_end_L;
pt_end_L_img(:,3) = -pt_end_L_img(:,3);
pt_sta_P_img = pt_sta_P;
pt_sta_P_img(:,3) = -pt_sta_P_img(:,3);
pt_end_P_img = pt_end_P;
pt_end_P_img(:,3) = -pt_end_P_img(:,3);

[TdL, id_TdL] = sol_td(pt_sta_L, pt_end_L, dt);
[TdP, id_TdP] = sol_td(pt_sta_P, pt_end_P, dt);
[TdGL, id_TdGL] = sol_td_img(pt_sta_L,pt_end_L, pt_sta_L_img,pt_end_L_img, dt);
[TdGP, id_TdGP] = sol_td_img(pt_sta_P,pt_end_P, pt_sta_P_img,pt_end_P_img, dt);

id_TdL(2*pi*fmax*TdL<1e-2)=0;
id_TdP(2*pi*fmax*TdP<1e-2)=0;
id_TdGL(2*pi*fmax*TdGL<1e-2)=0;
id_TdGP(2*pi*fmax*TdGP<1e-2)=0;

[Rpeec, Lpeec, Ppeec] = para_main_fila_rlp(pt_sta_L, pt_end_L, dv_L, d1_L, l_L, ...
    pt_sta_P, pt_end_P, dv_P, d1_P, l_P, flag_p);
offset = 0;
[Rg,Lg,Pg, Rgself,Lgself] = ground_pec(pt_sta_L,pt_end_L,dv_L,re_L,l_L, ...
    pt_sta_P,pt_end_P,dv_P,re_P,l_P, frq, offset, flag_p); 



% [Rs1, Ls1] = para_self_multi_frq(shape_L(1), d1_L(1), d2_L(1), ...
%     l_L(1), R_pul(1), sig(1), mur(1)*ones(1,Nf), frq);
% [Rs, Ls, Rn, Ln] = main_vectfit_z(Rs1(1,1:Nf),Ls1(1,1:Nf),frq, 2, Lpeec);
% Lpeec = Lpeec+diag(Ls*ones(Nseg,1)-diag(Lpeec));
% Rpeec = diag(Rs*ones(Nseg,1));
% Rfit = ones(Nseg,1)*Rn;
% Lfit = ones(Nseg,1)*Ln;

Rpeec = diag(0*ones(Nb_seg,1));
Rfit = [];
Lfit = [];


[nod_start, nod_end, nod_P] = sol_peec_brn_nod_create( ...
    pt_sta_L,pt_end_L, pt_P, [],[], [],[], flag_p);


%% set source pt
Rec=[]; Lec=[]; Cec=[]; 
Rec = [5; 0; 0;]; % add a ground resistance

ngnd = max([nod_start; nod_end]) + 1;
ncon_down = max([nod_start; nod_end]) + 2;

nod_start = [nod_start; nod_start(1);  nod_start(sum(Ns(1:11))+1);  nod_end(sum(Ns(1:12))); ];
nod_end =   [nod_end;        ngnd;     nod_start(1);                nod_end(sum(Ns(1:10)));];

nod_ec = [ ngnd;];


nod_sr = [nod_end(sum(Ns(1:11)))  ngnd];
nod_gnd =  ngnd;

[Rec,Lec,Cec, Pnew,Pgnew, Amtx, id_sv,id_si, id_TdP,id_TdGP,nod_new] = sol_net_mna_td_gnd(Lpeec,Ppeec, Pg,...
    Rfit,Lfit, Rec,Lec,Cec, nod_P,id_TdP,id_TdGP, nod_start,nod_end, nod_gnd, nod_sr, nod_ec, sr_type);


tic
[Ibt, Vnt] = sol_peec_time_mna_td_gnd( Rpeec,Lpeec,Pnew, ...
    Rec,Lec,Cec, -Lg,-Pgnew, id_TdL,id_TdP, id_TdGL,id_TdGP, ...
    Amtx, id_sv,id_si, sr_t, dt, Nt );
toc



figure(61)
hold on
plot(t(1:Nt-1)*1e6, Ibt(sum(Ns(1:11)),1:Nt-1)/1e3,'r-','LineWidth',2);   % top
plot(t(1:Nt-1)*1e6, Ibt(sum(Ns(1:10)),1:Nt-1)/1e3,'r-','LineWidth',1);  % rbs up
plot(t(1:Nt-1)*1e6, Ibt(1,1:Nt-1)/1e3,'r-','LineWidth',1);  % rbs down

% plot(t(1:Nt-1)*1e6, Ibt(sum(Ns(1:12)),1:Nt-1)/1e3,'r--','LineWidth',1);  % cable up

ind_decay = t(1:Nt-1)>1.89e-6;
decay_factor = ones(1,Nt-1);
Icable_up = Ibt(sum(Ns(1:12)),1:Nt-1);
[~,ind0] = min(abs(Icable_up(20:end)));
% Icable_up(ind0(1)+20:end) = 0;
% decay_factor(ind_decay) = 1-1/8e-6*(t(ind_decay)-1.89e-6);
plot(t(1:Nt-1)*1e6, Icable_up.*decay_factor/1e3,'r--','LineWidth',1);  % cable up

% plot(t(1:Nt-1)*1e6, Ibt(sum(Ns(1:11))+1,1:Nt-1)/1e3,'r-.','LineWidth',1);  % cable down

Icable_down = Ibt(sum(Ns(1:11))+1,1:Nt-1);
% % [~,ind0] = min(abs(Icable_down(20:end)));
% % Icable_down(ind0(1)+20:end) = 0;
plot(t(1:Nt-1)*1e6, Icable_down.*decay_factor/1e3,'r-.','LineWidth',1);  % cable down

ylabel('Current(kA)')
xlabel('Time(us)')

legend('ST-Top','ST-RBS','ST-Cable Up','ST-Cable Down')


figure(62)
hold on
plot(t(1:Nt-1)*1e6, Vnt(sum(Ns(1:11))+1,1:Nt-1)/1e3,'r-','LineWidth',1);
plot(t(1:Nt-1)*1e6, Vnt(sum(Ns(1:10))+1,1:Nt-1)/1e3,'r--','LineWidth',1);
plot(t(1:Nt-1)*1e6, Vnt(1,1:Nt-1)/1e3,'r--','LineWidth',1);
plot(t(1:Nt-1)*1e6, Vnt(108,1:Nt-1)/1e3,'r--','LineWidth',1);
plot(t(1:Nt-1)*1e6, Vnt(sum(Ns(1:11))+2,1:Nt-1)/1e3,'r--','LineWidth',1);
% plot(t(1:Nt-1)*1e6, Vnt(1,1:Nt-1)/1e3,'r-.','LineWidth',1);
ylabel('Voltage(kV)')
xlabel('Time(us)')
legend('ST-Top','ST-RBS','ST-Cable Up','ST-Cable Down')


