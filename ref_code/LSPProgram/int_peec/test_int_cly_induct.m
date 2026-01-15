mu0 = 4*pi*1e-7;
%% parameters of tower section
ps1 = [0 0 0];
ps2 = [0 0 10];
r1 = [5e-3;];
[dv1, len1] = line_dv(ps1,ps2);

Nint = 5;

% field point
pf1 = [r1 0 0];
pf2 = [r1 0 10];
r2 = r1;
dv2 = dv1;


% Perimeter of the shell
dSshell0 = 2*pi*r1;


int = induct_cyl_num(r1, len1)
int = mu0/(4*pi)*int_fila_p(ps1, ps2, dv1, r1, pf1, pf2, dv1, r2)
% cyliner shell - filament integration
mu0/(4*pi)./(dSshell0)*int_cyl_fila_p(ps1, ps2, dv1, r1, pf1, pf2, dv2, Nint)
% cyliner shell - cyliner shell integration
mu0/(4*pi)./(dSshell0.^2)*int_cyl_p(ps1, ps2, dv1, r1, pf1, pf2, dv2, r2, Nint)

%% these two formulas match well with induc_cyl_num 
% Perimeter of the shell
dSshell = 2*pi*r1;

% annular - filametn integration (used for 2D meshing)
r0 = r1;
b0 = 0;
d0 = r1;
dSa = pi*(r1^2-(0.9*r1)^2);
mu0/(4*pi)./(dSa)*int_anl_fila_p3d(0, 2*pi, 0.9*r1, r1, r0, b0, d0, len1, Nint)

% arc - filament integratoin (used for 2D meshing)
mu0/(4*pi)./(dSshell)*int_arc_fila_p3d(0, 2*pi, r1, r0, b0, d0, len1, Nint)


