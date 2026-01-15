% configuration of the medium
d1 = 3e-3;
epr1 =1;
mur1 = 1;
sig1 = 3.8e7;

d2 = 2e-3;
epr2 =1;
mur2 = 4000;
sig2 = 2.17e6;




frq = 50;
w0 = 2*pi*frq;

mu0 = 4e-7*pi;
ep0 = 8.854187817e-12;

% configuration of the source
Irms = 200;

% measured points
y_meas = 300e-3;
x_meas = linspace(-250,250,100)*1e-3;

xs1 = -0.1;
ys1 = -200e-3;
xs2 = 0.1;
ys2 = -200e-3;
xn = x_meas;
yn = y_meas;
[Hx1, Hy1] = h_multi_inf_fila(xs1, ys1, xn, yn);
[Hx2, Hy2] = h_multi_inf_fila(xs2, ys2, xn, yn);
Hx = Irms*Hx1-Irms*Hx2;
Hy = Irms*Hy1-Irms*Hy2;
Br = mu0*sqrt(Hx.^2+Hy.^2);


T = double_shield_t_h_iw(d1,sig1,mur1,epr1, d2,sig2,mur2,epr2, w0);


% Bcmplx = B_field_noshield*exp(-1j*k0*r);

B_withshield = Br*T;
max(abs(B_withshield))*1e6


if length(x_meas)==1
    pt_axis = y_meas;
else
    pt_axis = x_meas;
end

figure(3)
hold on
plot(pt_axis, Br*1e6)
plot(pt_axis, abs(B_withshield)*1e6)
xlabel('Distance (m)')
ylabel('B Flux (uH)')



