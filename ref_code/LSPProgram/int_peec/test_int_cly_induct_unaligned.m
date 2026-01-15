mu0 = 4*pi*1e-7;

%% parameters of tower section
ps1 = [0 0 0];
ps2 = [0 0 10];
r1 = [500e-3;];
[dv1, len1] = line_dv(ps1,ps2);

Nint = 5;

% field point
pf1 = [0 0 10];
pf2 = [0 0 15];
r2 = 200e-3;
dv2 = dv1;


% Perimeter of the shell
dSshell0 = 2*pi*r1;


int = mu0/(4*pi)*int_fila_p(ps1, ps2, dv1, r1, pf1, pf2, dv1, r2)
% cyliner shell - filament integration
mu0/(4*pi)./(dSshell0)*int_cyl_fila_p(ps1, ps2, dv1, r1, pf1, pf2, dv2, Nint)
% cyliner shell - cyliner shell integration
mu0/(4*pi)./(dSshell0.^2)*int_cyl_p(ps1, ps2, dv1, r1, pf1, pf2, dv2, r2, Nint)


