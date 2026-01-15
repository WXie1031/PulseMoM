
fname = 'errNN_case40_rat.mat';

load(fname);

frq_plt = [500,200e3,1e6];
Nfplt = length(frq_plt);

frq = dat.frq;

figure(5)
subplot(3,1,1)
hold on
for ik = 1:Nfplt
ind = frq == frq_plt(ik);
idmodify = errR(:,ind)>10;
% errR(idmodify,ind) = errR(idmodify,ind)-sign(errR(idmodify,ind)).*floor(abs(errR(idmodify,ind)))+sign(errR(idmodify,ind))*9;
plot(errR(:,ind))
end
legend('500Hz','200kHz','1MHz')
title('Error in Resistance (%)')
axis([0 inf  -10 10 ])

subplot(3,1,2)
hold on
for ik = 1:Nfplt
ind = frq == frq_plt(ik);
idmodify = errL(:,ind)>2;
% errL(idmodify,ind) = errL(idmodify,ind)-sign(errL(idmodify,ind)).*fix(errL(idmodify,ind))+sign(errL(idmodify,ind))*2;
plot(errL(:,ind))
end
legend('500Hz','200kHz','1MHz')
title('Error in Self Inductance (%)')
axis([0 inf  -3 3 ])

subplot(3,1,3)
hold on
for ik = 1:Nfplt
ind = frq == frq_plt(ik);
plot(errML(:,ind))
end
legend('500Hz','200kHz','1MHz')
title('Error in Mutual Indutance (%)')
axis([0 inf  -2 2 ])

