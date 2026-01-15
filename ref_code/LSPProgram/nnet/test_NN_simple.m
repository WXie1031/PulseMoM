clc

frq = [1 50 100 200 500 1e3 2e3 5e3 10e3 20e3 50e3 100e3  200e3 500e3 1e6];
% frq_interp = linspace(1,1e6, 50);
Nf = size(frq,2);
murtmp = 1;
sig_cu = 5.8e7;

dat = [];
k = 0;
%% 1. data 1x3 horizontal 3 cox cables + 1x2 horizontal 2 coaxial cables
%   O O 
%  O O O 

k = k+1;
disp(['case ',num2str(k)])

dat.info = '1x3 horizontal 3 coaxial cables + 1x2 horizontal 2 coaxial cables'; 

dat.frq = frq;
dat.shp_id = [
    2100; 2100; 2100; 2100;2100; 2100; 2100; 2100;2100; 2100; ]; %sdc up 1-2

dat.pt_2d = [
    100 12.5; 100 12.5; 137.5 12.5; 137.5 12.5; 175 12.5; 175 12.5; ... %cox down 1-3
    118.75 45; 118.75 45; 156.25 45; 156.25 45;  %cox up 1-5
    ]*1e-3; 
dat.dim1 = [12.45; 4.5; 12.45; 4.5; 12.45; 4.5; 12.45; 4.5; 12.45; 4.5; ]*1e-3;
dat.dim2 = [11.65; 0; 11.65; 0; 11.65; 0; 11.65; 0; 11.65; 0; ]*1e-3;
dat.S = pi*(dat.dim1.^2-dat.dim2.^2);
dat.Rpul = 1./(sig_cu.*dat.S);
dat.len = 1.33;

Nc = size(dat.pt_2d,1);
mur = ones(Nc,Nf)*murtmp;
dat.sig = 1./(dat.Rpul.*dat.S);
[Rmesh1,Lmesh1, Rself1,Lself1, dat.RmeshMF,dat.LmeshMF] = main_mesh2d_cmplt( ...
    dat.shp_id, dat.pt_2d, dat.dim1, dat.dim2, dat.Rpul,dat.sig, mur,dat.len, dat.frq);




[Rmesh2,Lmesh2, Rself2,Lself2, dat.RmeshNN,dat.LmeshNN] = main_mesh2d_nn( ...
    dat.shp_id, dat.pt_2d, dat.dim1, dat.dim2, dat.Rpul,dat.sig,mur,dat.len, dat.frq, 1);


Nc = size(dat.dim1,1);
shp_id = 2001*ones(Nc,1);
len = dat.len*ones(Nc,1);
sig = sig_cu*ones(Nc,1);
[Rskin, Lskin] = para_self_multi_frq(shp_id, dat.dim1, dat.dim2, ...
    len, dat.Rpul, sig, mur, frq);

RRrat1 = zeros(Nc,Nf);
RRrat2 = zeros(Nc,Nf);
for ik = 1:Nc
    RRrat1 = squeeze(dat.RmeshMF(ik,ik,:))./Rskin(ik,:)';
    RRrat2 = squeeze(dat.RmeshMF(ik,ik,:))./squeeze(dat.RmeshNN(ik,ik,:));
end

Rprox1 = zeros(Nc,Nf);
Rprox2 = zeros(Nc,Nf);
for ik = 1:Nc
    Rprox1 = squeeze(dat.RmeshMF(ik,ik,:))-Rskin(ik,:)';
    Rprox2 = squeeze(dat.RmeshNN(ik,ik,:))-Rskin(ik,:)';
end


figure(1)
hold on
plot(frq,RRrat1,'k')
plot(frq,RRrat2,'k--')

figure(2)
hold on
plot(frq,Rprox1,'k')
plot(frq,Rprox2,'k--')

    

figure(3)
hold on
for ik = 1:Nc
plot(frq,squeeze(dat.RmeshMF(ik,ik,:))*1.1,'k');
plot(frq,squeeze(dat.RmeshNN(ik,ik,:)),'k--');
% plot(frq,Rskin(ik,:),'k-.')
end

% figure(4)
% hold on
% for ik = 1:Nc
% plot(frq,squeeze(dat.LmeshMF(ik,ik,:)),'k');
% plot(frq,squeeze(dat.LmeshNN(ik,ik,:)),'k--');
% % plot(frq,Rskin(ik,:),'k-.')
% end

Rnum = squeeze(dat.RmeshMF(ik,ik,end))*1.1;
Rnn = squeeze(dat.RmeshNN(ik,ik,end));

(Rnum-Rnn)./Rnum*100


