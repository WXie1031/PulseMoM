

% frq = [1 50 100 200 500 1e3 2e3  5e3 10e3 20e3 50e3 100e3  200e3 500e3];%
frq = [1 50 100 200 500 1e3 2e3  5e3 10e3 20e3 50e3];%
mur = 35;

% frq = 1e6;

shape = 0;
N=10;
M=10;


W=10e-3;
T=10e-3;
Rdc = [1.28]*1e-3;


% shape: 0-rectangle  1-round   2-annular
% shape = 1;
% W = 5e-3*2;
% Rdc = [3.6250]*1e-3;

[Rmag0, Lmag0] = cal_RL_rec_mag_2d(shape, W, T, N,M, Rdc, mur, frq);
[Rmag2D, Lmag2D] = cal_RL_rec_mag_2d_test(shape, W, T, N,M, Rdc, 1, frq);
% [Rmag2Dtest, Lmag2Dtest] = cal_RL_rec_mag_loop_2d_test(W, T,Rdc, mur, frq);
%  [Rmag, Lmag] = SOL_M0(W, T, N,M, Rdc, mur, frq);

Ltmp = 4*2e-7*(log(2*1.65)-1)*40e-3*4e-3;

sig = 1./(Rdc.*W.*T);
for ik = 1:length(frq)
    Rskin(ik) = resis_bar_ac(W, T, Rdc, sig, 16, 1, frq(ik));
end

figure(4);
hold on
 semilogx(frq/1e3,squeeze(Rmag0(1,:))*1e3);
semilogx(frq/1e3,squeeze(Rmag2D(1,:))*1e3,'r.-');
semilogx(frq/1e3,Rskin*1e3,'--');
% semilogx(frq/1e3,squeeze(Rmag2Dtest(1,:))*1e3);
% semilogx(frq/1e3,squeeze(Rmag(1,:))*1e3);
xlabel('f(kHz)');
ylabel('R(mohm)');
grid on

figure(5);
hold on
 semilogx(frq/1e3,squeeze(Lmag0(1,:))*1e6);
semilogx(frq/1e3,squeeze(Lmag2D(1,:))*1e6,'k.-');
% semilogx(frq/1e3,squeeze(Lmag2Dtest(1,:))*1e6);
% semilogx(frq/1e3,squeeze(Lmag(1,:))*1e6);
xlabel('f(kHz)');
ylabel('L(uH)');
grid on



