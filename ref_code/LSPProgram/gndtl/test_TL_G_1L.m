
pt_2d = [ 0 2
    0 1;
    0 -0.5;
    0 -2];

re = [4e-3;4e-3;4e-3;4e-3];

len = 2.5;
sig_soil = 1/65;
epr_soil = 1;

frq = [1 100 200 500 1e3 2e3 5e3 10e3 20e3 50e3 100e3 200e3 500e3 1e6];
frq = [50 100 500 1000 5e3 10e3 50e3 100e3 500e3 1e6];

[Rm1, Lm1, ~,Rsg11,Lsg11] = zg_tline_ol_b1([], [], pt_2d,re, len, ...
    sig_soil, epr_soil, frq);

[Rsg12, Lsg12, Zsg12] = zg_tline_log_ol_b1(pt_2d,re, len, ...
    sig_soil, epr_soil, frq);

[Rm1, Lm1, ~,Rsg13,Lsg13] = zg_tline_uc_b1(Rm1, Lm1, pt_2d,re, len, ...
    sig_soil, epr_soil, frq);

[Rm1, Lm1, ~] = zg_tline_ou_b1(Rm1, Lm1, pt_2d,re, len, ...
    sig_soil, epr_soil, frq);


mu0 = 4*pi*1e-7;
ep0 = 8.85e-12;
w = 2*pi*frq;
ep_soil = epr_soil*ep0;

rg = sqrt( 1j*w*mu0.*(sig_soil + 1j*w*ep_soil) );
cplx_depth1 = 1./rg;   % complex skin depth

Ztl1 = 1j*w.*mu0./(2*pi).* log( (2*(abs(pt_2d(1,2))+cplx_depth1)./re(1) ) ).*len;
Rg1 = real(Ztl1);
Lg1 = imag(Ztl1)./w;


% Ytl = rg.^2./Zg
Zg1 = Rsg11(2,:)+1j*w.*Lsg11(2,:);
Zg2 = Rsg12(2,:)+1j*w.*Lsg12(2,:);
Zg3 = Rsg13(3,:)+1j*w.*Lsg13(3,:);
Zg4 = Rg1(1,:)+1j*w.*Lg1(1,:);
Yg1 = rg.^2./Zg1;
Yg2 = rg.^2./Zg2;
Yg3 = rg.^2./Zg3;
Yg4 = rg.^2./Zg4;

figure;
loglog(frq, Rsg11(1:2,:),'k');
hold on;
semilogx(frq, Rsg12(1:2,:),'k--s')
semilogx(frq, Rsg13(3:4,:),'k--x')
semilogx(frq, Rg1,'r--v')
hold off
grid on
xlabel('Frquency (Hz)');
ylabel('R (ohm/m)');
title('R -- 1 Layer Comparison');
legend('ol num1','ol num2','ol log1','ol log2','ub num1','ub num2','cplx');

figure;
loglog(frq, Lsg11(1:2,:)*1e6,'k');
hold on;
semilogx(frq, Lsg12(1:2,:)*1e6,'k--s')
semilogx(frq, Lsg13(3:4,:)*1e6,'k--x')
semilogx(frq, Lg1*1e6,'r--v')
hold off
grid on
xlabel('Frquency (Hz)');
ylabel('L (uH/m)');
title('L -- 1 Layer Comparison');
legend('ol num1','ol num2','ol log1','ol log2','ub num1','ub num2','cplx');

figure;
loglog(frq, abs(Rsg11(2,:)+1j*w.*Lsg11(2,:)),'k');
hold on;
semilogx(frq, abs(Rsg12(2,:)+1j*w.*Lsg12(2,:)),'k--s')
semilogx(frq, abs(Rsg13(3,:)+1j*w.*Lsg13(3,:)),'k--x')
semilogx(frq, abs(Rg1(1,:)+1j*w.*Lg1(1,:)),'r--v')
hold off
grid on
xlabel('Frquency (Hz)');
ylabel('|Z| (ohm/m)');
title('Z -- 1 Layer Comparison');
legend('ol num1','ol log1','ub num2','cplx');

figure;
loglog(frq, abs(Yg1),'k');
hold on;
semilogx(frq, abs(Yg2),'k--s')
semilogx(frq, abs(Yg3),'k--x')
semilogx(frq, abs(Yg4),'r--v')
hold off
grid on
xlabel('Frquency (Hz)');
ylabel('|Y| (ohm/m)');
title('Z -- 1 Layer Comparison');
legend('ol num1','ol log1','ub num2','cplx');