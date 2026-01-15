ep0 = 8.85*1e-12;
%% parameters of tower section
ps1 = [0 0 0];
ps2 = [0 0 10];
r1 = [1e-3;];
[dv1, len1] = line_dv(ps1,ps2);

Nint = 15;

% field point
pf1 = ps1;
pf2 = ps2;
r2 = r1;
dv2 = dv1;


% Perimeter of the shell
dCshell = 2*pi*r1;




%% this formula match well with induc_cyl_num 
% arc - filament integratoin (used for 2D meshing)
% r0 = r1;
% b0 = 0;
% d0 = 0;
% 1/(4*pi*ep0)/(len1)/(len1)./(dCshell)*int_arc_fila_p3d(0, 2*pi, r1, r0, b0, d0, len1, Nint)

% self inductance for cylinder - filament formula
1/(4*pi*ep0)./(len1*len1) .* int_fila_p(ps1,ps2,dv1,r1,pf1,pf2,dv2,r2)/len1

cal_P_fila(ps1,ps2,dv1,len1,r1, pf1,pf2,dv2,len1,r2)/len1

cal_L_fila(ps1,ps2,dv1,len1,r1, pf1,pf2,dv2,len1,r2)/len1
