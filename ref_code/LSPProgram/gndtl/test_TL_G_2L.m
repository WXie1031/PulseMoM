
pt_2d = [ 0 2
    0 1;
    0 -0.5;
    0 -2];

re = [4e-3;4e-3;4e-3;4e-3];

len = 2.5;


frq = [1 100 200 500 1e3 2e3 5e3 10e3 20e3 50e3 100e3 200e3 500e3 1e6];
frq = [50 100 500 1000 5e3 10e3 50e3 100e3 500e3 1e6];


sig_g1 = 1/50;
epr_g1=1;
sig_g2=1/20;
epr_g2=1;
dg1=2.7;
[Rm2, Lm2, Zg,Rsg21,Lsg21] = zg_tline_ol_b2([], [], pt_2d,re, len, ...
    sig_g1,epr_g1,dg1, sig_g2,epr_g2, frq);

[Rsg22, Lsg22, Zsg] = zg_tline_log_ol_b2(pt_2d,re, len, ...
    sig_g1,epr_g1,dg1, sig_g2,epr_g2, frq);

[Rm2, Lm2, Zg,Rsg23,Lsg23] = zg_tline_uc_b2(Rm2, Lm2, pt_2d,re, len, ...
    sig_g1,epr_g1,dg1, sig_g2,epr_g2, frq);

[Rm2, Lm2, Zg3] = zg_tline_ou_b2(Rm2, Lm2, pt_2d,re, len, ...
    sig_g1,epr_g1,dg1, sig_g2,epr_g2, frq);




rg1 = sqrt( 1j.*w.*mu0.*(sig_g1 + 1j.*w.*ep0*epr_g1) );
rg2 = sqrt( 1j.*w.*mu0.*(sig_g2 + 1j.*w.*ep0*epr_g2) );
rg_eq = rg1.* ( (rg1+rg2)-(rg1-rg2).*exp(-2*rg1.*dg1) ) ./ ...
    ( (rg1+rg2)+(rg1-rg2).*exp(-2*rg1.*dg1) );
cplex_depth2 = 1./rg_eq;
Ztl2 = 1j*w.*mu0./(2*pi).*( log( (2*cplex_depth2)./re(1) ) ).*len;
Ztl2 = 1j*w.*mu0./(2*pi).*( log( 2*( abs(pt_2d(1,2))+cplex_depth2 )./re(1) ) ).*len;
Rg2 = real(Ztl2);
Lg2 = imag(Ztl2)./w;


figure;
hold on;
semilogx(frq, Rsg21(1:2,:),'k');
semilogx(frq, Rsg22(1:2,:),'k--s')
semilogx(frq, Rsg23(3:4,:),'k--x')
semilogx(frq, Rg2,'r--v')
hold off
grid on
xlabel('Frquency (Hz)');
ylabel('R (ohm/m)');
title('R -- 2 Layer Comparison');
legend('ol num1','ol num2','ol log1','ol log2','ub num1','ub num2','cplx');

figure;
hold on;
semilogx(frq, Lsg21(1:2,:)*1e6,'k');
semilogx(frq, Lsg22(1:2,:)*1e6,'k--s')
semilogx(frq, Lsg23(3:4,:)*1e6,'k--x')
semilogx(frq, Lg2*1e6,'r--v')
hold off
grid on
xlabel('Frquency (Hz)');
ylabel('L (uH/m)');
title('L -- 2 Layer Comparison');
legend('ol num1','ol num2','ol log1','ol log2','ub num1','ub num2','cplx');

figure;
hold on;
semilogx(frq, abs(Rsg21(1,:)+1j*w.*Lsg21(1,:)),'k');
semilogx(frq, abs(Rsg22(1,:)+1j*w.*Lsg22(1,:)),'k--s')
semilogx(frq, abs(Rsg23(3,:)+1j*w.*Lsg23(3,:)),'k--x')
semilogx(frq, abs(Rg2(1,:)+1j*w.*Lg2(1,:)),'r--v')
hold off
grid on
xlabel('Frquency (Hz)');
ylabel('|Z| (ohm/m)');
title('Z -- 2 Layer Comparison');