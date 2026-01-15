
frq = [1 50 100 200 500 1e3 2e3 5e3 10e3 20e3 50e3 100e3  200e3 500e3 1e6];
frq = [1 50 100 200 300 400 500 700 1e3 1.5e3 2e3 3e3 4e3 5e3 7e3 10e3 15e3 20e3 30e3 40e3 50e3 70e3 100e3 150e3 200e3 300e3 400e3 500e3 700e3 1e6 1.2e6 1.5e6 2e6 2.5e6 3.5e6 5e6];
frq_interp = linspace(1,1e6, 50);
Nf = size(frq,2);
murtmp = 1;
sig_cu = 5.8e7;

sname = 'database_nn_interp2';

% DEVICE_TYPESIGN_CONDUCTOR_BCF = 1002;
% DEVICE_TYPESIGN_CONDUCTOR_BCL = 1003;
data_proximity = cell(20,1);

k = 0;

%% data 1x2 r=5mm conductors - close - 1m
% O O
k = k+1;
dat.info = '1x2 r=5mm conductors - close'; 
dat.frq = frq;
dat.shp_id = [2100; 2100;];
dat.pt_2d = [0 0; 10 0;]*1e-3;
dat.dim1 = [5; 5;]*1e-3;
dat.dim2 = [0; 0;]*1e-3;
dta.S = pi*(dat.dim1.^2-dat.dim2.^2);
dat.Rpul = 1./(sig_cu.*dta.S);
dat.len = 1;

Nc = size(dat.pt_2d,1);
mur = ones(Nc,Nf)*murtmp;

[Rmesh1,Lmesh1, Rself1,Lself1, dat.RmeshMF,dat.LmeshMF] = mesh2d_main_complete( ...
    dat.shp_id, dat.pt_2d, dat.dim1, dat.dim2, dat.Rpul,mur,dat.len, dat.frq);

data_proximity{k,1} = dat;


%% data 1x2 r=5mm conductors - close - 3m
% O O
k = k+1;
dat.info = '1x2 r=5mm conductors - close'; 
dat.frq = frq;
dat.shp_id = [2100; 2100;];
dat.pt_2d = [0 0; 10 0;]*1e-3;
dat.dim1 = [5; 5;]*1e-3;
dat.dim2 = [0; 0;]*1e-3;
dta.S = pi*(dat.dim1.^2-dat.dim2.^2);
dat.Rpul = 1./(sig_cu.*dta.S);
dat.len = 3;

Nc = size(dat.pt_2d,1);
mur = ones(Nc,Nf)*murtmp;

[Rmesh1,Lmesh1, Rself1,Lself1, dat.RmeshMF,dat.LmeshMF] = mesh2d_main_complete( ...
    dat.shp_id, dat.pt_2d, dat.dim1, dat.dim2, dat.Rpul,mur,dat.len, dat.frq);

data_proximity{k,1} = dat;


%% data 1x2 r=5mm conductors - close - 5m
% O O
k = k+1;
dat.info = '1x2 r=5mm conductors - close - 5m'; 
dat.frq = frq;
dat.shp_id = [2100; 2100;];
dat.pt_2d = [0 0; 10 0;]*1e-3;
dat.dim1 = [5; 5;]*1e-3;
dat.dim2 = [0; 0;]*1e-3;
dta.S = pi*(dat.dim1.^2-dat.dim2.^2);
dat.Rpul = 1./(sig_cu.*dta.S);
dat.len = 5;

Nc = size(dat.pt_2d,1);
mur = ones(Nc,Nf)*murtmp;

[Rmesh1,Lmesh1, Rself1,Lself1, dat.RmeshMF,dat.LmeshMF] = mesh2d_main_complete( ...
    dat.shp_id, dat.pt_2d, dat.dim1, dat.dim2, dat.Rpul,mur,dat.len, dat.frq);

data_proximity{k,1} = dat;


%% data 1x2 r=5mm conductors - mid distance - 1m
% O  O
k = k+1;
dat.info = '1x2 r=5mm conductors - mid distance - 1m'; 
dat.frq = frq;
dat.shp_id = [2100; 2100;];
dat.pt_2d = [0 0; 20 0;]*1e-3;
dat.dim1 = [5; 5;]*1e-3;
dat.dim2 = [0; 0;]*1e-3;
dta.S = pi*(dat.dim1.^2-dat.dim2.^2);
dat.Rpul = 1./(sig_cu.*dta.S);
dat.len = 1;

Nc = size(dat.pt_2d,1);
mur = ones(Nc,Nf)*murtmp;

[Rmesh1,Lmesh1, Rself1,Lself1, dat.RmeshMF,dat.LmeshMF] = mesh2d_main_complete( ...
    dat.shp_id, dat.pt_2d, dat.dim1, dat.dim2, dat.Rpul,mur,dat.len, dat.frq);

data_proximity{k,1} = dat;


%% data 1x2 r=5mm conductors - mid distance - 4m
% O  O
k = k+1;
dat.info = '1x2 r=5mm conductors - mid distance - 4m'; 
dat.frq = frq;
dat.shp_id = [2100; 2100;];
dat.pt_2d = [0 0; 20 0;]*1e-3;
dat.dim1 = [5; 5;]*1e-3;
dat.dim2 = [0; 0;]*1e-3;
dta.S = pi*(dat.dim1.^2-dat.dim2.^2);
dat.Rpul = 1./(sig_cu.*dta.S);
dat.len = 4;

Nc = size(dat.pt_2d,1);
mur = ones(Nc,Nf)*murtmp;

[Rmesh1,Lmesh1, Rself1,Lself1, dat.RmeshMF,dat.LmeshMF] = mesh2d_main_complete( ...
    dat.shp_id, dat.pt_2d, dat.dim1, dat.dim2, dat.Rpul,mur,dat.len, dat.frq);

data_proximity{k,1} = dat;



%% data 1x2 r=5mm conductors - far distance - 2m
% O   O
k = k+1;
dat.info = '1x2 r=5mm conductors - far distance - 2m'; 
dat.frq = frq;
dat.shp_id = [2100; 2100;];
dat.pt_2d = [0 0; 30 0;]*1e-3;
dat.dim1 = [5; 5;]*1e-3;
dat.dim2 = [0; 0;]*1e-3;
dta.S = pi*(dat.dim1.^2-dat.dim2.^2);
dat.Rpul = 1./(sig_cu.*dta.S);
dat.len = 2;

Nc = size(dat.pt_2d,1);
mur = ones(Nc,Nf)*murtmp;

[Rmesh1,Lmesh1, Rself1,Lself1, dat.RmeshMF,dat.LmeshMF] = mesh2d_main_complete( ...
    dat.shp_id, dat.pt_2d, dat.dim1, dat.dim2, dat.Rpul,mur,dat.len, dat.frq);

data_proximity{k,1} = dat;


%% data 1x2 r=5mm conductors - far distance - 3.5m
% O   O
k = k+1;
dat.info = '1x2 r=5mm conductors - far distance - 3.5m'; 
dat.frq = frq;
dat.shp_id = [2100; 2100;];
dat.pt_2d = [0 0; 30 0;]*1e-3;
dat.dim1 = [5; 5;]*1e-3;
dat.dim2 = [0; 0;]*1e-3;
dta.S = pi*(dat.dim1.^2-dat.dim2.^2);
dat.Rpul = 1./(sig_cu.*dta.S);
dat.len = 3.5;

Nc = size(dat.pt_2d,1);
mur = ones(Nc,Nf)*murtmp;

[Rmesh1,Lmesh1, Rself1,Lself1, dat.RmeshMF,dat.LmeshMF] = mesh2d_main_complete( ...
    dat.shp_id, dat.pt_2d, dat.dim1, dat.dim2, dat.Rpul,mur,dat.len, dat.frq);

data_proximity{k,1} = dat;


%% data 1x2 r=5mm & r=1mm conductors - close - 2.5m
k = k+1;
dat.info = '1x2 r=5mm + r=1mm conductors - close - 2.5m'; 
% O o
dat.frq = frq;
dat.shp_id = [2100; 2100;];
dat.pt_2d = [0 0; 10 0;]*1e-3;
dat.dim1 = [5; 1;]*1e-3;
dat.dim2 = [0; 0;]*1e-3;
dta.S = pi*(dat.dim1.^2-dat.dim2.^2);
dat.Rpul = 1./(sig_cu.*dta.S);
dat.len = 2.5;

Nc = size(dat.pt_2d,1);
mur = ones(Nc,Nf)*murtmp;

[Rmesh1,Lmesh1, Rself1,Lself1, dat.RmeshMF,dat.LmeshMF] = mesh2d_main_complete( ...
    dat.shp_id, dat.pt_2d, dat.dim1, dat.dim2, dat.Rpul,mur,dat.len, dat.frq);

data_proximity{k,1} = dat;


%% data 1x2 r=5mm + r=1mm conductors - close - 5m
k = k+1;
dat.info = '1x2 r=5mm + r=1mm conductors - close - 5m'; 
% O o
dat.frq = frq;
dat.shp_id = [2100; 2100;];
dat.pt_2d = [0 0; 10 0;]*1e-3;
dat.dim1 = [5; 1;]*1e-3;
dat.dim2 = [0; 0;]*1e-3;
dta.S = pi*(dat.dim1.^2-dat.dim2.^2);
dat.Rpul = 1./(sig_cu.*dta.S);
dat.len = 5;

Nc = size(dat.pt_2d,1);
mur = ones(Nc,Nf)*murtmp;

[Rmesh1,Lmesh1, Rself1,Lself1, dat.RmeshMF,dat.LmeshMF] = mesh2d_main_complete( ...
    dat.shp_id, dat.pt_2d, dat.dim1, dat.dim2, dat.Rpul,mur,dat.len, dat.frq);

data_proximity{k,1} = dat;



%% data 1x2 r=5mm + r=1mm conductors - mid distance - 1m
% O  o
k = k+1;
dat.info = '1x2 r=5mm + r=1mm conductors - mid distance - 1m'; 
dat.frq = frq;
dat.shp_id = [2100; 2100;];
dat.pt_2d = [0 0; 20 0;]*1e-3;
dat.dim1 = [5; 1;]*1e-3;
dat.dim2 = [0; 0;]*1e-3;
dta.S = pi*(dat.dim1.^2-dat.dim2.^2);
dat.Rpul = 1./(sig_cu.*dta.S);
dat.len = 1;

Nc = size(dat.pt_2d,1);
mur = ones(Nc,Nf)*murtmp;

[Rmesh1,Lmesh1, Rself1,Lself1, dat.RmeshMF,dat.LmeshMF] = mesh2d_main_complete( ...
    dat.shp_id, dat.pt_2d, dat.dim1, dat.dim2, dat.Rpul,mur,dat.len, dat.frq);

data_proximity{k,1} = dat;

%% data 1x2 r=5mm + r=1mm conductors - mid distance - 3m
% O  o
k = k+1;
dat.info = '1x2 r=5mm + r=1mm conductors - mid distance - 3m'; 
dat.frq = frq;
dat.shp_id = [2100; 2100;];
dat.pt_2d = [0 0; 20 0;]*1e-3;
dat.dim1 = [5; 1;]*1e-3;
dat.dim2 = [0; 0;]*1e-3;
dta.S = pi*(dat.dim1.^2-dat.dim2.^2);
dat.Rpul = 1./(sig_cu.*dta.S);
dat.len = 3;

Nc = size(dat.pt_2d,1);
mur = ones(Nc,Nf)*murtmp;

[Rmesh1,Lmesh1, Rself1,Lself1, dat.RmeshMF,dat.LmeshMF] = mesh2d_main_complete( ...
    dat.shp_id, dat.pt_2d, dat.dim1, dat.dim2, dat.Rpul,mur,dat.len, dat.frq);

data_proximity{k,1} = dat;


%% data 1x2 r=5mm + r=1mm conductors - far distance - 1.5m
% O   o
k = k+1;
dat.info = '1x2 r=5mm + r=1mm conductors - far distance - 1.5m'; 
dat.frq = frq;
dat.shp_id = [2100; 2100;];
dat.pt_2d = [0 0; 30 0;]*1e-3;
dat.dim1 = [5; 1;]*1e-3;
dat.dim2 = [0; 0;]*1e-3;
dta.S = pi*(dat.dim1.^2-dat.dim2.^2);
dat.Rpul = 1./(sig_cu.*dta.S);
dat.len = 1.5;

Nc = size(dat.pt_2d,1);
mur = ones(Nc,Nf)*murtmp;

[Rmesh1,Lmesh1, Rself1,Lself1, dat.RmeshMF,dat.LmeshMF] = mesh2d_main_complete( ...
    dat.shp_id, dat.pt_2d, dat.dim1, dat.dim2, dat.Rpul,mur,dat.len, dat.frq);

data_proximity{k,1} = dat;


%% data 1x2 r=5mm + r=1mm conductors - far distance - 4.5m
% O   o
k = k+1;
dat.info = '1x2 r=5mm + r=1mm conductors - far distance - 4.5m'; 
dat.frq = frq;
dat.shp_id = [2100; 2100;];
dat.pt_2d = [0 0; 30 0;]*1e-3;
dat.dim1 = [5; 1;]*1e-3;
dat.dim2 = [0; 0;]*1e-3;
dta.S = pi*(dat.dim1.^2-dat.dim2.^2);
dat.Rpul = 1./(sig_cu.*dta.S);
dat.len = 4.5;

Nc = size(dat.pt_2d,1);
mur = ones(Nc,Nf)*murtmp;

[Rmesh1,Lmesh1, Rself1,Lself1, dat.RmeshMF,dat.LmeshMF] = mesh2d_main_complete( ...
    dat.shp_id, dat.pt_2d, dat.dim1, dat.dim2, dat.Rpul,mur,dat.len, dat.frq);

data_proximity{k,1} = dat;



%% data 1x2 r=5mm conductor + cox cable - close
% O O
k = k+1;
dat.info = '1x2 r=5mm conductor + cox cable - close'; 
dat.frq = frq;
dat.shp_id = [2100; 2100; 2100];
dat.pt_2d = [0 0; 10 0;10 0;]*1e-3;
dat.dim1 = [5; 5; 1]*1e-3;
dat.dim2 = [0; 4.2; 0]*1e-3;
dta.S = pi*(dat.dim1.^2-dat.dim2.^2);
dat.Rpul = 1./(sig_cu.*dta.S);
dat.len = 1.2;

Nc = size(dat.pt_2d,1);
mur = ones(Nc,Nf)*murtmp;

[Rmesh1,Lmesh1, Rself1,Lself1, dat.RmeshMF,dat.LmeshMF] = mesh2d_main_complete( ...
    dat.shp_id, dat.pt_2d, dat.dim1, dat.dim2, dat.Rpul,mur,dat.len, dat.frq);

data_proximity{k,1} = dat;


%% data 1x2 r=5mm conductor + cox - mid distance
% O  O
k = k+1;
dat.info = '1x2 r=5mm conductor + cox - mid distance'; 
dat.frq = frq;
dat.shp_id = [2100; 2100;2100;];
dat.pt_2d = [0 0; 20 0; 20 0;]*1e-3;
dat.dim1 = [5; 5; 1]*1e-3;
dat.dim2 = [0; 4.2; 0]*1e-3;
dta.S = pi*(dat.dim1.^2-dat.dim2.^2);
dat.Rpul = 1./(sig_cu.*dta.S);
dat.len = 1;

Nc = size(dat.pt_2d,1);
mur = ones(Nc,Nf)*murtmp;

[Rmesh1,Lmesh1, Rself1,Lself1, dat.RmeshMF,dat.LmeshMF] = mesh2d_main_complete( ...
    dat.shp_id, dat.pt_2d, dat.dim1, dat.dim2, dat.Rpul,mur,dat.len, dat.frq);

data_proximity{k,1} = dat;

%% data 1x2 r=5mm conductor + cox - far distance
% O   O
k = k+1;
dat.info = '1x2 r=5mm conductor + cox - far distance'; 
dat.frq = frq;
dat.shp_id = [2100; 2100;2100;];
dat.pt_2d = [0 0; 30 0; 30 0;]*1e-3;
dat.dim1 = [5; 5; 1]*1e-3;
dat.dim2 = [0; 4.2; 0]*1e-3;
dta.S = pi*(dat.dim1.^2-dat.dim2.^2);
dat.Rpul = 1./(sig_cu.*dta.S);
dat.len = 5;

Nc = size(dat.pt_2d,1);
mur = ones(Nc,Nf)*murtmp;

[Rmesh1,Lmesh1, Rself1,Lself1, dat.RmeshMF,dat.LmeshMF] = mesh2d_main_complete( ...
    dat.shp_id, dat.pt_2d, dat.dim1, dat.dim2, dat.Rpul,mur,dat.len, dat.frq);

data_proximity{k,1} = dat;



%% data 1x2 r=1mm conductor + r=5mm cox - close distance
k = k+1;
dat.info = '1x2 r=1mm conductor + r=5mm cox - close'; 
% O o
dat.frq = frq;
dat.shp_id = [2100; 2100;2100;];
dat.pt_2d = [0 0; 10 0; 10 0]*1e-3;
dat.dim1 = [1; 5; 1]*1e-3;
dat.dim2 = [0; 4.2; 0]*1e-3;
dta.S = pi*(dat.dim1.^2-dat.dim2.^2);
dat.Rpul = 1./(sig_cu.*dta.S);
dat.len = 4.5;

Nc = size(dat.pt_2d,1);
mur = ones(Nc,Nf)*murtmp;

[Rmesh1,Lmesh1, Rself1,Lself1, dat.RmeshMF,dat.LmeshMF] = mesh2d_main_complete( ...
    dat.shp_id, dat.pt_2d, dat.dim1, dat.dim2, dat.Rpul,mur,dat.len, dat.frq);

data_proximity{k,1} = dat;


%% data 1x2 r=1mm conductor + r=5mm cox - mid distance
% O  o
k = k+1;
dat.info = '1x2 r=1mm conductor + r=5mm cox - mid distance'; 
dat.frq = frq;
dat.shp_id = [2100; 2100;2100];
dat.pt_2d = [0 0; 20 0;20 0]*1e-3;
dat.dim1 = [1; 5; 1]*1e-3;
dat.dim2 = [0; 4.2; 0]*1e-3;
dta.S = pi*(dat.dim1.^2-dat.dim2.^2);
dat.Rpul = 1./(sig_cu.*dta.S);
dat.len = 3.4;

Nc = size(dat.pt_2d,1);
mur = ones(Nc,Nf)*murtmp;

[Rmesh1,Lmesh1, Rself1,Lself1, dat.RmeshMF,dat.LmeshMF] = mesh2d_main_complete( ...
    dat.shp_id, dat.pt_2d, dat.dim1, dat.dim2, dat.Rpul,mur,dat.len, dat.frq);

data_proximity{k,1} = dat;

%% data 1x2 r=1mm conductor + r=5mm cox - far distance
% O   o
k = k+1;
dat.info = '1x2 r=1mm conductor + r=5mm cox - far distance'; 
dat.frq = frq;
dat.shp_id = [2100; 2100;2100];
dat.pt_2d = [0 0; 30 0;30 0;]*1e-3;
dat.dim1 = [1; 5; 1]*1e-3;
dat.dim2 = [0; 4.2; 0]*1e-3;
dta.S = pi*(dat.dim1.^2-dat.dim2.^2);
dat.Rpul = 1./(sig_cu.*dta.S);
dat.len = 1.5;

Nc = size(dat.pt_2d,1);
mur = ones(Nc,Nf)*murtmp;

[Rmesh1,Lmesh1, Rself1,Lself1, dat.RmeshMF,dat.LmeshMF] = mesh2d_main_complete( ...
    dat.shp_id, dat.pt_2d, dat.dim1, dat.dim2, dat.Rpul,mur,dat.len, dat.frq);

data_proximity{k,1} = dat;



%% data 1x2 r=5mm conductor + sdc cable - close
% O O
k = k+1;
dat.info = '1x2 r=5mm conductor + sdc cable - close'; 
dat.frq = frq;
dat.shp_id = [2100; 2100; 2100;2100];
dat.pt_2d = [0 0; 10 0;8 0;12 0;]*1e-3;
dat.dim1 = [5; 4; 1.3;1.3;]*1e-3;
dat.dim2 = [0; 3.4; 0;0;]*1e-3;
dta.S = pi*(dat.dim1.^2-dat.dim2.^2);
dat.Rpul = 1./(sig_cu.*dta.S);
dat.len = 2.2;

Nc = size(dat.pt_2d,1);
mur = ones(Nc,Nf)*murtmp;

[Rmesh1,Lmesh1, Rself1,Lself1, dat.RmeshMF,dat.LmeshMF] = mesh2d_main_complete( ...
    dat.shp_id, dat.pt_2d, dat.dim1, dat.dim2, dat.Rpul,mur,dat.len, dat.frq);

data_proximity{k,1} = dat;


%% data 1x2 r=5mm conductor + sdc - mid distance
% O  O
k = k+1;
dat.info = '1x2 r=5mm conductor + sdc - mid distance'; 
dat.frq = frq;
dat.shp_id = [2100; 2100;2100;2100];
dat.pt_2d = [0 0; 20 0; 18 0; 22 0;]*1e-3;
dat.dim1 = [5; 4; 1.3;1.3;]*1e-3;
dat.dim2 = [0; 3.4; 0;0;]*1e-3;
dta.S = pi*(dat.dim1.^2-dat.dim2.^2);
dat.Rpul = 1./(sig_cu.*dta.S);
dat.len = 1.7;

Nc = size(dat.pt_2d,1);
mur = ones(Nc,Nf)*murtmp;

[Rmesh1,Lmesh1, Rself1,Lself1, dat.RmeshMF,dat.LmeshMF] = mesh2d_main_complete( ...
    dat.shp_id, dat.pt_2d, dat.dim1, dat.dim2, dat.Rpul,mur,dat.len, dat.frq);

data_proximity{k,1} = dat;

%% data 1x2 r=5mm conductor + sdc - far distance
% O   O
k = k+1;
dat.info = '1x2 r=5mm conductor + sdc - far distance'; 
dat.frq = frq;
dat.shp_id = [2100; 2100;2100;2100];
dat.pt_2d = [0 0; 30 0; 28 0;32 0;]*1e-3;
dat.dim1 = [5; 4; 1.3;1.3;]*1e-3;
dat.dim2 = [0; 3.4; 0;0;]*1e-3;
dta.S = pi*(dat.dim1.^2-dat.dim2.^2);
dat.Rpul = 1./(sig_cu.*dta.S);
dat.len = 0.8;

Nc = size(dat.pt_2d,1);
mur = ones(Nc,Nf)*murtmp;

[Rmesh1,Lmesh1, Rself1,Lself1, dat.RmeshMF,dat.LmeshMF] = mesh2d_main_complete( ...
    dat.shp_id, dat.pt_2d, dat.dim1, dat.dim2, dat.Rpul,mur,dat.len, dat.frq);

data_proximity{k,1} = dat;



%% data 1x2 r=1mm conductor +sdc - close distance
k = k+1;
dat.info = '1x2 r=1mm conductor + sdc - close'; 
% O o
dat.frq = frq;
dat.shp_id = [2100; 2100;2100;2100];
dat.pt_2d = [0 0; 10 0; 8 0;12 0;]*1e-3;
dat.dim1 = [1; 4; 1.3;1.3;]*1e-3;
dat.dim2 = [0; 3.4; 0;0;]*1e-3;
dta.S = pi*(dat.dim1.^2-dat.dim2.^2);
dat.Rpul = 1./(sig_cu.*dta.S);
dat.len = 3.7;

Nc = size(dat.pt_2d,1);
mur = ones(Nc,Nf)*murtmp;

[Rmesh1,Lmesh1, Rself1,Lself1, dat.RmeshMF,dat.LmeshMF] = mesh2d_main_complete( ...
    dat.shp_id, dat.pt_2d, dat.dim1, dat.dim2, dat.Rpul,mur,dat.len, dat.frq);

data_proximity{k,1} = dat;


%% data 1x2 r=1mm conductor + sdc - mid distance
% O  o
k = k+1;
dat.info = '1x2 r=1mm conductor + sdc - mid distance'; 
dat.frq = frq;
dat.shp_id = [2100; 2100;2100;2100];
dat.pt_2d = [0 0; 20 0;18 0;22 0;]*1e-3;
dat.dim1 = [1; 4; 1.3;1.3;]*1e-3;
dat.dim2 = [0; 3.4; 0;0;]*1e-3;
dta.S = pi*(dat.dim1.^2-dat.dim2.^2);
dat.Rpul = 1./(sig_cu.*dta.S);
dat.len = 3.4;

Nc = size(dat.pt_2d,1);
mur = ones(Nc,Nf)*murtmp;

[Rmesh1,Lmesh1, Rself1,Lself1, dat.RmeshMF,dat.LmeshMF] = mesh2d_main_complete( ...
    dat.shp_id, dat.pt_2d, dat.dim1, dat.dim2, dat.Rpul,mur,dat.len, dat.frq);

data_proximity{k,1} = dat;

%% data 1x2 r=1mm conductor + sdc - far distance
% O   o
k = k+1;
dat.info = '1x2 r=1mm conductor + sdc - far distance'; 
dat.frq = frq;
dat.shp_id = [2100; 2100;2100;2100];
dat.pt_2d = [0 0; 30 0;28 0;32 0;]*1e-3;
dat.dim1 = [1; 4; 1.3;1.3;]*1e-3;
dat.dim2 = [0; 3.4; 0;0;]*1e-3;
dta.S = pi*(dat.dim1.^2-dat.dim2.^2);
dat.Rpul = 1./(sig_cu.*dta.S);
dat.len = 1.9;

Nc = size(dat.pt_2d,1);
mur = ones(Nc,Nf)*murtmp;

[Rmesh1,Lmesh1, Rself1,Lself1, dat.RmeshMF,dat.LmeshMF] = mesh2d_main_complete( ...
    dat.shp_id, dat.pt_2d, dat.dim1, dat.dim2, dat.Rpul,mur,dat.len, dat.frq);

data_proximity{k,1} = dat;


%% data 1x3 horizontal 3 coaxial cables - 20mm distance
%  O O O

k = k+1;
dat.info = '1x3 horizontal 3 coaxial cables'; 
dat.frq = frq;
dat.shp_id = [2100; 2100; 2100; 2100;2100; 2100;];

dat.pt_2d = [100 3; 100 3; 120 3; 120 3; 140 3; 140 3;]*1e-3;
dat.dim1 = [6.05; 1.8; 6.05; 1.8; 6.05; 1.8;]*1e-3;
dat.dim2 = [5.25; 0; 5.25; 0; 5.25; 0;]*1e-3;
% dat.Rpul = [1.18e-3;  1.05e-3; 1.18e-3;  1.05e-3; 1.18e-3;  1.05e-3; ...
%     1.18e-3;  1.05e-3; 1.18e-3;  1.05e-3; 1.18e-3;  1.05e-3; ...
%     1.2e-3; 3.2e-3; 3.2e-3];
dta.S = pi*(dat.dim1.^2-dat.dim2.^2);
dat.Rpul = 1./(sig_cu.*dta.S);
dat.len = 1.7;

Nc = size(dat.pt_2d,1);
mur = ones(Nc,Nf)*murtmp;

[Rmesh2,Lmesh2, Rself2,Lself2, dat.RmeshMF,dat.LmeshMF] = mesh2d_main_complete( ...
    dat.shp_id, dat.pt_2d, dat.dim1, dat.dim2, dat.Rpul,mur,dat.len, dat.frq);

data_proximity{k,1} = dat;



%% data 1x2 horizontal 3 cox cables + 1 sdc cable - small
%  O O o

k = k+1;
dat.info = '1x2 horizontal 3 coaxial cables + 1 sdc cable - small'; 
dat.frq = frq;
dat.shp_id = [2100;2100; 2100;2100; 2100;2100;2100;];

dat.pt_2d = [100 3; 100 3; 120 3; 120 3; 135 2; 133 2; 137 2;]*1e-3;
dat.dim1 = [6.05; 1.8; 6.05; 1.8; 4; 1.3; 1.3;]*1e-3;
dat.dim2 = [5.25; 0; 5.25; 0; 3.4; 0; 0;]*1e-3;
dta.S = pi*(dat.dim1.^2-dat.dim2.^2);
dat.Rpul = 1./(sig_cu.*dta.S);
dat.len = 2;

Nc = size(dat.pt_2d,1);
mur = ones(Nc,Nf)*murtmp;

[Rmesh2,Lmesh2, Rself2,Lself2, dat.RmeshMF,dat.LmeshMF] = mesh2d_main_complete( ...
    dat.shp_id, dat.pt_2d, dat.dim1, dat.dim2, dat.Rpul,mur,dat.len, dat.frq);

data_proximity{k,1} = dat;


%% data 1x2 horizontal 3 cox cables + 1 sdc cable - big
%  O O o

k = k+1;
dat.info = '1x2 horizontal 2 cox cables + 1 sdc cable - big'; 
dat.frq = frq;
dat.shp_id = [2100;2100; 2100;2100; 2100;2100;2100;];

dat.pt_2d = [100 12.5; 100 12.5; 137.5 12.5; 137.5 12.5; 160 4; 158 4; 162 4;]*1e-3;
dat.dim1 = [12.45; 4.5; 12.45; 4.5; 4; 1.3; 1.3;]*1e-3;
dat.dim2 = [11.65; 0; 11.65; 0; 3.4; 0; 0;]*1e-3;
dta.S = pi*(dat.dim1.^2-dat.dim2.^2);
dat.Rpul = 1./(sig_cu.*dta.S);
dat.len = 2.7;

Nc = size(dat.pt_2d,1);
mur = ones(Nc,Nf)*murtmp;

[Rmesh2,Lmesh2, Rself2,Lself2, dat.RmeshMF,dat.LmeshMF] = mesh2d_main_complete( ...
    dat.shp_id, dat.pt_2d, dat.dim1, dat.dim2, dat.Rpul,mur,dat.len, dat.frq);

data_proximity{k,1} = dat;


%% data 2x2 vetical 4 coaxial cables - 37mm distance
%  O O
%  O O
% -----
k = k+1;
dat.info = '2x2 vetical 4 coaxial cables'; 
dat.frq = frq;
dat.shp_id = [2100; 2100; 2100; 2100;2100; 2100; 2100; 2100;];
dat.pt_2d = [100 12.5; 100 12.5; 100 52.5; 100 52.5; 140 12.5; 140 12.5; 140 52.5;140 52.5; ]*1e-3;
dat.dim1 = [12.45; 4.5; 12.45; 4.5; 12.45; 4.5; 12.45; 4.5;]*1e-3;
dat.dim2 = [11.65; 0; 11.65; 0; 11.65; 0; 11.65; 0; ]*1e-3;
dta.S = pi*(dat.dim1.^2-dat.dim2.^2);
dat.Rpul = 1./(sig_cu.*dta.S);
dat.len = 2.5;

Nc = size(dat.pt_2d,1);
mur = ones(Nc,Nf)*murtmp;

[Rmesh2,Lmesh2, Rself2,Lself2, dat.RmeshMF,dat.LmeshMF] = mesh2d_main_complete( ...
    dat.shp_id, dat.pt_2d, dat.dim1, dat.dim2, dat.Rpul,mur,dat.len, dat.frq);
data_proximity{k,1} = dat;


%% data 2x2 vetical 4 cox cables + 1 cox cable - 37mm distance
%  O O
%  O O O
% -----
k = k+1;
dat.info = '2x2 vetical 4 coaxial cables + 1 coaxial cable'; 
dat.frq = frq;
dat.shp_id = [2100; 2100; 2100; 2100;2100; 2100;2100; 2100;2100; 2100;];
dat.pt_2d = [100 12.5; 100 12.5; 100 52.5; 100 52.5;  ...
    140 12.5; 140 12.5; 140 52.5; 140 52.5; 180 12.5; 180 12.5;]*1e-3;
dat.dim1 = [12.45; 4.5; 12.45; 4.5; 12.45; 4.5; 12.45; 4.5; 12.45; 4.5;]*1e-3;
dat.dim2 = [11.65; 0; 11.65; 0; 11.65; 0;11.65; 0; 11.65; 0;]*1e-3;
dta.S = pi*(dat.dim1.^2-dat.dim2.^2);
dat.Rpul = 1./(sig_cu.*dta.S);
dat.len = 2.;

Nc = size(dat.pt_2d,1);
mur = ones(Nc,Nf)*murtmp;

[Rmesh2,Lmesh2, Rself2,Lself2, dat.RmeshMF,dat.LmeshMF] = mesh2d_main_complete( ...
    dat.shp_id, dat.pt_2d, dat.dim1, dat.dim2, dat.Rpul,mur,dat.len, dat.frq);
data_proximity{k,1} = dat;


%% data 2x2 vetical 4 cox cables + 1 sdc cable - 37mm distance
%  O O
%  O O o
% -----
k = k+1;
dat.info = '2x2 vetical 4 coaxial cables + 1 sdc cable'; 
dat.frq = frq;
dat.shp_id = [2100; 2100; 2100; 2100;2100; 2100;2100; 2100;2100; 2100;2100;];
dat.pt_2d = [100 12.5; 100 12.5; 100 52.5; 100 52.5;  ...
    140 12.5; 140 12.5; 140 52.5; 140 52.5; 165 4; 163 4;167 4;]*1e-3;
dat.dim1 = [12.45; 4.5; 12.45; 4.5; 12.45; 4.5; 12.45; 4.5; 4;1.3;1.3;]*1e-3;
dat.dim2 = [11.65; 0; 11.65; 0; 11.65; 0;11.65; 0; 3.4;0;0;]*1e-3;
dta.S = pi*(dat.dim1.^2-dat.dim2.^2);
dat.Rpul = 1./(sig_cu.*dta.S);
dat.len = 1.7;

Nc = size(dat.pt_2d,1);
mur = ones(Nc,Nf)*murtmp;

[Rmesh2,Lmesh2, Rself2,Lself2, dat.RmeshMF,dat.LmeshMF] = mesh2d_main_complete( ...
    dat.shp_id, dat.pt_2d, dat.dim1, dat.dim2, dat.Rpul,mur,dat.len, dat.frq);
data_proximity{k,1} = dat;


%% data 2x2 vetical 4 cox cables + 2x1 vetical 2 sdc cables
%  O O o
%  O O o
% -----
k = k+1;
dat.info = '2x2 vetical 4 cox cables + 2x1 vetical 2 sdc cables'; 
dat.frq = frq;
dat.shp_id = [2100;2100; 2100;2100; 2100;2100; 2100;2100; 2100;2100;2100; 2100;2100;2100;];
dat.pt_2d = [100 12.5; 100 12.5; 100 52.5; 100 52.5; 140 12.5; 140 12.5; 140 52.5; 140 52.5;  ...
    165 4; 163 4;167 4; 165 15; 163 15;167 15;]*1e-3;
dat.dim1 = [12.45; 4.5; 12.45; 4.5; 12.45; 4.5; 12.45; 4.5; 4;1.3;1.3; 4;1.3;1.3;]*1e-3;
dat.dim2 = [11.65; 0; 11.65; 0; 11.65; 0;11.65; 0; 3.4;0;0; 3.4;0;0;]*1e-3;
dta.S = pi*(dat.dim1.^2-dat.dim2.^2);
dat.Rpul = 1./(sig_cu.*dta.S);
dat.len = 2.4;

Nc = size(dat.pt_2d,1);
mur = ones(Nc,Nf)*murtmp;

[Rmesh2,Lmesh2, Rself2,Lself2, dat.RmeshMF,dat.LmeshMF] = mesh2d_main_complete( ...
    dat.shp_id, dat.pt_2d, dat.dim1, dat.dim2, dat.Rpul,mur,dat.len, dat.frq);
data_proximity{k,1} = dat;


%% data 1x3 horizontal 3 cox cables + 1x2 horizontal 2 coaxial cables
%   O O 
%  O O O 

k = k+1;
dat.info = '1x3 horizontal 3 coaxial cables + 1x2 horizontal 2 coaxial cables'; 

dat.frq = frq;
dat.shp_id = [
    2100; 2100; 2100; 2100;2100; 2100; 2100; 2100;2100; 2100; ]; %sdc up 1-2

dat.pt_2d = [
    100 12.5; 100 12.5; 137.5 12.5; 137.5 12.5; 175 12.5; 175 12.5; ... %cox down 1-3
    118.75 45; 118.75 45; 156.25 45; 156.25 45;  %cox up 1-5
    ]*1e-3; 
dat.dim1 = [12.45; 4.5; 12.45; 4.5; 12.45; 4.5; 12.45; 4.5; 12.45; 4.5; ]*1e-3;
dat.dim2 = [11.65; 0; 11.65; 0; 11.65; 0; 11.65; 0; 11.65; 0; ]*1e-3;
dta.S = pi*(dat.dim1.^2-dat.dim2.^2);
dat.Rpul = 1./(sig_cu.*dta.S);
dat.len = 1.33;

Nc = size(dat.pt_2d,1);
mur = ones(Nc,Nf)*murtmp;

[Rmesh2,Lmesh2, Rself2,Lself2, dat.RmeshMF,dat.LmeshMF] = mesh2d_main_complete( ...
    dat.shp_id, dat.pt_2d, dat.dim1, dat.dim2, dat.Rpul,mur,dat.len, dat.frq);

data_proximity{k,1} = dat;

%% data 1x4 horizontal 4 sdc cables + 1x3 horizontal 3 sdc cables
%   o o o
%  o o o o

k = k+1;
dat.info = '1x4 horizontal sdc cables + 1x3 horizontal 3 sdc cables'; 

dat.frq = frq;
dat.shp_id = [2100; 2100;2100; 2100; 2100;2100; 2100; 2100;2100; 2100; 2100;2100; ...  %sdc down 1-4
    2100; 2100;2100; 2100; 2100;2100; ]; %sdc up 1-2

dat.pt_2d = [
    312 4; 310 4; 314 4; 330 4; 328 4; 332 4; 348 4; 346 4; 350 4; 366 4; 364 4; 368 4; ... %sdc down 1-4
    321 17.6; 319 17.6; 323 17.6; 339 17.6; 337 17.6; 341 17.6; ... %sdc up 1-2
    ]*1e-3; 
dat.dim1 = [4; 1.3; 1.3; 4; 1.3; 1.3; 4; 1.3; 1.3; 4; 1.3; 1.3;  4; 1.3; 1.3; 4; 1.3; 1.3;]*1e-3;
dat.dim2 = [3.4; 0; 0; 3.4; 0; 0; 3.4; 0; 0;  3.4; 0; 0; 3.4; 0; 0;  3.4; 0; 0;]*1e-3;
dta.S = pi*(dat.dim1.^2-dat.dim2.^2);
dat.Rpul = 1./(sig_cu.*dta.S);
dat.len = 3.65;

Nc = size(dat.pt_2d,1);
mur = ones(Nc,Nf)*murtmp;

[Rmesh2,Lmesh2, Rself2,Lself2, dat.RmeshMF,dat.LmeshMF] = mesh2d_main_complete( ...
    dat.shp_id, dat.pt_2d, dat.dim1, dat.dim2, dat.Rpul,mur,dat.len, dat.frq);

data_proximity{k,1} = dat;


%% data 3x3 9 coaxial cables - 37mm distance
%  O O O
%  O O O
%  O O O
% -----
k = k+1;
dat.info = '3x3 9 coaxial cables'; 
dat.frq = frq;
dat.shp_id = [2100; 2100; 2100; 2100;2100; 2100;2100; 2100; 2100; 2100;2100; 2100;...
    2100; 2100;2100; 2100;2100; 2100;];
dat.pt_2d = [100 12.5; 100 12.5; 100 52.5; 100 52.5; 100 92.5;100 92.5; ...
    140 12.5; 140 12.5; 140 52.5; 140 52.5; 140 92.5;140 92.5; ...
    180 12.5; 180 12.5; 180 52.5; 180 52.5; 180 92.5;180 92.5;]*1e-3;
dat.dim1 = [12.45; 4.5; 12.45; 4.5; 12.45; 4.5;12.45; 4.5; 12.45; 4.5; 12.45; 4.5;...
    12.45; 4.5; 12.45; 4.5; 12.45; 4.5;]*1e-3;

dat.dim2 = [11.65; 0; 11.65; 0; 11.65; 0; 11.65; 0; 11.65; 0; 11.65; 0; ...
    11.65; 0; 11.65; 0; 11.65; 0; ]*1e-3;
dta.S = pi*(dat.dim1.^2-dat.dim2.^2);
dat.Rpul = 1./(sig_cu.*dta.S);
dat.len = 1.5;

Nc = size(dat.pt_2d,1);
mur = ones(Nc,Nf)*murtmp;

[Rmesh2,Lmesh2, Rself2,Lself2, dat.RmeshMF,dat.LmeshMF] = mesh2d_main_complete( ...
    dat.shp_id, dat.pt_2d, dat.dim1, dat.dim2, dat.Rpul,mur,dat.len, dat.frq);
data_proximity{k,1} = dat;


%% data 3x3 9 sdc cables - 37mm distance
%  o o o
%  o o o
%  o o o
% -----
k = k+1;
dat.info = '3x3 9 sdc cables'; 
dat.frq = frq;
dat.shp_id = [2100;2100;2100; 2100;2100;2100; 2100;2100;2100; 2100;2100;2100;...
    2100;2100;2100; 2100;2100;2100; 2100;2100;2100; 2100;2100;2100; 2100;2100;2100;];
dat.pt_2d = [100 4; 98 4; 102 4; 100 19; 98 19;102 19; 100 34; 98 34;102 34;...
    120 4; 118 4; 122 4; 120 19; 118 19;122 19; 120 34; 118 34;122 34; ...
    140 4; 138 4; 142 4; 140 19; 138 19;142 19; 140 34; 138 34;142 34;]*1e-3;
dat.dim1 = [4; 1.3; 1.3; 4; 1.3; 1.3;4; 1.3; 1.3;4; 1.3; 1.3; 4; 1.3; 1.3;...
    4; 1.3; 1.3; 4; 1.3; 1.3; 4; 1.3; 1.3; 4; 1.3; 1.3;]*1e-3;

dat.dim2 = [3.4; 0; 0;3.4; 0; 0; 3.4; 0; 0;3.4; 0; 0;3.4; 0; 0; 3.4; 0; 0; ...
    3.4; 0; 0; 3.4; 0; 0;3.4; 0; 0; ]*1e-3;
dta.S = pi*(dat.dim1.^2-dat.dim2.^2);
dat.Rpul = 1./(sig_cu.*dta.S);
dat.len = 1.8;

Nc = size(dat.pt_2d,1);
mur = ones(Nc,Nf)*murtmp;

[Rmesh2,Lmesh2, Rself2,Lself2, dat.RmeshMF,dat.LmeshMF] = mesh2d_main_complete( ...
    dat.shp_id, dat.pt_2d, dat.dim1, dat.dim2, dat.Rpul,mur,dat.len, dat.frq);
data_proximity{k,1} = dat;



%% data 3x2 vetical 6 coaxial cables - 37mm distance
%  O O
%  O O
%  O O
% -----
k = k+1;
dat.info = '3x2 vetical 6 coaxial cables'; 
dat.frq = frq;
dat.shp_id = [2100; 2100; 2100; 2100;2100; 2100;...
    2100; 2100;2100; 2100;2100; 2100;];
dat.pt_2d = [100 12.5; 100 12.5; 100 52.5; 100 52.5; 100 92.5;100 92.5; ...
    140 12.5; 140 12.5; 140 52.5; 140 52.5; 140 92.5;140 92.5;]*1e-3;
dat.dim1 = [12.45; 4.5; 12.45; 4.5; 12.45; 4.5;...
    12.45; 4.5; 12.45; 4.5; 12.45; 4.5;]*1e-3;
dat.dim2 = [11.65; 0; 11.65; 0; 11.65; 0; ...
    11.65; 0; 11.65; 0; 11.65; 0; ]*1e-3;
dta.S = pi*(dat.dim1.^2-dat.dim2.^2);
dat.Rpul = 1./(sig_cu.*dta.S);
dat.len = 2.5;

Nc = size(dat.pt_2d,1);
mur = ones(Nc,Nf)*murtmp;

[Rmesh2,Lmesh2, Rself2,Lself2, dat.RmeshMF,dat.LmeshMF] = mesh2d_main_complete( ...
    dat.shp_id, dat.pt_2d, dat.dim1, dat.dim2, dat.Rpul,mur,dat.len, dat.frq);
data_proximity{k,1} = dat;


%% data 3x2 vetical 6 coaxial cables + 3 sdc cables - 37mm distance
%  O O o
%  O O o
%  O O o
% -------
k = k+1;
dat.info = '3x2 vetical 6 coaxial cables + 3 vertical sdc cables'; 
dat.frq = frq;
dat.shp_id = [2100;2100; 2100;2100; 2100;2100; 2100;2100; 2100;2100; 2100;2100;...
    2100;2100;2100; 2100;2100;2100; 2100;2100;2100; ];
dat.pt_2d = [100 12.5; 100 12.5; 100 52.5; 100 52.5; 100 92.5;100 92.5; ...
    140 12.5; 140 12.5; 140 52.5; 140 52.5; 140 92.5;140 92.5; ...
    165 4; 163 4; 167 4; 165 24; 163 24; 167 24; 165 44; 163 44; 167 44;]*1e-3;
dat.dim1 = [12.45; 4.5; 12.45; 4.5; 12.45; 4.5; 12.45; 4.5; 12.45; 4.5; 12.45; 4.5;...
    4; 1.3; 1.3; 4; 1.3; 1.3; 4; 1.3; 1.3]*1e-3;
dat.dim2 = [11.65; 0; 11.65; 0; 11.65; 0; 11.65; 0; 11.65; 0; 11.65; 0; ...
    3.4; 0; 0; 3.4; 0; 0; 3.4; 0; 0;]*1e-3;
dta.S = pi*(dat.dim1.^2-dat.dim2.^2);
dat.Rpul = 1./(sig_cu.*dta.S);
dat.len = 2.13;

Nc = size(dat.pt_2d,1);
mur = ones(Nc,Nf)*murtmp;

[Rmesh2,Lmesh2, Rself2,Lself2, dat.RmeshMF,dat.LmeshMF] = mesh2d_main_complete( ...
    dat.shp_id, dat.pt_2d, dat.dim1, dat.dim2, dat.Rpul,mur,dat.len, dat.frq);
data_proximity{k,1} = dat;



%% data 1x6 horizontal 6 coaxial cables - 37mm distance
%  O O O O O O

k = k+1;
dat.info = '1x6 horizontal 6 coaxial cables'; 
dat.frq = frq;
dat.shp_id = [2100; 2100; 2100; 2100;2100; 2100;...
    2100; 2100;2100; 2100;2100; 2100;];

dat.pt_2d = [100 12.5; 100 12.5; 137.5 12.5; 137.5 12.5; 175 12.5; 175 12.5; ...
    212.5 12.5; 212.5 12.5; 250 12.5; 250 12.5; 287.5 12.5; 287.5 12.5;]*1e-3;
dat.dim1 = [12.45; 4.5; 12.45; 4.5; 12.45; 4.5;...
    12.45; 4.5; 12.45; 4.5; 12.45; 4.5;]*1e-3;
dat.dim2 = [11.65; 0; 11.65; 0; 11.65; 0; ...
    11.65; 0; 11.65; 0; 11.65; 0;]*1e-3;
% dat.Rpul = [1.18e-3;  1.05e-3; 1.18e-3;  1.05e-3; 1.18e-3;  1.05e-3; ...
%     1.18e-3;  1.05e-3; 1.18e-3;  1.05e-3; 1.18e-3;  1.05e-3; ...
%     1.2e-3; 3.2e-3; 3.2e-3];
dta.S = pi*(dat.dim1.^2-dat.dim2.^2);
dat.Rpul = 1./(sig_cu.*dta.S);
dat.len = 3;

Nc = size(dat.pt_2d,1);
mur = ones(Nc,Nf)*murtmp;

[Rmesh2,Lmesh2, Rself2,Lself2, dat.RmeshMF,dat.LmeshMF] = mesh2d_main_complete( ...
    dat.shp_id, dat.pt_2d, dat.dim1, dat.dim2, dat.Rpul,mur,dat.len, dat.frq);

data_proximity{k,1} = dat;


%% data 1x6 horizontal 6 coaxial cables + 1 sdc cable - 37mm distance
%  O O O O O O o

k = k+1;
dat.info = '1x6 horizontal 6 coaxial cables + 1 sdc cable'; 
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
% dat.Rpul = [1.18e-3;  1.05e-3; 1.18e-3;  1.05e-3; 1.18e-3;  1.05e-3; ...
%     1.18e-3;  1.05e-3; 1.18e-3;  1.05e-3; 1.18e-3;  1.05e-3; ...
%     1.2e-3; 3.2e-3; 3.2e-3];
dta.S = pi*(dat.dim1.^2-dat.dim2.^2);
dat.Rpul = 1./(sig_cu.*dta.S);
dat.len = 1.3;

Nc = size(dat.pt_2d,1);
mur = ones(Nc,Nf)*murtmp;

[Rmesh2,Lmesh2, Rself2,Lself2, dat.RmeshMF,dat.LmeshMF] = mesh2d_main_complete( ...
    dat.shp_id, dat.pt_2d, dat.dim1, dat.dim2, dat.Rpul,mur,dat.len, dat.frq);

data_proximity{k,1} = dat;

%% data 1x6 horizontal 6 coaxial cables + 3 sdc cable - 37mm distance
%  O O O O O O o o o

k = k+1;
dat.info = '1x6 horizontal 6 coaxial cables + 3 sdc cable'; 
dat.frq = frq;
dat.shp_id = [2100; 2100; 2100; 2100;2100; 2100;...
    2100; 2100;2100; 2100;2100; 2100; ...
    2100; 2100;2100; 2100; 2100;2100; 2100; 2100;2100;];

dat.pt_2d = [100 12.5; 100 12.5; 137.5 12.5; 137.5 12.5; 175 12.5; 175 12.5; ...
    212.5 12.5; 212.5 12.5; 250 12.5; 250 12.5; 287.5 12.5; 287.5 12.5; ...
    315 4; 313 4; 317 4; 340 4; 338 4; 342 4; 365 4; 363 4; 367 4;]*1e-3;
dat.dim1 = [12.45; 4.5; 12.45; 4.5; 12.45; 4.5;...
    12.45; 4.5; 12.45; 4.5; 12.45; 4.5;...
    4; 1.3; 1.3; 4; 1.3; 1.3; 4; 1.3; 1.3]*1e-3;
dat.dim2 = [11.65; 0; 11.65; 0; 11.65; 0; ...
    11.65; 0; 11.65; 0; 11.65; 0; ...
    3.4; 0; 0; 3.4; 0; 0; 3.4; 0; 0;]*1e-3;
% dat.Rpul = [1.18e-3;  1.05e-3; 1.18e-3;  1.05e-3; 1.18e-3;  1.05e-3; ...
%     1.18e-3;  1.05e-3; 1.18e-3;  1.05e-3; 1.18e-3;  1.05e-3; ...
%     1.2e-3; 3.2e-3; 3.2e-3];
dta.S = pi*(dat.dim1.^2-dat.dim2.^2);
dat.Rpul = 1./(sig_cu.*dta.S);
dat.len = 3.3;

Nc = size(dat.pt_2d,1);
mur = ones(Nc,Nf)*murtmp;

[Rmesh2,Lmesh2, Rself2,Lself2, dat.RmeshMF,dat.LmeshMF] = mesh2d_main_complete( ...
    dat.shp_id, dat.pt_2d, dat.dim1, dat.dim2, dat.Rpul,mur,dat.len, dat.frq);

data_proximity{k,1} = dat;



%% data 3x4 vetical 12 coaxial cables - 65mm distance
%  O  O  O  O
%  O  O  O  O
%  O  O  O  O
% -----
k = k+1;
dat.info = '3x4 vetical 12 coaxial cables - 65mm distance'; 
dat.frq = frq;
dat.shp_id = [2100;2100; 2100;2100; 2100;2100; 2100;2100; 2100;2100; 2100;2100;...
    2100;2100; 2100;2100; 2100;2100; 2100;2100; 2100;2100; 2100;2100;];
dat.pt_2d = [
    100 12.5; 100 12.5; 100 77.5; 100 77.5; 100 142.5;100 142.5; ...
    165 12.5; 165 12.5; 165 77.5; 165 77.5; 165 142.5;165 142.5; ...
    230 12.5; 230 12.5; 230 77.5; 230 77.5; 230 142.5;230 142.5; ...
    295 12.5; 295 12.5; 295 77.5; 295 77.5; 295 142.5;295 142.5; ]*1e-3;
dat.dim1 = [12.45; 4.5; 12.45; 4.5; 12.45; 4.5; 12.45; 4.5; 12.45; 4.5; 12.45; 4.5;...
    12.45; 4.5; 12.45; 4.5; 12.45; 4.5; 12.45; 4.5; 12.45; 4.5; 12.45; 4.5;]*1e-3;
dat.dim2 = [11.65; 0; 11.65; 0; 11.65; 0; 11.65; 0; 11.65; 0; 11.65; 0; ...
    11.65; 0; 11.65; 0; 11.65; 0; 11.65; 0; 11.65; 0; 11.65; 0; ]*1e-3;
    %5;  5;
% dat.Rpul = [1.18e-3;  1.05e-3; 1.18e-3;  1.05e-3; 1.18e-3;  1.05e-3; ...
%     1.18e-3;  1.05e-3; 1.18e-3;  1.05e-3; 1.18e-3;  1.05e-3; ]; 
%     % 0.226123e-3; 0.226123e-3];
dta.S = pi*(dat.dim1.^2-dat.dim2.^2);
dat.Rpul = 1./(sig_cu.*dta.S);
dat.len = 3.2;

Nc = size(dat.pt_2d,1);
mur = ones(Nc,Nf)*murtmp;

[Rmesh2,Lmesh2, Rself2,Lself2, dat.RmeshMF,dat.LmeshMF] = mesh2d_main_complete( ...
    dat.shp_id, dat.pt_2d, dat.dim1, dat.dim2, dat.Rpul,mur,dat.len, dat.frq);
data_proximity{k,1} = dat;


%% data 3x4 vetical 12 coaxial cables - 40mm distance - 3.2m
%  O O O O
%  O O O O
%  O O O O
% -----
k = k+1;
dat.info = '3x4 vetical 12 coaxial cables - 40mm distance - 3.2m'; 
dat.frq = frq;
dat.shp_id = [2100;2100; 2100;2100; 2100;2100; 2100;2100; 2100;2100; 2100;2100;...
    2100;2100; 2100;2100; 2100;2100; 2100;2100; 2100;2100; 2100;2100;];
dat.pt_2d = [
    100 12.5; 100 12.5; 100 52.5; 100 52.5; 100 92.5;100 92.5; ...
    140 12.5; 140 12.5; 140 52.5; 140 52.5; 140 92.5;140 92.5; ...
    180 12.5; 180 12.5; 180 52.5; 180 52.5; 180 92.5;180 92.5; ...
    220 12.5; 220 12.5; 220 52.5; 220 52.5; 220 92.5;220 92.5; ]*1e-3;
dat.dim1 = [12.45; 4.5; 12.45; 4.5; 12.45; 4.5; 12.45; 4.5; 12.45; 4.5; 12.45; 4.5;...
    12.45; 4.5; 12.45; 4.5; 12.45; 4.5; 12.45; 4.5; 12.45; 4.5; 12.45; 4.5;]*1e-3;
dat.dim2 = [11.65; 0; 11.65; 0; 11.65; 0; 11.65; 0; 11.65; 0; 11.65; 0; ...
    11.65; 0; 11.65; 0; 11.65; 0; 11.65; 0; 11.65; 0; 11.65; 0; ]*1e-3;
    %5;  5;
% dat.Rpul = [1.18e-3;  1.05e-3; 1.18e-3;  1.05e-3; 1.18e-3;  1.05e-3; ...
%     1.18e-3;  1.05e-3; 1.18e-3;  1.05e-3; 1.18e-3;  1.05e-3; ]; 
%     % 0.226123e-3; 0.226123e-3];
dta.S = pi*(dat.dim1.^2-dat.dim2.^2);
dat.Rpul = 1./(sig_cu.*dta.S);
dat.len = 3.2;

Nc = size(dat.pt_2d,1);
mur = ones(Nc,Nf)*murtmp;

[Rmesh2,Lmesh2, Rself2,Lself2, dat.RmeshMF,dat.LmeshMF] = mesh2d_main_complete( ...
    dat.shp_id, dat.pt_2d, dat.dim1, dat.dim2, dat.Rpul,mur,dat.len, dat.frq);
data_proximity{k,1} = dat;


%% data 4x4 vetical 16 coaxial cables - 40mm distance
%  O O O O
%  O O O O
%  O O O O
%  O O O O
% -----
k = k+1;
dat.info = '4x4 vetical 16 coaxial cables - 40mm distance'; 
dat.frq = frq;
dat.shp_id = [2100;2100; 2100;2100; 2100;2100; 2100;2100; 2100;2100; 2100;2100;...
    2100;2100; 2100;2100; 2100;2100; 2100;2100; 2100;2100; 2100;2100;];
dat.pt_2d = [
    100 12.5; 100 12.5; 100 52.5; 100 52.5; 100 92.5;100 92.5; 100 132.5;100 132.5;...
    140 12.5; 140 12.5; 140 52.5; 140 52.5; 140 92.5;140 92.5; 140 132.5;140 132.5;...
    180 12.5; 180 12.5; 180 52.5; 180 52.5; 180 92.5;180 92.5; 180 132.5;180 132.5;...
    220 12.5; 220 12.5; 220 52.5; 220 52.5; 220 92.5;220 92.5; 220 132.5;220 132.5;]*1e-3;
dat.dim1 = [12.45; 4.5; 12.45; 4.5; 12.45; 4.5; 12.45; 4.5; 12.45; 4.5;...
    12.45; 4.5; 12.45; 4.5; 12.45; 4.5; 12.45; 4.5; 12.45; 4.5; 12.45; 4.5;...
     12.45; 4.5; 12.45; 4.5; 12.45; 4.5; 12.45; 4.5; 12.45; 4.5]*1e-3;
dat.dim2 = [11.65; 0; 11.65; 0; 11.65; 0; 11.65; 0; 11.65; 0; 11.65; 0;11.65; 0; 11.65; 0; ...
    11.65; 0; 11.65; 0; 11.65; 0; 11.65; 0; 11.65; 0; 11.65; 0;  11.65; 0; 11.65; 0; ]*1e-3;
dta.S = pi*(dat.dim1.^2-dat.dim2.^2);
dat.Rpul = 1./(sig_cu.*dta.S);
dat.len = 3.2;

Nc = size(dat.pt_2d,1);
mur = ones(Nc,Nf)*murtmp;

[Rmesh2,Lmesh2, Rself2,Lself2, dat.RmeshMF,dat.LmeshMF] = mesh2d_main_complete( ...
    dat.shp_id, dat.pt_2d, dat.dim1, dat.dim2, dat.Rpul,mur,dat.len, dat.frq);
data_proximity{k,1} = dat;



%% data 3x5 vetical 15 coaxial cables - 40mm distance
%  O O O O O
%  O O O O O
%  O O O O O
% -----
k = k+1;
dat.info = '3x5 vetical 15 coaxial cables - 40mm distance'; 
dat.frq = frq;
dat.shp_id = [2100;2100; 2100;2100; 2100;2100; 2100;2100; 2100;2100; 2100;2100;  2100;2100;...
    2100;2100; 2100;2100; 2100;2100; 2100;2100; 2100;2100; 2100;2100; 2100;2100; 2100;2100;];
dat.pt_2d = [
    100 12.5; 100 12.5; 100 52.5; 100 52.5; 100 92.5;100 92.5; ...
    140 12.5; 140 12.5; 140 52.5; 140 52.5; 140 92.5;140 92.5; ...
    180 12.5; 180 12.5; 180 52.5; 180 52.5; 180 92.5;180 92.5; ...
    220 12.5; 220 12.5; 220 52.5; 220 52.5; 220 92.5;220 92.5; ...
    260 12.5; 260 12.5; 260 52.5; 260 52.5; 260 92.5;260 92.5;]*1e-3;
dat.dim1 = [12.45; 4.5; 12.45; 4.5; 12.45; 4.5; 12.45; 4.5; 12.45; 4.5; 12.45; 4.5;12.45; 4.5;...
    12.45; 4.5; 12.45; 4.5; 12.45; 4.5; 12.45; 4.5; 12.45; 4.5; 12.45; 4.5;12.45; 4.5; 12.45; 4.5; ]*1e-3;
dat.dim2 = [11.65; 0; 11.65; 0; 11.65; 0; 11.65; 0; 11.65; 0; 11.65; 0; 11.65; 0; ...
    11.65; 0; 11.65; 0; 11.65; 0; 11.65; 0; 11.65; 0; 11.65; 0; 11.65; 0; 11.65; 0;]*1e-3;
dta.S = pi*(dat.dim1.^2-dat.dim2.^2);
dat.Rpul = 1./(sig_cu.*dta.S);
dat.len = 2.2;

Nc = size(dat.pt_2d,1);
mur = ones(Nc,Nf)*murtmp;

[Rmesh2,Lmesh2, Rself2,Lself2, dat.RmeshMF,dat.LmeshMF] = mesh2d_main_complete( ...
    dat.shp_id, dat.pt_2d, dat.dim1, dat.dim2, dat.Rpul,mur,dat.len, dat.frq);
data_proximity{k,1} = dat;


%% data 4x5 vetical 15 coaxial cables - 40mm distance - 1.2m
%  O O O O O
%  O O O O O
%  O O O O O
%  O O O O O
% -----
k = k+1;
dat.info = '4x5 vetical 2 coaxial cables - 40mm distance - 1.2m'; 
dat.frq = frq;
dat.shp_id = [2100;2100; 2100;2100; 2100;2100; 2100;2100; 2100;2100; 2100;2100; ...
    2100;2100; 2100;2100; 2100;2100; 2100;2100; 2100;2100; 2100;2100; 2100;2100;  ...
    2100;2100; 2100;2100; 2100;2100; 2100;2100; 2100;2100; 2100;2100; 2100;2100;];
dat.pt_2d = [
    100 12.5; 100 12.5; 100 52.5; 100 52.5; 100 92.5;100 92.5; 100 132.5;100 132.5;...
    140 12.5; 140 12.5; 140 52.5; 140 52.5; 140 92.5;140 92.5; 140 132.5;140 132.5;...
    180 12.5; 180 12.5; 180 52.5; 180 52.5; 180 92.5;180 92.5; 180 132.5;180 132.5;...
    220 12.5; 220 12.5; 220 52.5; 220 52.5; 220 92.5;220 92.5; 220 132.5;220 132.5;...
    260 12.5; 260 12.5; 260 52.5; 260 52.5; 260 92.5;260 92.5; 260 132.5;260 132.5;]*1e-3;
dat.dim1 = [12.45; 4.5; 12.45; 4.5; 12.45; 4.5; 12.45; 4.5; 12.45; 4.5; ...
    12.45; 4.5;12.45; 4.5;12.45; 4.5; 12.45; 4.5; 12.45; 4.5; 12.45; 4.5; 12.45; 4.5;...
    12.45; 4.5; 12.45; 4.5; 12.45; 4.5; 12.45; 4.5; 12.45; 4.5; 12.45; 4.5;12.45; 4.5; 12.45; 4.5; ]*1e-3;
dat.dim2 = [11.65; 0; 11.65; 0; 11.65; 0; 11.65; 0; 11.65; 0; 11.65; 0; 11.65; 0; ...
    11.65; 0; 11.65; 0; 11.65; 0; 11.65; 0; 11.65; 0; 11.65; 0; 11.65; 0; 11.65; 0;...
    11.65; 0; 11.65; 0; 11.65; 0; 11.65; 0; 11.65; 0; ]*1e-3;
dta.S = pi*(dat.dim1.^2-dat.dim2.^2);
dat.Rpul = 1./(sig_cu.*dta.S);
dat.len = 1.2;

Nc = size(dat.pt_2d,1);
mur = ones(Nc,Nf)*murtmp;

[Rmesh2,Lmesh2, Rself2,Lself2, dat.RmeshMF,dat.LmeshMF] = mesh2d_main_complete( ...
    dat.shp_id, dat.pt_2d, dat.dim1, dat.dim2, dat.Rpul,mur,dat.len, dat.frq);
data_proximity{k,1} = dat;




%% data 4x5 vetical 15 coaxial cables - 40mm distance - 4.2m
%  O O O O O
%  O O O O O
%  O O O O O
%  O O O O O
% -----
k = k+1;
dat.info = '4x5 vetical 2 coaxial cables - 40mm distance - 4.2m'; 
dat.frq = frq;
dat.shp_id = [2100;2100; 2100;2100; 2100;2100; 2100;2100; 2100;2100; 2100;2100; ...
    2100;2100; 2100;2100; 2100;2100; 2100;2100; 2100;2100; 2100;2100; 2100;2100;  ...
    2100;2100; 2100;2100; 2100;2100; 2100;2100; 2100;2100; 2100;2100; 2100;2100;];
dat.pt_2d = [
    100 12.5; 100 12.5; 100 52.5; 100 52.5; 100 92.5;100 92.5; 100 132.5;100 132.5;...
    140 12.5; 140 12.5; 140 52.5; 140 52.5; 140 92.5;140 92.5; 140 132.5;140 132.5;...
    180 12.5; 180 12.5; 180 52.5; 180 52.5; 180 92.5;180 92.5; 180 132.5;180 132.5;...
    220 12.5; 220 12.5; 220 52.5; 220 52.5; 220 92.5;220 92.5; 220 132.5;220 132.5;...
    260 12.5; 260 12.5; 260 52.5; 260 52.5; 260 92.5;260 92.5; 260 132.5;260 132.5;]*1e-3;
dat.dim1 = [12.45; 4.5; 12.45; 4.5; 12.45; 4.5; 12.45; 4.5; 12.45; 4.5; ...
    12.45; 4.5;12.45; 4.5;12.45; 4.5; 12.45; 4.5; 12.45; 4.5; 12.45; 4.5; 12.45; 4.5;...
    12.45; 4.5; 12.45; 4.5; 12.45; 4.5; 12.45; 4.5; 12.45; 4.5; 12.45; 4.5;12.45; 4.5; 12.45; 4.5; ]*1e-3;
dat.dim2 = [11.65; 0; 11.65; 0; 11.65; 0; 11.65; 0; 11.65; 0; 11.65; 0; 11.65; 0; ...
    11.65; 0; 11.65; 0; 11.65; 0; 11.65; 0; 11.65; 0; 11.65; 0; 11.65; 0; 11.65; 0;...
    11.65; 0; 11.65; 0; 11.65; 0; 11.65; 0; 11.65; 0; ]*1e-3;
dta.S = pi*(dat.dim1.^2-dat.dim2.^2);
dat.Rpul = 1./(sig_cu.*dta.S);
dat.len = 4.2;

Nc = size(dat.pt_2d,1);
mur = ones(Nc,Nf)*murtmp;

[Rmesh2,Lmesh2, Rself2,Lself2, dat.RmeshMF,dat.LmeshMF] = mesh2d_main_complete( ...
    dat.shp_id, dat.pt_2d, dat.dim1, dat.dim2, dat.Rpul,mur,dat.len, dat.frq);
data_proximity{k,1} = dat;



%% data 3x4 vetical 12 coaxial cables + 3x2 vertical 4 sdc cables - 40mm distance - 2.2m
%  O O O O oo
%  O O O O oo
%  O O O O oo
% -----
k = k+1;
dat.info = '3x4 vetical 12 coaxial cables + 3x2 vertical 4 sdc cables - 40mm distance - 2.2'; 
dat.frq = frq;
dat.shp_id = [2100;2100; 2100;2100; 2100;2100; 2100;2100; 2100;2100; 2100;2100;...
    2100;2100; 2100;2100; 2100;2100; 2100;2100; 2100;2100; 2100;2100; ...
    2100;2100;2100; 2100;2100;2100; 2100;2100;2100; ...
    2100;2100;2100; 2100;2100;2100; 2100;2100;2100; ];
dat.pt_2d = [
    100 12.5; 100 12.5; 100 52.5; 100 52.5; 100 92.5;100 92.5; ...
    140 12.5; 140 12.5; 140 52.5; 140 52.5; 140 92.5;140 92.5; ...
    180 12.5; 180 12.5; 180 52.5; 180 52.5; 180 92.5;180 92.5; ...
    220 12.5; 220 12.5; 220 52.5; 220 52.5; 220 92.5;220 92.5; ...
    245 4; 243 4; 247 4; 245 24; 243 24;247 24; 245 44;243 44;247 44;...
    265 4; 263 4; 267 4; 265 24; 263 24;267 24; 265 44;263 44;267 44;...
    ]*1e-3;
dat.dim1 = [12.45; 4.5; 12.45; 4.5; 12.45; 4.5; 12.45; 4.5; 12.45; 4.5; 12.45; 4.5;...
    12.45; 4.5; 12.45; 4.5; 12.45; 4.5; 12.45; 4.5; 12.45; 4.5; 12.45; 4.5; ...
    4; 1.3; 1.3; 4; 1.3; 1.3; 4; 1.3; 1.3; 4; 1.3; 1.3; 4; 1.3; 1.3; 4; 1.3; 1.3;]*1e-3;
dat.dim2 = [11.65; 0; 11.65; 0; 11.65; 0; 11.65; 0; 11.65; 0; 11.65; 0; ...
    11.65; 0; 11.65; 0; 11.65; 0; 11.65; 0; 11.65; 0; 11.65; 0; ...
    3.4; 0; 0; 3.4; 0; 0; 3.4; 0; 0; 3.4; 0; 0; 3.4; 0; 0; 3.4; 0; 0;]*1e-3;
dta.S = pi*(dat.dim1.^2-dat.dim2.^2);
dat.Rpul = 1./(sig_cu.*dta.S);
dat.len = 2.2;

Nc = size(dat.pt_2d,1);
mur = ones(Nc,Nf)*murtmp;

[Rmesh2,Lmesh2, Rself2,Lself2, dat.RmeshMF,dat.LmeshMF] = mesh2d_main_complete( ...
    dat.shp_id, dat.pt_2d, dat.dim1, dat.dim2, dat.Rpul,mur,dat.len, dat.frq);
data_proximity{k,1} = dat;


%% data 3x4 vetical 12 coaxial cables + 3x2 vertical 4 sdc cables - 40mm distance - 5m
%  O O O O oo
%  O O O O oo
%  O O O O oo
% -----
k = k+1;
dat.info = '3x4 vetical 12 coaxial cables + 3x2 vertical 4 sdc cables - 40mm distance - 5m'; 
dat.frq = frq;
dat.shp_id = [2100;2100; 2100;2100; 2100;2100; 2100;2100; 2100;2100; 2100;2100;...
    2100;2100; 2100;2100; 2100;2100; 2100;2100; 2100;2100; 2100;2100; ...
    2100;2100;2100; 2100;2100;2100; 2100;2100;2100; ...
    2100;2100;2100; 2100;2100;2100; 2100;2100;2100; ];
dat.pt_2d = [
    100 12.5; 100 12.5; 100 52.5; 100 52.5; 100 92.5;100 92.5; ...
    140 12.5; 140 12.5; 140 52.5; 140 52.5; 140 92.5;140 92.5; ...
    180 12.5; 180 12.5; 180 52.5; 180 52.5; 180 92.5;180 92.5; ...
    220 12.5; 220 12.5; 220 52.5; 220 52.5; 220 92.5;220 92.5; ...
    245 4; 243 4; 247 4; 245 24; 243 24;247 24; 245 44;243 44;247 44;...
    265 4; 263 4; 267 4; 265 24; 263 24;267 24; 265 44;263 44;267 44;...
    ]*1e-3;
dat.dim1 = [12.45; 4.5; 12.45; 4.5; 12.45; 4.5; 12.45; 4.5; 12.45; 4.5; 12.45; 4.5;...
    12.45; 4.5; 12.45; 4.5; 12.45; 4.5; 12.45; 4.5; 12.45; 4.5; 12.45; 4.5; ...
    4; 1.3; 1.3; 4; 1.3; 1.3; 4; 1.3; 1.3; 4; 1.3; 1.3; 4; 1.3; 1.3; 4; 1.3; 1.3;]*1e-3;
dat.dim2 = [11.65; 0; 11.65; 0; 11.65; 0; 11.65; 0; 11.65; 0; 11.65; 0; ...
    11.65; 0; 11.65; 0; 11.65; 0; 11.65; 0; 11.65; 0; 11.65; 0; ...
    3.4; 0; 0; 3.4; 0; 0; 3.4; 0; 0; 3.4; 0; 0; 3.4; 0; 0; 3.4; 0; 0;]*1e-3;
    %5;  5;
% dat.Rpul = [1.18e-3;  1.05e-3; 1.18e-3;  1.05e-3; 1.18e-3;  1.05e-3; ...
%     1.18e-3;  1.05e-3; 1.18e-3;  1.05e-3; 1.18e-3;  1.05e-3; ]; 
%     % 0.226123e-3; 0.226123e-3];
dta.S = pi*(dat.dim1.^2-dat.dim2.^2);
dat.Rpul = 1./(sig_cu.*dta.S);
dat.len = 5;

Nc = size(dat.pt_2d,1);
mur = ones(Nc,Nf)*murtmp;

[Rmesh2,Lmesh2, Rself2,Lself2, dat.RmeshMF,dat.LmeshMF] = mesh2d_main_complete( ...
    dat.shp_id, dat.pt_2d, dat.dim1, dat.dim2, dat.Rpul,mur,dat.len, dat.frq);
data_proximity{k,1} = dat;



%% data 1x6 horizontal 6 coaxial cables + 3 sdc cable - 3.33m
%   O O O O O   o o o
%  O O O O O O o o o o

k = k+1;
dat.info = '1x6 horizontal coaxial + 1x5 horizontal 6 coaxial + 7 sdc cables'; 

dat.frq = frq;
dat.shp_id = [
    2100; 2100; 2100; 2100;2100; 2100; 2100; 2100;2100; 2100;2100; 2100; ... %cox down 1-6
    2100; 2100; 2100; 2100;2100; 2100;2100; 2100;2100; 2100; ... %cox up 1-5
    2100; 2100;2100; 2100; 2100;2100; 2100; 2100;2100; 2100; 2100;2100; ...  %sdc down 1-4
    2100; 2100;2100; 2100; 2100;2100; ]; %sdc up 1-2

dat.pt_2d = [
    100 12.5; 100 12.5; 137.5 12.5; 137.5 12.5; 175 12.5; 175 12.5; ... %cox down 1-3
    212.5 12.5; 212.5 12.5; 250 12.5; 250 12.5; 287.5 12.5; 287.5 12.5; ... %cox down 4-6
    118.75 45; 118.75 45; 156.25 45; 156.25 45; 193.75 45; 193.75 45; 231.25 45; 231.25 45;  268.75 45; 268.75 45;  %cox up 1-5
    312 4; 310 4; 314 4; 330 4; 328 4; 332 4; 348 4; 346 4; 350 4; 366 4; 364 4; 368 4; ... %sdc down 1-4
    321 17.6; 319 17.6; 323 17.6; 339 17.6; 337 17.6; 341 17.6; ... %sdc up 1-2
    ]*1e-3; 
dat.dim1 = [12.45; 4.5; 12.45; 4.5; 12.45; 4.5; 12.45; 4.5; 12.45; 4.5; 12.45; 4.5; ...
    12.45; 4.5; 12.45; 4.5; 12.45; 4.5; 12.45; 4.5; 12.45; 4.5;...
    4; 1.3; 1.3; 4; 1.3; 1.3; 4; 1.3; 1.3; 4; 1.3; 1.3;  4; 1.3; 1.3; 4; 1.3; 1.3;]*1e-3;
dat.dim2 = [11.65; 0; 11.65; 0; 11.65; 0; 11.65; 0; 11.65; 0; 11.65; 0; ...
    11.65; 0; 11.65; 0; 11.65; 0; 11.65; 0; 11.65; 0; ...
    3.4; 0; 0; 3.4; 0; 0; 3.4; 0; 0;  3.4; 0; 0; 3.4; 0; 0;  3.4; 0; 0;]*1e-3;
% dat.Rpul = [1.18e-3;  1.05e-3; 1.18e-3;  1.05e-3; 1.18e-3;  1.05e-3; ...
%     1.18e-3;  1.05e-3; 1.18e-3;  1.05e-3; 1.18e-3;  1.05e-3; ...
%     1.2e-3; 3.2e-3; 3.2e-3];
dta.S = pi*(dat.dim1.^2-dat.dim2.^2);
dat.Rpul = 1./(sig_cu.*dta.S);
dat.len = 3.33;

Nc = size(dat.pt_2d,1);
mur = ones(Nc,Nf)*murtmp;

[Rmesh2,Lmesh2, Rself2,Lself2, dat.RmeshMF,dat.LmeshMF] = mesh2d_main_complete( ...
    dat.shp_id, dat.pt_2d, dat.dim1, dat.dim2, dat.Rpul,mur,dat.len, dat.frq);

data_proximity{k,1} = dat;


%% data 1x6 horizontal 6 coaxial cables + 3 sdc cable - 37mm distance - 5.4m
%   O O O O O   o o o
%  O O O O O O o o o o

k = k+1;
dat.info = '1x6 horizontal coaxial + 1x5 horizontal 6 coaxial + 7 sdc cables - 5.4m'; 

dat.frq = frq;
dat.shp_id = [
    2100; 2100; 2100; 2100;2100; 2100; 2100; 2100;2100; 2100;2100; 2100; ... %cox down 1-6
    2100; 2100; 2100; 2100;2100; 2100;2100; 2100;2100; 2100; ... %cox up 1-5
    2100; 2100;2100; 2100; 2100;2100; 2100; 2100;2100; 2100; 2100;2100; ...  %sdc down 1-4
    2100; 2100;2100; 2100; 2100;2100; ]; %sdc up 1-2

dat.pt_2d = [
    100 12.5; 100 12.5; 137.5 12.5; 137.5 12.5; 175 12.5; 175 12.5; ... %cox down 1-3
    212.5 12.5; 212.5 12.5; 250 12.5; 250 12.5; 287.5 12.5; 287.5 12.5; ... %cox down 4-6
    118.75 45; 118.75 45; 156.25 45; 156.25 45; 193.75 45; 193.75 45; 231.25 45; 231.25 45;  268.75 45; 268.75 45;  %cox up 1-5
    312 4; 310 4; 314 4; 330 4; 328 4; 332 4; 348 4; 346 4; 350 4; 366 4; 364 4; 368 4; ... %sdc down 1-4
    321 17.6; 319 17.6; 323 17.6; 339 17.6; 337 17.6; 341 17.6; ... %sdc up 1-2
    ]*1e-3; 
dat.dim1 = [12.45; 4.5; 12.45; 4.5; 12.45; 4.5; 12.45; 4.5; 12.45; 4.5; 12.45; 4.5; ...
    12.45; 4.5; 12.45; 4.5; 12.45; 4.5; 12.45; 4.5; 12.45; 4.5;...
    4; 1.3; 1.3; 4; 1.3; 1.3; 4; 1.3; 1.3; 4; 1.3; 1.3;  4; 1.3; 1.3; 4; 1.3; 1.3;]*1e-3;
dat.dim2 = [11.65; 0; 11.65; 0; 11.65; 0; 11.65; 0; 11.65; 0; 11.65; 0; ...
    11.65; 0; 11.65; 0; 11.65; 0; 11.65; 0; 11.65; 0; ...
    3.4; 0; 0; 3.4; 0; 0; 3.4; 0; 0;  3.4; 0; 0; 3.4; 0; 0;  3.4; 0; 0;]*1e-3;
% dat.Rpul = [1.18e-3;  1.05e-3; 1.18e-3;  1.05e-3; 1.18e-3;  1.05e-3; ...
%     1.18e-3;  1.05e-3; 1.18e-3;  1.05e-3; 1.18e-3;  1.05e-3; ...
%     1.2e-3; 3.2e-3; 3.2e-3];
dta.S = pi*(dat.dim1.^2-dat.dim2.^2);
dat.Rpul = 1./(sig_cu.*dta.S);
dat.len = 5.4;

Nc = size(dat.pt_2d,1);
mur = ones(Nc,Nf)*murtmp;

[Rmesh2,Lmesh2, Rself2,Lself2, dat.RmeshMF,dat.LmeshMF] = mesh2d_main_complete( ...
    dat.shp_id, dat.pt_2d, dat.dim1, dat.dim2, dat.Rpul,mur,dat.len, dat.frq);

data_proximity{k,1} = dat;



%% Interpolate the data to more frequency points




%% save the data
save(sname, 'data_proximity');





