clear
clc


%Frequency series parameters
Nf = 10;
FreqStart   = 20e9;      %in Hz
FreqStop    = 400e9;      %in Hz

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
dlx = [5;5;5];
dly = [2;2;2;];
dlz = [1;1;1;];
% 1 - input the interval length
% 2 - input the number of the segment
dl_type = 2;  


h = 0.05;

pt_mid = [0 0 0;
    0 0 -h;
    0 0 -h/2;];  % center of the plate - multi-plate is supported

Nc = size(pt_mid,1);
% demension of the plate
dim1 = 2*ones(Nc,1);
dim2 = 1*ones(Nc,1);
dim3 = [1e-4;5e-3;1e-4;];

sig = 5.8e7*ones(Nc,1);
R_pul = 1./(sig.*(dim1.*dim2));  % resistivity


% default
shp_id = 1001*ones(Nc,1);
mur = 1*ones(Nc,1);
epr = [1;1;2.2];

dv = ones(Nc,1)*[0 0 1];

%% mesh the plate
p_flag = 1;  % capacitor is calculated or not
% mesh the plate
% [pt_sta_L, pt_end_L, pt_mid_L, dv_L, shpid_L,d1_L,d2_L, R_L, sig_L,mur_L,erp_L, ...
%     pt_P, pt_mid_P, dv_P, shp_id_P,d1_P,d2_P, Nbx,Nby] = mesh_plate_2d ...
%     (pt_mid, dv, shp_id, dim1, dim2, R_pul, sig, mur, epr, dlx, dly, dl_type, p_flag);

[pt_sta_L,pt_end_L,pt_mid_L, dv_L, shpid_L,d1_L,d2_L,d3_L, R_L, sig_L,mur_L,epr_L, ...
    pt_P,pt_mid_P, dv_P, shape_P,d1_P,d2_P, Nsx,Nsy,Nsz] = ...
    mesh_plate_3d(pt_mid, dv, shp_id, dim1,dim2,dim3, R_pul, sig,mur,epr, ...
    dlx,dly,dlz, dl_type, p_flag);

Nbx = sum(Nsx);
sum(Nsy);
sum(Nsz);

Nbt = size(pt_mid_L,1);
if p_flag>0
    Nn = size(pt_P,1);
else
    Nn = 0;
end

%% plot the plate - inductance cell and capacitance cell
figure(55)
hold on
plot3(pt_mid_L(1:Nbx,1),pt_mid_L(1:Nbx,2),pt_mid_L(1:Nbx,3),'x','MarkerSize',4)
plot3(pt_mid_L(Nbx+1:end,1),pt_mid_L(Nbx+1:end,2),pt_mid_L(Nbx+1:end,3),'+','MarkerSize',4)
for ik=1:Nbt
    arrowPlot([pt_sta_L(ik,1),pt_end_L(ik,1)],[pt_sta_L(ik,2),pt_end_L(ik,2)], 'number', 1)
end
hold on
plot(pt_sta_L(1:Nbx,1),pt_sta_L(1:Nbx,2),'s','MarkerSize',6)
plot(pt_end_L(1:Nbx,1),pt_end_L(1:Nbx,2),'s','MarkerSize',6)

posL = [pt_mid_L(:,1)-d1_L/2, pt_mid_L(:,2)-d2_L/2, d1_L,d2_L];
for ik=1:Nbx
    rectangle('Position', posL(ik,:),'LineStyle','--','EdgeColor','b');
end
for ik=Nbx+1:size(posL,1)
    rectangle('Position', posL(ik,:),'LineStyle','--','EdgeColor','g');
end

if p_flag>0
    plot(pt_mid_P(:,1),pt_mid_P(:,2),'o','MarkerSize',6)
    posP = [pt_mid_P(:,1)-d1_P/2, pt_mid_P(:,2)-d2_P/2, d1_P,d2_P];
    for ik=1:size(posP,1)
        rectangle('Position', posP(ik,:),'LineStyle','-.','EdgeColor','r');
    end
    
end

pos = [pt_mid(:,1)-dim1/2,pt_mid(:,2)-dim2/2, dim1, dim2];
for ik = 1:Nc
rectangle('Position', pos(ik,:), 'LineWidth',1)
end
% grid on
hold off
axis equal



% 
x_plot = 1;
y_plot = 1;
z_plot = 0;
p_plot = 1;
mid_plot = 0;

figure(56)
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

Se = zeros(Nbt,1);
le = zeros(Nbt,1);

id_x = sum(abs(dv_L-[1 0 0]),2)<=1e-6;
id_y = sum(abs(dv_L-[0 1 0]),2)<=1e-6;

Se(id_x) = d2_L(id_x);
le(id_x) = d1_L(id_x);
Se(id_y) = d1_L(id_y);
le(id_y) = d2_L(id_y);
Cc = diag( cap_die(Se, le, erp_L) );


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
id_sv = 1;
Nsr = length(id_sv);

% - calculate port impedance -- require a port impedance
V = zeros(Nn,1);
I = zeros(Nn,Nf);
FeedCurrent = zeros(1,Nf);
FeedVoltage = zeros(1,Nf);
FeedImpedance = zeros(1,Nf);
FeedPower = zeros(1,Nf);
S11 = zeros(1,Nf);
V(id_sv) = 1;
R0 = 50;  % reference impedance
for ik = 1:Nf
    I(:,ik) = Ymtx(:,:,ik)*V;
    
    FeedCurrent(:,ik)   = sum(I(id_sv,ik),2);
    FeedVoltage(:,ik)   = mean(V(id_sv),2);
    FeedImpedance(:,ik) = FeedVoltage(ik)./FeedCurrent(ik);
    FeedPower(:,ik)    = 1/2*real(FeedCurrent(ik).*conj(FeedVoltage(ik)));
    
    Zin = FeedImpedance(:,ik);
    S11(:,ik) = (Zin-R0)./(Zin+R0); 
end
% may be not correct
VSWR = (1+abs(S11))./(1-abs(S11));


% - compare with the objective function
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


% - plot 
% plot(frq,real(S11))
plot(frq,abs(S11))


