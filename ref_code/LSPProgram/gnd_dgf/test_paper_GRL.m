I1 = exp(2*1j*kz(1,1:Np).*(0.25*ones(1,Np)))*dkp;
figure(14);
subplot(3,2,1)
hold on
plot(real(kp),sign(real(I1)).*log10(abs(real(I1))))
ylabel('log_1_0(Re I_1)')
subplot(3,2,2)
hold on
plot(real(kp),sign(imag(I1)).*log10(abs(imag(I1))))
ylabel('log_1_0(Im I_1)')
xlim([0 100]);


I2 = 1/(8*pi)*( -1j*kz(1,1:Np).*GRe_ij(1,:)./(sig_lyr(1)+1j*w0*ep0*epr_lyr(1)) ...
    - 1j*w0*mu_lyr(1)*GRh_ij(1,:)./(1j*kz(1,1:Np)) )*dkp;
subplot(3,2,3)
hold on
plot(real(kp),sign(real(I2)).*log10(abs(real(I2))))
ylabel('log_1_0(Re I_2)')
subplot(3,2,4)
hold on
plot(real(kp),sign(imag(I2)).*log10(abs(imag(I2))))
ylabel('log_1_0(Im I_2)')

It = I1.*I2/dkp;
subplot(3,2,5)
hold on
plot(real(kp),sign(real(It)).*log10(abs(real(It))))
ylabel('log_1_0(Re I)')
subplot(3,2,6)
hold on
plot(real(kp),sign(imag(It)).*log10(abs(imag(It))))
ylabel('log_1_0(Im I)')

