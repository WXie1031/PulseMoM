mu0 = 4*pi*1e-7;
rw=5e-3; 
d=15e-3;
%d=2;
Lreturn = mu0/pi*log(  d/(rw) );  % transmissline return inductance

mur = 35;
sig = 3512400;
len =1;
f0 = [1 50 100 200 500 1e3 2e3 5e3 10e3 20e3 50e3 100e3 200e3 500e3 1e6 2e6];
Nf = length(f0);
R = zeros(1,Nf);
Lin = zeros(1,Nf);
for ik=1:Nf
[R(ik), Lin(ik)] =zin_cir_ac(rw, 0, sig, mur, len, f0(ik));
end

R*1e3*2
(Lin*2+Lreturn)*1e6
% 
% figure(1)
% hold on
% plot(f0/1e3, Lin*1e6);
