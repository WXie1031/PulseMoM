clear
clc


%Frequency series parameters
Nf = 41;
FreqStart   = 10e6;      %in Hz
FreqStop    = 1e9;      %in Hz

Nf = 40;
% center 750e6;
FreqStart   = 500e6;      %in Hz
FreqStop    = 1000e6;      %in Hz

if Nf == 1
    frq = FreqStart;
    frq_plt = FreqStart;
else
    df = (FreqStop-FreqStart)/(Nf-1);
    frq = FreqStart+df*(0:Nf-1)';
    
    frq_plt = FreqStart+df*ceil(Nf/2);
end

vc = 3e8;  % speed of light
wlength = vc/max(frq);

%% plate geometry information
% No. of grids in x and y direction
dl = [40;]+1;
% 1 - input the interval length
% 2 - input the number of the segment
dl_type = 2;

pt_start = [-0.1 0 0;];
pt_end   = [0.1 0 0;];

Nc = size(pt_start,1);
% demension of the plate
dim1 = 1e-3*ones(Nc,1);
dim2 = 0.*ones(Nc,1);
re = dim1;

sig = 5.8e7*ones(Nc,1);
% sig = 1e20*ones(Nc,1);
R_pul = 1./(sig.*(dim1.*dim2));  % resistivity
R_pul = zeros(Nc,1);

% default
shp_id = 4000*ones(Nc,1);
mur = 1*ones(Nc,1);
epr = 1*ones(Nc,1);
[dv, len] = line_dv(pt_start, pt_end);

%% mesh the plate
p_flag = 1;  % capacitor is calculated or not
% mesh the plate
[pt_sta_L,pt_end_L, dv_L,re_L l_L, shape_L,d1_L,d2_L, R_L,sig_L,mur_L,epr_L, ...
    pt_sta_P, pt_end_P, pt_P, re_P, dv_P, l_P, shape_P,d1_P,d2_P] ...
    = mesh_line_3d(pt_start,pt_end,dv,re,len,shp_id,dim1,dim2, ...
    R_pul,sig, mur,epr, dl, dl_type, p_flag);
Nbt = size(pt_sta_L,1);
if p_flag>0
    Nb = size(pt_sta_L,1);
    Nn = size(pt_P,1);
else
    Nb = size(pt_sta_L,1);
    Nn = 0;
end

%% plot the plate - inductance cell and capacitance cell
pt_mid_P = (pt_sta_P + pt_end_P)/2;
figure(56)
hold on
if p_flag>0
    plot(pt_mid_P(:,1),pt_mid_P(:,2),'ko','MarkerSize',6)
end
axis equal



%% calculate the parameters
dt_td = 1e-12;
[TdL, ~] = sol_td(pt_sta_L, pt_end_L, dt_td);
if p_flag>0
    [TdP, ~] = sol_td(pt_sta_P, pt_end_P, dt_td);
else
    TdP = [];
end
[Rpeec, Lpeec, Ppeec] = para_main_fila_rlp(pt_sta_L, pt_end_L, dv_L, d1_L, l_L, ...
    pt_sta_P, pt_end_P, dv_P, d1_P, l_P, p_flag);


Zmtx = zeros(Nbt,Nbt,Nf);
Pmtx = zeros(Nn,Nn,Nf);
for ik = 1:Nf
    w0 = 2*pi*frq(ik);
    Zmtx(:,:,ik) = Rpeec + 1j*w0*Lpeec.*exp(-1j*w0*TdL);
    if p_flag>0
        Pmtx(:,:,ik) = Ppeec*exp(-1j*w0*TdP);
    end
end

% generate node information based on the connection
[nod_start, nod_end, nod_P] = sol_peec_brn_nod_create( ...
    pt_sta_L,pt_end_L, pt_P, [],[], [],[], p_flag);
% conenction matrix
Amtx = sol_a_mtx(nod_start, nod_end, nod_P);

Zb_mtx = zeros(Nbt,Nbt,Nf);
Yb_mtx = zeros(Nbt,Nbt,Nf);
Yn_mtx = zeros(Nn,Nn,Nf);
for ik = 1:Nf
    w0 = 2*pi*frq(ik);
%     Yn_mtx(:,:,ik) = Amtx*inv(Zmtx(:,:,ik))*Amtx' + 1j*w0*inv(Pmtx(:,:,ik));
    Zb_mtx(:,:,ik) = Zb_mtx(:,:,ik) + 1/(1j*w0)*Amtx'*Pmtx(:,:,ik)*Amtx;
%     Yb_mtx(:,:,ik) = inv(Zb_mtx(:,:,ik));
end

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% add optimization method

% - change Ymtx, pt_P, nod_P
% id_sv = [ceil(Nn/2);ceil(Nn/2)+1;];
id_si = [ceil(Nn/2)+1;];

Nsr = length(id_si);

hold on
plot(pt_mid_P(id_si,1),pt_mid_P(id_si,2),'o','MarkerSize',10,'MarkerFaceColor','r')


% Km = zeros(Nn,Nport);
Rec=[];Lec=[];Cec=[];
sr_f = ones(1,Nf);
id_si_tmp = zeros(Nn,1);
id_sv_tmp = zeros(Nbt,1);
% id_sv_tmp(id_sv(1)) = 1;
% id_sv_tmp(id_sv(2)) = -1;
id_si_tmp(id_si(1)) = 1;
[Ibf, Vnf] = sol_peec_freq_mna( Rpeec,Lpeec,Ppeec, TdL,TdP, Rec,Lec,Cec, ...
    Amtx, id_sv_tmp,id_si_tmp, sr_f, frq );

% [Vnf1] = sol_peec_freq_na( Rpeec,Lpeec,Ppeec, TdL,TdP, Rec,Lec,Cec, ...
%     Amtx, id_sv_tmp,id_si_tmp, sr_f, frq );

% Ibf = sol_peec_freq_mla( Rpeec,Lpeec,Ppeec, TdL,TdP, Rec,Lec,Cec, ...
%     Amtx, id_sv_tmp,id_si, sr_f, frq );

% - calculate port impedance -- require a port impedance
FeedCurrent = zeros(1,Nf);
FeedVoltage = zeros(1,Nf);
FeedImpedance = zeros(1,Nf);
FeedPower = zeros(1,Nf);
Zinput = zeros(1,Nf);


Vs = zeros(Nbt,1);
Vs(id_sv) = 1;
id_port = zeros(Nbt,1);
id_port(id_sv) = 1;

R0 = 50;  % reference impedance


I = zeros(Nb,Nf);
for ik = 1:Nf
    I(:,ik) = (Zb_mtx(:,:,ik))\(Vs);
    
    FeedCurrent(:,ik)   = sum(Ibf(id_sv));
    %     FeedCurrent(:,ik)   = I(id_sv(1),ik)-I(id_sv(2),ik);
    FeedVoltage(:,ik)   = mean(Vs(id_sv));
    FeedImpedance(:,ik) = FeedVoltage(ik)./FeedCurrent(ik);
    FeedPower(:,ik)    = 1/2*real(FeedCurrent(ik).*conj(FeedVoltage(ik)));
    
    Zinput(:,ik) = FeedImpedance(:,ik);
%     Zinput(:,ik) = 1./(id_port'*Yb_mtx(:,:,ik)*id_port);
end
% may be not correct
S11 = -20*log((Zinput-R0)./(Zinput+R0));
VSWR = (1+abs(S11))./(1-abs(S11));


% - compare with the objective function
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


% - plot
if Nf>1
    figure(40)
    hold on
%     plot(frq,real(Zinput),'r')
%     plot(frq,imag(Zinput),'r--')
    plot(frq,abs(Zinput),'k')
    title('Z(ohm)')
    
    figure(41)
    hold on
%     plot(frq,real(S11),'b')
%     plot(frq,imag(S11),'b--')
    plot(frq,(S11),'k')
    title('S11')
else
    Zinput
    abs(Zinput)
end



