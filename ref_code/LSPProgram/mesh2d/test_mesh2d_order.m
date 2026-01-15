
frq = 20e3;

shp_id = [1100; 2100; 2100; 1002; 1003;];
pt_2D = [0 0; 0 460; 0 480; 0 0; 0 -300;]*1e-3;
dim1 = [500; 5; 5; 40;  40]*1e-3;
dim2 = [490; 0; 0; 4;  4;]*1e-3;
sig = [1.5e6;   5.8e7;  5.8e7; 5.8e7;   1e6;];
S = pi*(dim1.^2-dim2.^2);
Rpul = 1./(sig.*S);

mur = [1;1;1;1;1;];


len = 1;

[Rmesh1, Lmesh1] = main_mesh2d_cmplt(shp_id, pt_2D, dim1, dim2, ...
    Rpul, sig, mur, len, frq);


shp_id = [ 2100; 2100; 1002; 1003; 1100;];
pt_2D = [ 0 460; 0 480; 0 0; 0 -300; 0 0;]*1e-3;
dim1 = [ 5; 5;  40;  40;  500;]*1e-3;
dim2 = [  0; 0;  4;  4;  490; ]*1e-3;
sig = [  5.8e7; 5.8e7;   5.8e7;   1e6;  1.5e6; ];
S = pi*(dim1.^2-dim2.^2);
Rpul = 1./(sig.*S);


[Rmesh2, Lmesh2] = main_mesh2d_cmplt_backup( shp_id, pt_2D, dim1, dim2, ...
    Rpul, sig, mur, len, frq);



