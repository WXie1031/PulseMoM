
frq = [1 50 100 200 500 1e3 2e3  5e3  10e3 20e3 50e3 100e3  200e3 500e3 1e6];
%frq = linspace(1,1e6, 20);
Nf = size(frq,2);
murtmp = 1;
sig = 5.8e7;

% DEVICE_TYPESIGN_CONDUCTOR_BCF = 1002;
% DEVICE_TYPESIGN_CONDUCTOR_BCL = 1003;
data_proximity = cell(20,1);

k = 0;

%% data 1
k = k+1;
dat.frq = frq;
dat.shp_id = [2100; 2100;];
dat.pt_2d = [0 0; 10 0;]*1e-3;
dat.dim1 = [5; 5;]*1e-3;
dat.dim2 = [0; 0;]*1e-3;

dat.Rpul = [2.1952e-04; 2.1952e-04];
dat.len = 3;

Nc = size(dat.pt_2d,1);
mur = ones(Nc,Nf)*murtmp;

[Rmesh1,Lmesh1, Rself1,Lself1, dat.RmeshMF,dat.LmeshMF] = main_mesh2d_cmplt( ...
    dat.shp_id, dat.pt_2d, dat.dim1, dat.dim2, dat.Rpul,mur,dat.len, dat.frq);

data_proximity{k,1} = dat;


%% data 2
k = k+1;
dat.frq = frq;
dat.shp_id = [2100; 2100;];
dat.pt_2d = [0 0; 20 0;]*1e-3;
dat.dim1 = [5; 5;]*1e-3;
dat.dim2 = [0; 0;]*1e-3;

dat.Rpul = [2.1952e-04; 2.1952e-04];
dat.len = 3;

Nc = size(dat.pt_2d,1);
mur = ones(Nc,Nf)*murtmp;

[Rmesh1,Lmesh1, Rself1,Lself1, dat.RmeshMF,dat.LmeshMF] = main_mesh2d_cmplt( ...
    dat.shp_id, dat.pt_2d, dat.dim1, dat.dim2, dat.Rpul,mur,dat.len, dat.frq);

data_proximity{k,1} = dat;

%% data 3
k = k+1;
dat.frq = frq;
dat.shp_id = [2100; 2100;];
dat.pt_2d = [0 0; 30 0;]*1e-3;
dat.dim1 = [5; 5;]*1e-3;
dat.dim2 = [0; 0;]*1e-3;

dat.Rpul = [2.1952e-04; 2.1952e-04];
dat.len = 3;

Nc = size(dat.pt_2d,1);
mur = ones(Nc,Nf)*murtmp;

[Rmesh1,Lmesh1, Rself1,Lself1, dat.RmeshMF,dat.LmeshMF] = main_mesh2d_cmplt( ...
    dat.shp_id, dat.pt_2d, dat.dim1, dat.dim2, dat.Rpul,mur,dat.len, dat.frq);

data_proximity{k,1} = dat;



%% data 4
k = k+1;
dat.frq = frq;
dat.shp_id = [2100; 2100;];
dat.pt_2d = [0 0; 10 0;]*1e-3;
dat.dim1 = [5; 1;]*1e-3;
dat.dim2 = [0; 0;]*1e-3;

dat.Rpul = [2.1952e-04; 2.1952e-04];
dat.len = 3;

Nc = size(dat.pt_2d,1);
mur = ones(Nc,Nf)*murtmp;

[Rmesh1,Lmesh1, Rself1,Lself1, dat.RmeshMF,dat.LmeshMF] = main_mesh2d_cmplt( ...
    dat.shp_id, dat.pt_2d, dat.dim1, dat.dim2, dat.Rpul,mur,dat.len, dat.frq);

data_proximity{k,1} = dat;


%% data 5
k = k+1;
dat.frq = frq;
dat.shp_id = [2100; 2100;];
dat.pt_2d = [0 0; 20 0;]*1e-3;
dat.dim1 = [5; 1;]*1e-3;
dat.dim2 = [0; 0;]*1e-3;

dat.Rpul = [2.1952e-04; 2.1952e-04];
dat.len = 3;

Nc = size(dat.pt_2d,1);
mur = ones(Nc,Nf)*murtmp;

[Rmesh1,Lmesh1, Rself1,Lself1, dat.RmeshMF,dat.LmeshMF] = main_mesh2d_cmplt( ...
    dat.shp_id, dat.pt_2d, dat.dim1, dat.dim2, dat.Rpul,mur,dat.len, dat.frq);

data_proximity{k,1} = dat;

%% data 6
k = k+1;
dat.frq = frq;
dat.shp_id = [2100; 2100;];
dat.pt_2d = [0 0; 30 0;]*1e-3;
dat.dim1 = [5; 1;]*1e-3;
dat.dim2 = [0; 0;]*1e-3;

dat.Rpul = [2.1952e-04; 2.1952e-04];
dat.len = 3;

Nc = size(dat.pt_2d,1);
mur = ones(Nc,Nf)*murtmp;

[Rmesh1,Lmesh1, Rself1,Lself1, dat.RmeshMF,dat.LmeshMF] = main_mesh2d_cmplt( ...
    dat.shp_id, dat.pt_2d, dat.dim1, dat.dim2, dat.Rpul,mur,dat.len, dat.frq);

data_proximity{k,1} = dat;


%% data 7
k = k+1;
dat.frq = frq;
dat.shp_id = [2100; 2100;2100;2100];

dis = 12;
dat.pt_2d = [0 0; 0 dis;-dis*cos(pi/6)  -dis*sin(pi/6);dis*cos(pi/6)  -dis*sin(pi/6);]*1e-3;
dat.dim1 = [2.8209; 10; 10; 10;]*1e-3;
dat.dim2 = [0; 9; 9; 9;]*1e-3;

dat.Rpul = [6.8968e-04; 4.8584e-04;4.8584e-04;4.8584e-04;];
dat.len = 3;

Nc = size(dat.pt_2d,1);
mur = ones(Nc,Nf)*murtmp;

[Rmesh1,Lmesh1, Rself1,Lself1, dat.RmeshMF,dat.LmeshMF] = main_mesh2d_cmplt( ...
    dat.shp_id, dat.pt_2d, dat.dim1, dat.dim2, dat.Rpul,mur,dat.len, dat.frq);

data_proximity{k,1} = dat;


%% data 8
k = k+1;
dat.frq = frq;
dat.shp_id = [2100; 2100;2100;2100];

dis = 17;
dat.pt_2d = [0 0; 0 dis;-dis*cos(pi/6)  -dis*sin(pi/6);dis*cos(pi/6)  -dis*sin(pi/6);]*1e-3;
dat.dim1 = [2.8209; 10; 10; 10;]*1e-3;
dat.dim2 = [0; 9; 9; 9;]*1e-3;

dat.Rpul = [6.8968e-04; 4.8584e-04;4.8584e-04;4.8584e-04;];
dat.len = 3;

Nc = size(dat.pt_2d,1);
mur = ones(Nc,Nf)*murtmp;

[Rmesh1,Lmesh1, Rself1,Lself1, dat.RmeshMF,dat.LmeshMF] = main_mesh2d_cmplt( ...
    dat.shp_id, dat.pt_2d, dat.dim1, dat.dim2, dat.Rpul,mur,dat.len, dat.frq);

data_proximity{k,1} = dat;


%% data 9
k = k+1;
dat.frq = frq;
dat.shp_id = [2100; 2100;2100;2100];

dis = 22;
dat.pt_2d = [0 0; 0 dis;-dis*cos(pi/6)  -dis*sin(pi/6);dis*cos(pi/6)  -dis*sin(pi/6);]*1e-3;
dat.dim1 = [2.8209; 10; 10; 10;]*1e-3;
dat.dim2 = [0; 9; 9; 9;]*1e-3;

dat.Rpul = [6.8968e-04; 4.8584e-04;4.8584e-04;4.8584e-04;];
dat.len = 3;

Nc = size(dat.pt_2d,1);
mur = ones(Nc,Nf)*murtmp;

[Rmesh1,Lmesh1, Rself1,Lself1, dat.RmeshMF,dat.LmeshMF] = main_mesh2d_cmplt( ...
    dat.shp_id, dat.pt_2d, dat.dim1, dat.dim2, dat.Rpul,mur,dat.len, dat.frq);

data_proximity{k,1} = dat;




%% data 12
k = k+1;
dat.shp_id = [2100; 2100; 1003; 1003;];
dat.pt_2d = [0 80; 0 80; 0 0; 300 0;]*1e-3;
dat.dim1 = [12.45; 4.5; 50; -50]*1e-3;
dat.dim2 = [11.65; 0;  5;  5;]*1e-3;
dat.Rpul = [1.18e-3;  1.05e-3;  0.226123e-3; 0.226123e-3];
dat.len = 3;

Nc = size(dat.pt_2d,1);
mur = ones(Nc,Nf)*murtmp;

[Rmesh1,Lmesh1, Rself1,Lself1, dat.RmeshMF,dat.LmeshMF] = main_mesh2d_cmplt( ...
    dat.shp_id, dat.pt_2d, dat.dim1, dat.dim2, dat.Rpul,mur,dat.len, dat.frq);

data_proximity{k,1} = dat;

%% data 13
k = k+1;
dat.frq = frq;
dat.shp_id = [2100; 2100; 2100; 2100;2100; 2100;...
    2100; 2100;2100; 2100;2100; 2100; ...
    1003; 1003;];
dat.pt_2d = [100 12.5; 100 12.5; 100 52.5; 100 52.5; 100 92.5;100 92.5; ...
    140 12.5; 140 12.5; 140 52.5; 140 52.5; 140 92.5;140 92.5; ...
    0 0; 300 0;]*1e-3;
dat.dim1 = [12.45; 4.5; 12.45; 4.5; 12.45; 4.5;...
    12.45; 4.5; 12.45; 4.5; 12.45; 4.5;...
    50; -50]*1e-3;
dat.dim2 = [11.65; 0; 11.65; 0; 11.65; 0; ...
    11.65; 0; 11.65; 0; 11.65; 0; ...
    5;  5;]*1e-3;
dat.Rpul = [1.18e-3;  1.05e-3; 1.18e-3;  1.05e-3; 1.18e-3;  1.05e-3; ...
    1.18e-3;  1.05e-3; 1.18e-3;  1.05e-3; 1.18e-3;  1.05e-3; ...
    0.226123e-3; 0.226123e-3];
dat.len = 3;

murtmp = 1;

Nc = size(dat.pt_2d,1);
mur = ones(Nc,Nf)*murtmp;

[Rmesh2,Lmesh2, Rself2,Lself2, dat.RmeshMF,dat.LmeshMF] = main_mesh2d_cmplt( ...
    dat.shp_id, dat.pt_2d, dat.dim1, dat.dim2, dat.Rpul,mur,dat.len, dat.frq);
data_proximity{k,1} = dat;

%% data 14
k = k+1;
dat.frq = frq;
dat.shp_id = [2100; 2100; 2100; 2100;2100; 2100;...
    2100; 2100;2100; 2100;2100; 2100; ...
    2100; 2100;2100;];

dat.pt_2d = [100 12.5; 100 12.5; 137.5 12.5; 137.5 12.5; 175 12.5; 175 12.5; ...
    212.5 12.5; 212.5 12.5; 250 12.5; 250 12.5; 287.5 12.5; 287.5 12.5; ...
    315 4; 313 4; 317 4;]*1e-3;
dat.dim1 = [12.45; 4.5; 12.45; 4.5; 12.45; 4.5;...
    12.45; 4.5; 12.45; 4.5; 12.45; 4.5;...
    4; 1.3; 1.3]*1e-3;
dat.dim2 = [11.65; 0; 11.65; 0; 11.65; 0; ...
    11.65; 0; 11.65; 0; 11.65; 0; ...
    3.4; 0; 0;]*1e-3;
dat.Rpul = [1.18e-3;  1.05e-3; 1.18e-3;  1.05e-3; 1.18e-3;  1.05e-3; ...
    1.18e-3;  1.05e-3; 1.18e-3;  1.05e-3; 1.18e-3;  1.05e-3; ...
    1.2e-3; 3.2e-3; 3.2e-3];
dat.len = 3;

murtmp = 1;

Nc = size(dat.pt_2d,1);
mur = ones(Nc,Nf)*murtmp;

[Rmesh2,Lmesh2, Rself2,Lself2, dat.RmeshMF,dat.LmeshMF] = main_mesh2d_cmplt( ...
    dat.shp_id, dat.pt_2d, dat.dim1, dat.dim2, dat.Rpul,mur,dat.len, dat.frq);

data_proximity{k,1} = dat;

%% save the data
save('database_nn_lin20pt', 'data_proximity');





