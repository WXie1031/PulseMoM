clear
clc


%Frequency series parameters
Nf = 40;
FreqStart   = 10e6;      %in Hz
FreqStop    = 1e9;      %in Hz

% Nf = 1;
% FreqStart   = 750e6;      %in Hz
% FreqStop    = 1e9;      %in Hz

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
dlx = [20;20];
dly = [1;1];
% 1 - input the interval length
% 2 - input the number of the segment
dl_type = 2;

pt_mid = [-0.0501 0 0;
    0.0501 0 0;];  % center of the plate - multi-plate is supported


Nc = size(pt_mid,1);
% demension of the plate
dim1 = 0.1*ones(Nc,1);
dim2 = 0.004*ones(Nc,1);

sig = 5.8e7*ones(Nc,1);
% sig = 1e20*ones(Nc,1);
R_pul = 1./(sig.*(dim1.*dim2));  % resistivity
R_pul = 0*R_pul;

% default
shp_id = 1001*ones(Nc,1);
mur = 1*ones(Nc,1);
epr = 1*ones(Nc,1);
dv = ones(Nc,1)*[0 0 1];

%% mesh the plate
p_flag = 1;  % capacitor is calculated or not
% mesh the plate
[pt_sta_L, pt_end_L, pt_mid_L, dv_L, shape_L,d1_L,d2_L, R_L, sig_L,mur_L,epr_L, ...
    pt_P, pt_mid_P, dv_P, shape_P,d1_P,d2_P, Nbx,Nby] = mesh_plate_2d ...
    (pt_mid, dv, shp_id, dim1, dim2, R_pul, sig, mur,epr, dlx, dly, dl_type, p_flag);
Nbt = size(pt_mid_L,1);
if p_flag>0
    Nn = size(pt_P,1);
else
    Nn = 0;
end

%% plot the plate - inductance cell and capacitance cell
% figure(54)
% hold on
% pt_tmp_L = unique([pt_sta_L;pt_end_L],'rows');
% plot(pt_tmp_L(:,1),pt_tmp_L(:,2),'+','MarkerSize',4)
% plot(pt_P(:,1),pt_P(:,2),'o','MarkerSize',6)
% axis equal

% figure(55)
% hold on
% plot(pt_mid_L(1:Nbx,1),pt_mid_L(1:Nbx,2),'x','MarkerSize',4)
% plot(pt_mid_L(Nbx+1:end,1),pt_mid_L(Nbx+1:end,2),'+','MarkerSize',4)
% for ik=1:Nbt
%     arrowPlot([pt_sta_L(ik,1),pt_end_L(ik,1)],[pt_sta_L(ik,2),pt_end_L(ik,2)], 'number', 1)
% end
% hold on
% plot(pt_sta_L(1:Nbx,1),pt_sta_L(1:Nbx,2),'s','MarkerSize',6)
% plot(pt_end_L(1:Nbx,1),pt_end_L(1:Nbx,2),'s','MarkerSize',6)
%
% posL = [pt_mid_L(:,1)-d1_L/2, pt_mid_L(:,2)-d2_L/2, d1_L,d2_L];
% for ik=1:Nbx
%     rectangle('Position', posL(ik,:),'LineStyle','--','EdgeColor','b');
% end
% for ik=Nbx+1:size(posL,1)
%     rectangle('Position', posL(ik,:),'LineStyle','--','EdgeColor','g');
% end
% axis equal


figure(56)
hold on
if p_flag>0
    plot(pt_mid_P(:,1),pt_mid_P(:,2),'bo','MarkerSize',6)
    posP = [pt_mid_P(:,1)-d1_P/2, pt_mid_P(:,2)-d2_P/2, d1_P,d2_P];
    for ik=1:size(posP,1)
        rectangle('Position', posP(ik,:),'LineStyle','-.','EdgeColor','r');
    end
end
axis equal



%% calculate the parameters
dt_td = 1e-12;
[TdL, ~] = sol_td(pt_mid_L, dt_td);
if p_flag>0
    [TdP, ~] = sol_td(pt_mid_P, dt_td);
else
    TdP = [];
end
[Rpeec, Lpeec, Ppeec] = para_main_plate_2d(pt_mid_L, dv_L, d1_L, d2_L, R_L,...
    pt_mid_P, dv_P, d1_P, d2_P, p_flag);

Zx = zeros(Nbx,Nbx,Nf);
Zy = zeros(Nby,Nby,Nf);
Zxy = zeros(Nbx,Nby,Nf);
Zmtx = zeros(Nbt,Nbt,Nf);
Pmtx = zeros(Nn,Nn,Nf);
for ik = 1:Nf
    w0 = 2*pi*frq(ik);
    Zmtx(:,:,ik) = Rpeec + 1j*w0*Lpeec.*exp(-1j*w0*TdL);
    Zx(:,:,ik) = Zmtx(1:Nbx,1:Nbx,ik);
    Zxy(:,:,ik) = Zmtx(1:Nbx,Nbx+(1:Nby),ik);
    Zy = Zmtx(Nbx+(1:Nby),Nbx+(1:Nby),ik);
    if p_flag>0
        Pmtx(:,:,ik) = Ppeec*exp(-1j*w0*TdP);
    end
end

% generate node information based on the connection
[nod_start, nod_end, nod_P] = sol_peec_brn_nod_create( ...
    pt_sta_L,pt_end_L, pt_P, [],[], [],[], p_flag);
% conenction matrix
Amtx = sol_a_mtx(nod_start, nod_end, nod_P);

Ymtx = zeros(Nn,Nn,Nf);
for ik = 1:Nf
    w0 = 2*pi*frq(ik);
    Ymtx(:,:,ik) = Amtx*inv(Zmtx(:,:,ik))*Amtx' + 1j*w0*inv(Pmtx(:,:,ik));
end

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% add optimization method


% - change Ymtx, pt_P, nod_P
id_si = [ceil(Nn/4); ceil(Nn*2/4)+1;];
Nsr = length(id_si);

hold on
plot(pt_mid_P(id_si,1),pt_mid_P(id_si,2),'o','MarkerSize',10,'MarkerFaceColor','r')

% - calculate port impedance -- require a port impedance
Vs = zeros(Nbt,1);
Is = zeros(Nn,Nf);
FeedCurrent = zeros(1,Nf);
FeedVoltage = zeros(1,Nf);
FeedImpedance = zeros(1,Nf);
FeedPower = zeros(1,Nf);
Zinput = zeros(1,Nf);
% Vs(id_sv(1)) = 1;
Is(id_si(2)) = -1;
Is(id_si) = 1;

Rec=[];Lec=[];Cec=[];
sr_f = ones(1,Nf);
id_sv = zeros(Nbt,1);
id_si_tmp = zeros(Nn,1);
id_si_tmp(id_si(1)) = 1;
id_si_tmp(id_si(2)) = -1;
[I, Vnf] = sol_peec_freq_mna( Rpeec,Lpeec,Ppeec, TdL,TdP, Rec,Lec,Cec, ...
    Amtx, id_sv,id_si_tmp, sr_f, frq );



R0 = 50;  % reference impedance
for ik = 1:Nf
    I(:,ik) = Ymtx(:,:,ik)*(Amtx*Vs);
    
    FeedCurrent(:,ik)   = sum(I(id_sv,ik));
    FeedVoltage(:,ik)   = mean(Vs(id_sv));
    FeedImpedance(:,ik) = FeedVoltage(ik)./FeedCurrent(ik);
    FeedPower(:,ik)    = 1/2*real(FeedCurrent(ik).*conj(FeedVoltage(ik)));
    
    Zinput(:,ik) = FeedImpedance(:,ik);
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
    
    figure(41)
    hold on
%     plot(frq,real(S11),'r')
%     plot(frq,imag(S11),'r--')
    plot(frq,abs(S11),'k')
else
    Zinput
end


