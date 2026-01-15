mu0 = 4*pi*1e-7;

shp_id = [1005;
    1005;
   1005;
    1005;
    1005;
    1100;];

pt_2D = [    0.6  0;
    0.6 0.4;
    0.6 0.46;
    -0.6 0.34;
    -0.6 0.4;
    0  0;];

dim1 = [5e-3; 5e-3; 5e-3; 5e-3; 5e-3; 0.8 ];
dim2 = [0; 0; 0; 0; 0; 0.7];
sig = [5.8e7; 5.8e7;  5.8e7; 5.8e7; 5.8e7;1e7;];
S = pi*(dim1.^2-dim2.^2);
Rpul = 1./(sig.*S);
mur = [1; 1; 1; 1; 1;40;];

len = 10;

frq = 10e3;
p_flag = 0;

% [Lmesh2D] = cal_L_mesh2d_spt(p2Dcir, p2Drec, p2Dagi, ...
%     p2Dspt, rospt, rispt, Rpuspt, murspt, lgrp, f0)
[Rmesh,Lmesh, Rself,Lself, RmeshMF,LmeshMF] = main_mesh2d_cmplt( ...
    shp_id, pt_2D, dim1, dim2, Rpul, sig,mur, len, frq, p_flag);


% Lext = induct_cir_ext(r1, len)
% induct_cyl_num(r1, len)

shp_id = [1005;
    1100;];

pt_2D = [0.6  0;
    0  0;];

dim1 = [5e-3; 0.8 ];
dim2 = [0; 0.7];
sig = [5.8e7; 1e7;];
S = pi*(dim1.^2-dim2.^2);
Rpul = 1./(sig.*S);
mur = [1; 1;];

len = 10;

frq = 10e3;
p_flag = 0;

[Rmesh,Lmesh, Rself,Lself, RmeshMF,LmeshMF] = main_mesh2d_cmplt( ...
    shp_id, pt_2D, dim1, dim2, Rpul, sig,mur, len, frq, p_flag);


