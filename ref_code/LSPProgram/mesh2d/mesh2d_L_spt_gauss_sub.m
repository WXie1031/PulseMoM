function Gwei = mesh2d_L_spt_gauss_sub(ang_rng, ang_oth, off_gwei)

Nang = size(ang_rng,2);
Nf = size(ang_oth,1);

sigma = 0.2;

ang_tmp = (ang_rng-2*pi);
Gwei_neg = sum(exp(-1/2*((ang_tmp-ang_oth*ones(1,Nang))/sigma).^2)/(sqrt(2*pi)*sigma),1)/Nf;

ang_tmp = (ang_rng+2*pi);
Gwei_pos = sum(exp(-1/2*((ang_tmp-ang_oth*ones(1,Nang))/sigma).^2)/(sqrt(2*pi)*sigma),1)/Nf;

Gwei = sum(exp(-1/2*((ang_rng-ang_oth*ones(1,Nang))/sigma).^2)/(sqrt(2*pi)*sigma),1)/Nf;

Gwei = (Gwei+Gwei_neg+Gwei_pos) + off_gwei;

% figure;
% hold on
% plot(ang_rng, Gwei_neg,'k-', 'LineWidth',1);
% plot(ang_rng, Gwei_pos,'k-.', 'LineWidth',1);
% plot(ang_rng, Gwei);
% set(gca,'xtick',0:pi/2:2*pi)
% set(gca,'xticklabel',{'0','\pi/2','\pi','3\pi/2','2\pi'})
% xlim([0 2*pi])

end