
data_name = 'database_nn_interp2';

load(data_name)


dat = data_proximity{50,1};

Nf = length(dat.frq);
Nc = length(dat.dim1);
mur = ones(Nc,1);

% [Rmesh,Lmesh, Rself,Lself, RmeshMF,LmeshMF] = main_mesh2d_cmplt( ...
%     dat.shp_id, dat.pt_2d, dat.dim1, dat.dim2, dat.Rpul, mur, dat.len, dat.frq);

[Rmesh,Lmesh, Rself,Lself, RmeshMF,LmeshMF] = main_mesh2d_nn( ...
    dat.shp_id, dat.pt_2d, dat.dim1, dat.dim2, dat.Rpul, dat.sig,mur, dat.len, dat.frq);

% [Rmesh,Lmesh, Rself,Lself] = main_mesh2d_nn_self( ...
%     dat.shp_id, dat.pt_2d, dat.dim1, dat.dim2, dat.Rpul, mur, dat.len, dat.frq);

% frq_cal = dat.frq(dat.frq<=100e3);
% frq_nn = dat.frq(dat.frq>100e3);
% [RmeshMF,LmeshMF] = main_mesh2d_nn_sub_rat(RmeshMF,LmeshMF, ...
%     dat.shp_id, dat.pt_2d, dat.dim1, dat.dim2, dat.Rpul, mur, dat.len, frq_cal, frq_nn, 2);

% 
% frq_cal = dat.frq(dat.frq<=100e3);
% frq_nn = dat.frq(dat.frq>100e3);
% [RmeshMF,LmeshMF] = main_mesh2d_nn_sub_val(RmeshMF,LmeshMF,frq_cal, frq_nn, 2);


errR = zeros(Nc,Nf);
figure(1)
subplot(2,1,1)
hold on
for ik=1:Nc
    plot(dat.frq/1e3,squeeze(dat.RmeshMF(ik,ik,:))*1e3)
    %plot(dat.frq/1e3,squeeze(RmeshMF(ik,ik,:))*1e3,'--')
    plot(dat.frq/1e3,squeeze(Rself(ik,:))*1e3,'--')
end
ylabel('Resistance(mohm)')
xlabel('Frequency(kHz)')
subplot(2,1,2)
hold on
for ik=1:Nc
    errR(ik,:) = -(squeeze(dat.RmeshMF(ik,ik,:))-Rself(ik,:)')./squeeze(dat.RmeshMF(ik,ik,:))*100;
    plot(dat.frq/1e3,errR(ik,:))
end
xlabel('Frequency(kHz)')
ylabel('Error(%)')

errL = zeros(Nc,Nf);
figure(2)
subplot(2,1,1)
hold on
for ik=1:Nc
    plot(dat.frq/1e3,squeeze(dat.LmeshMF(ik,ik,:))*1e6)
    plot(dat.frq/1e3,(Lself(ik,:))*1e6,'--')
end
xlabel('Frequency(kHz)')
ylabel('Self Inductance(uH)')
subplot(2,1,2)
hold on
for ik=1:Nc
    errL(ik,:) = -(squeeze(dat.LmeshMF(ik,ik,:))-(Lself(ik,:)'))./squeeze(dat.LmeshMF(ik,ik,:))*100;
    plot(dat.frq/1e3,errL(ik,:))
end
xlabel('Frequency(kHz)')
ylabel('Error(%)')

errML = zeros(Nc*(Nc+1)/2,Nf);
figure(4)
subplot(2,1,1)
hold on
for ik=1:Nc
    for ig=ik:Nc
        plot(dat.frq/1e3,squeeze(dat.LmeshMF(ik,ig,:))*1e6)
        plot(dat.frq/1e3,squeeze(LmeshMF(ik,ig,:))*1e6,'--')
    end
end
xlabel('Frequency(kHz)')
ylabel('Mutual Inductance(uH)')
subplot(2,1,2)
hold on
cnt = 0;
for ik=1:Nc
    for ig=ik:Nc
        cnt = cnt+1;
        errML(cnt,:) = -(squeeze(dat.LmeshMF(ik,ig,:))-squeeze(LmeshMF(ik,ig,:)))./squeeze(dat.LmeshMF(ik,ig,:))*100;
        plot(dat.frq/1e3,errML(cnt,:))
    end
end
xlabel('Frequency(kHz)')
ylabel('Error(%)')


figure(6);
subplot(3,1,1);
plot(errR(:,13:end),'DisplayName','errML(:,13:end)');
legend('200kHz','500Hz','1Mhz')
title('Error in Resistance(%)')
subplot(3,1,2);
plot(errL(:,13:end),'DisplayName','errML(:,13:end)');
legend('200kHz','500Hz','1Mhz')
title('Error in Self Inductance(%)')
subplot(3,1,3);
plot(errML(:,13:end),'DisplayName','errML(:,13:end)')
legend('200kHz','500Hz','1Mhz')
title('Error in Mutual Inductance(%)')

