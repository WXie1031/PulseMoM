

% frq = [1 50 100 200 500 1e3 2e3  5e3 10e3 20e3 50e3 100e3  200e3 500e3];%
frq = [10 100 200 500 1e3 2e3  5e3 10e3 20e3 50e3];%


% frq = 1e6;
mur = 10;

sig = 1e7;
epr = 1;

rout = 1e-3/2;
rin = 0;
len = 10;

Rpul = 1/(sig*pi*rout^2);


%% rectangle 2d
% shape: 0-rectangle  1-round   2-annular
% shape = 1;
% W = 5e-3*2;
% Rdc = [3.6250]*1e-3;

shape = 1;
W=2*rout;
T=[];
N = 20;
M=[];
[Rmag0, Lmag0] = cal_RL_rec_mag_2d(shape, W, T, N,M, Rpul, mur, frq);




%% new developed p3d

pt_2d = [0,0];
[~,~,~,~,~,RmeshMF,LmeshMF] = main_mesh2d_cmplt(1, pt_2d, rout,rin, ...
    Rpul, sig,mur,epr, len, frq);
Rmag2D = RmeshMF(:)/len;
Lmag2D = LmeshMF(:)/len;


%% skin effect
[R, Lin, Zin] = zin_cir_ac(rout, rin, sig, mur, len, frq);
R = R/len;
d = rout;
% Lext = 2*1e-7*len*(log(len/d+sqrt((len/d)^2+1))-sqrt(1+(d/len)^2)+d/len);
Lext = induct_cir_dc(rout, len, 0);
L = (Lin+Lext)/len;


figure(4);
semilogx(frq/1e3,R*1e3,'k');
hold on
semilogx(frq/1e3,(Rmag0)*1e3,'r');
semilogx(frq/1e3,(Rmag2D)*1e3,'b');
xlabel('f(kHz)');
ylabel('R(mohm)');
legend('Skin','2D','P3D')
grid on

figure(5);
semilogx(frq/1e3,L*1e6,'k');
hold on
semilogx(frq/1e3,(Lmag0)*1e6,'r');
semilogx(frq/1e3,(Lmag2D)*1e6,'b');
xlabel('f(kHz)');
ylabel('L(uH)');
legend('Skin','2D','P3D')

grid on



