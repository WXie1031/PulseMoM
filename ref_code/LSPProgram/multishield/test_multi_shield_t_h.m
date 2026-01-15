

Nf = 100;

frq = logspace(3,10,Nf);

frq=50;
Nf = length(frq);

mu0 = 4e-7*pi;
ep0 = 8.854187817e-12;
% configuration of the medium
epr_lyr = [ 1; 1; 1; 1];
mur_lyr = [1; 1; 8000; 1; ];
sig_lyr = [0; 5.8e7; 1e6;  0];
zbdy = [0; -0.25; -0.5; ]*1e-3;  % Relative coordinates

% configuration of the source
d = 0.2;
h = 0.2;
Irms = 50;


% measured points
z_meas = linspace(100,500,20)*1e-3;


% field without shield
k0 = 2*pi*frq*sqrt(mu0.*ep0);
[h_field_noshield, h_ang] = h_oppo_inf_fila(d, h+z_meas);
B_field_noshield  = mu0*Irms*h_field_noshield;

xs1 = -0.1;
ys1 = 0;
xs2 = 0.1;
ys2 = 0;
xn = 0;
yn = h+z_meas;
[Hx1, Hy1] = h_multi_inf_fila(xs1, ys1, xn, yn);
[Hx2, Hy2] = h_multi_inf_fila(xs2, ys2, xn, yn);
Hx = Hx1+Hx2;
Hy = Hy1+Hy2;
Br = mu0*sqrt(Hx.^2+Hy.^2);


T = zeros(Nf,1); 
for ik = 1:Nf
    f0 = frq(ik);
    w0= 2*pi*f0;

    T(ik) = multi_shield_t_h(h_ang, zbdy, epr_lyr,mur_lyr,sig_lyr, w0);
end

% B = B0*exp(j*k0*y)
r = linspace(0.1,0.5,20);

% Bcmplx = B_field_noshield*exp(-1j*k0*r);

B_field_withshield = Br*T;

figure(3)
hold on
plot(z_meas, Br*1e6)
plot(z_meas, abs(B_field_withshield)*1e6)
xlabel('Distance (m)')
ylabel('H Filed (uH)')



