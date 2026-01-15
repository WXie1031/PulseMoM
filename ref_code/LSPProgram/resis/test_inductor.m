
len=1;
mur=35;

re = 5e-3;

f0=[1 50 100 500 1e3 2e3 5e3 10e3 20e3 50e3  100e3  500e3  1e6 2e6];
%f0=[10e3 ];
sig = 1/(3.6250e-3*(pi*5e-3*5e-3));
Nf=length(f0);
R = zeros(1,Nf);
Lin = zeros(1,Nf);
for ik=1:Nf 
    [R(ik), Lin(ik)] = zin_cir_ac(re, 0, sig, mur, len, f0(ik));
end
Lin*1e6

Lext =  induct_cir_ext(re, len);
M =  induct_cir_ext(len, len);
Lt = Lext+Lin+M;

%R*1e3
%Lt*1e6

% figure(1);
% hold on;
% plot(f0/1e3,R*1e3);
% 
% figure(2);
% hold on;
% plot(f0/1e3,Lt*1e6);



re = sqrt(6/pi)*1e-3;

len = 100;
mur = 1;
sig = 5e8;

f0 = [ 1  100  1e3  10e3  20e3 ];

Nf=length(f0);
R = zeros(1,Nf);
Lin = zeros(1,Nf);

ep0 = 8.85*1e-12;
Pcol = 1/(4*pi*ep0)./(len*len) .* int_fila_p([0 0 0],[0 100 0],[0 1 0],re, ...
            [0 0 0],[0 100 0],[0 1 0],re);
Ct = 1/Pcol;

for ik=1:Nf 
    [R(ik), Lin(ik)] = zin_cir_ac(re, 0, sig, mur, len, f0(ik));
end
Lext =  induct_cir_ext(re, len);
R
Lt = Lext+Lin

fr = 1./( 2*pi*sqrt(Lt*Ct) );


