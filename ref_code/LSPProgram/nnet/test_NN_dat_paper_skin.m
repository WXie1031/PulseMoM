clc

frq = [1 50 100 200 500 1e3 2e3 5e3 10e3 20e3 50e3 100e3  200e3 500e3 1e6 2e6];
% frq_interp = linspace(1,1e6, 50);
Nf = size(frq,2);
murtmp = 1;
sig_cu = 5.8e7;

sname = 'dat_nn_paper_skin';

k = 0;
%% 1. data 1x3 horizontal 3 cox cables + 1x2 horizontal 2 coaxial cables
%   O O 
%  O O O 

k = k+1;
disp(['case ',num2str(k)])

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
dat.len = 5;

Nc = size(dat.pt_2d,1);
mur = ones(Nc,Nf)*murtmp;
dat.sig = 1./(dat.Rpul.*dta.S);

shp_id = 2100*ones(Nc,1);
pt_3d_start = [dat.pt_2d zeros(Nc,1)];
pt_3d_end = [dat.pt_2d dat.len*ones(Nc,1)];
dv = ones(Nc,1)*[0 0 1];
len = dat.len*ones(Nc,1);

[Rmtx, Lmtx] = para_main_fila(pt_3d_start, pt_3d_end, dv, dat.dim1, len);
dat.Lmtx = Lmtx;

[Rself, Lself] = para_self_multi_frq(shp_id, dat.dim1, dat.dim2, ...
    len, dat.Rpul, dat.sig, mur, frq);
dat.Rself = Rself;
dat.Lself = Lself;

dat_skin{k,1} = dat;


%% 2. data 1x4 horizontal 4 sdc cables + 1x3 horizontal 3 sdc cables
%   o o o
%  o o o o

k = k+1;
disp(['case ',num2str(k)])

dat.info = '1x4 horizontal sdc cables + 1x3 horizontal 3 sdc cables'; 

dat.frq = frq;
dat.shp_id = [2100;2100;2100; 2100;2100;2100; 2100;2100;2100; 2100;2100;2100;...  %sdc down 1-4
    2100;2100;2100; 2100;2100;2100; 2100;2100;2100;]; %sdc up 1-3

dat.pt_2d = [
    312 4; 310 4; 314 4; 330 4; 328 4; 332 4; 348 4; 346 4; 350 4; 366 4; 364 4; 368 4; ... %sdc down 1-3
    321 17.6; 319 17.6; 323 17.6; 339 17.6; 337 17.6; 341 17.6; 357 17.6; 355 17.6; 359 17.6; ... %sdc up 1-2
    ]*1e-3; 
dat.dim1 = [ 4;1.3;1.3; 4;1.3;1.3; 4;1.3;1.3;  4;1.3;1.3; 4;1.3;1.3; 4;1.3;1.3; 4;1.3;1.3;]*1e-3;
dat.dim2 = [ 3.4;0;0; 3.4;0;0;  3.4;0;0; 3.4;0;0;  3.4;0;0; 3.4;0;0; 3.4;0;0;]*1e-3;
dta.S = pi*(dat.dim1.^2-dat.dim2.^2);
dat.Rpul = 1./(sig_cu.*dta.S);
dat.len = 3.65;

Nc = size(dat.pt_2d,1);
mur = ones(Nc,Nf)*murtmp;
dat.sig = 1./(dat.Rpul.*dta.S);


shp_id = 2100*ones(Nc,1);
pt_3d_start = [dat.pt_2d zeros(Nc,1)];
pt_3d_end = [dat.pt_2d dat.len*ones(Nc,1)];
dv = ones(Nc,1)*[0 0 1];
len = dat.len*ones(Nc,1);

[Rmtx, Lmtx] = para_main_fila(pt_3d_start, pt_3d_end, dv, dat.dim1, len);
dat.Lmtx = Lmtx;

[Rself, Lself] = para_self_multi_frq(shp_id, dat.dim1, dat.dim2, ...
    len, dat.Rpul, dat.sig, mur, frq);
dat.Rself = Rself;
dat.Lself = Lself;

dat_skin{k,1} = dat;



%% 3. data 3x2 vetical 6 coaxial cables + 3 sdc cables - 37mm distance
%  O O o
%  O O o
%  O O o
% -------

k = k+1;
disp(['case ',num2str(k)])

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
dat.sig = 1./(dat.Rpul.*dta.S);


shp_id = 2100*ones(Nc,1);
pt_3d_start = [dat.pt_2d zeros(Nc,1)];
pt_3d_end = [dat.pt_2d dat.len*ones(Nc,1)];
dv = ones(Nc,1)*[0 0 1];
len = dat.len*ones(Nc,1);

[Rmtx, Lmtx] = para_main_fila(pt_3d_start, pt_3d_end, dv, dat.dim1, len);
dat.Lmtx = Lmtx;

[Rself, Lself] = para_self_multi_frq(shp_id, dat.dim1, dat.dim2, ...
    len, dat.Rpul, dat.sig, mur, frq);
dat.Rself = Rself;
dat.Lself = Lself;

dat_skin{k,1} = dat;



%% 4. data 1x6 horizontal 6 coaxial cables + 3 sdc cable - 37mm distance
%  O O O O O O o o o

k = k+1;
disp(['case ',num2str(k)])

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
dat.sig = 1./(dat.Rpul.*dta.S);


shp_id = 2100*ones(Nc,1);
pt_3d_start = [dat.pt_2d zeros(Nc,1)];
pt_3d_end = [dat.pt_2d dat.len*ones(Nc,1)];
dv = ones(Nc,1)*[0 0 1];
len = dat.len*ones(Nc,1);

[Rmtx, Lmtx] = para_main_fila(pt_3d_start, pt_3d_end, dv, dat.dim1, len);
dat.Lmtx = Lmtx;

[Rself, Lself] = para_self_multi_frq(shp_id, dat.dim1, dat.dim2, ...
    len, dat.Rpul, dat.sig, mur, frq);
dat.Rself = Rself;
dat.Lself = Lself;

dat_skin{k,1} = dat;


%% 5. data 4x5 vetical 15 coaxial cables - 40mm distance - 4.2m
%  O O O O O
%  O O O O O
%  O O O O O
%  O O O O O
% -----
k = k+1;
disp(['case ',num2str(k)])

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
dat.sig = 1./(dat.Rpul.*dta.S);


shp_id = 2100*ones(Nc,1);
pt_3d_start = [dat.pt_2d zeros(Nc,1)];
pt_3d_end = [dat.pt_2d dat.len*ones(Nc,1)];
dv = ones(Nc,1)*[0 0 1];
len = dat.len*ones(Nc,1);

[Rmtx, Lmtx] = para_main_fila(pt_3d_start, pt_3d_end, dv, dat.dim1, len);
dat.Lmtx = Lmtx;

[Rself, Lself] = para_self_multi_frq(shp_id, dat.dim1, dat.dim2, ...
    len, dat.Rpul, dat.sig, mur, frq);
dat.Rself = Rself;
dat.Lself = Lself;

dat_skin{k,1} = dat;


%% 6. data 3x3 9 sdc cables - 37mm distance
%  o o o o
%  o o o o
%  o o o o
% -----
k = k+1;
disp(['case ',num2str(k)])

dat.info = '3x4 12 sdc cables'; 
dat.frq = frq;
dat.shp_id = [2100;2100;2100; 2100;2100;2100; 2100;2100;2100; 2100;2100;2100;...
    2100;2100;2100; 2100;2100;2100; 2100;2100;2100; 2100;2100;2100; 2100;2100;2100;...
     2100;2100;2100; 2100;2100;2100; 2100;2100;2100;];
dat.pt_2d = [100 4; 98 4; 102 4; 100 19; 98 19;102 19; 100 34; 98 34;102 34;...
    120 4; 118 4; 122 4; 120 19; 118 19;122 19; 120 34; 118 34;122 34; ...
    140 4; 138 4; 142 4; 140 19; 138 19;142 19; 140 34; 138 34;142 34;...
    160 4; 158 4; 162 4; 160 19; 158 19;162 19; 160 34; 158 34;162 34;]*1e-3;
dat.dim1 = [4; 1.3; 1.3; 4; 1.3; 1.3;4; 1.3; 1.3;4; 1.3; 1.3; 4; 1.3; 1.3;...
    4; 1.3; 1.3; 4; 1.3; 1.3; 4; 1.3; 1.3; 4; 1.3; 1.3; 4; 1.3; 1.3; 4; 1.3; 1.3; 4; 1.3; 1.3;]*1e-3;

dat.dim2 = [3.4; 0; 0;3.4; 0; 0; 3.4; 0; 0;3.4; 0; 0;3.4; 0; 0; 3.4; 0; 0; ...
    3.4; 0; 0; 3.4; 0; 0;3.4; 0; 0; 3.4; 0; 0; 3.4; 0; 0; 3.4; 0; 0;]*1e-3;
dta.S = pi*(dat.dim1.^2-dat.dim2.^2);
dat.Rpul = 1./(sig_cu.*dta.S);
dat.len = 1.8;

Nc = size(dat.pt_2d,1);
mur = ones(Nc,Nf)*murtmp;
dat.sig = 1./(dat.Rpul.*dta.S);


shp_id = 2100*ones(Nc,1);
pt_3d_start = [dat.pt_2d zeros(Nc,1)];
pt_3d_end = [dat.pt_2d dat.len*ones(Nc,1)];
dv = ones(Nc,1)*[0 0 1];
len = dat.len*ones(Nc,1);

[Rmtx, Lmtx] = para_main_fila(pt_3d_start, pt_3d_end, dv, dat.dim1, len);
dat.Lmtx = Lmtx;

[Rself, Lself] = para_self_multi_frq(shp_id, dat.dim1, dat.dim2, ...
    len, dat.Rpul, dat.sig, mur, frq);
dat.Rself = Rself;
dat.Lself = Lself;

dat_skin{k,1} = dat;


%% 7. data 3x4 vetical 12 coaxial cables + 3x2 vertical 4 sdc cables - 40mm distance - 5m
%  O O O O oo
%  O O O O oo
%  O O O O oo
% -----
k = k+1;
disp(['case ',num2str(k)])

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
dat.sig = 1./(dat.Rpul.*dta.S);


shp_id = 2100*ones(Nc,1);
pt_3d_start = [dat.pt_2d zeros(Nc,1)];
pt_3d_end = [dat.pt_2d dat.len*ones(Nc,1)];
dv = ones(Nc,1)*[0 0 1];
len = dat.len*ones(Nc,1);

[Rmtx, Lmtx] = para_main_fila(pt_3d_start, pt_3d_end, dv, dat.dim1, len);
dat.Lmtx = Lmtx;

[Rself, Lself] = para_self_multi_frq(shp_id, dat.dim1, dat.dim2, ...
    len, dat.Rpul, dat.sig, mur, frq);
dat.Rself = Rself;
dat.Lself = Lself;

dat_skin{k,1} = dat;


%% 8. data 1x6 horizontal 6 coaxial cables + 3 sdc cable - 37mm distance - 5.4m
%   O O O O O   o o o
%  O O O O O O o o o o

k = k+1;
disp(['case ',num2str(k)])

dat.info = '1x6 horizontal coaxial + 1x5 horizontal 6 coaxial + 7 sdc cables - 5.4m'; 

dat.frq = frq;
dat.shp_id = [
    2100; 2100; 2100; 2100;2100; 2100; 2100; 2100;2100; 2100;2100; 2100; ... %cox down 1-6
    2100; 2100; 2100; 2100;2100; 2100;2100; 2100;2100; 2100; ... %cox up 1-5
    2100; 2100;2100; 2100; 2100;2100; 2100; 2100;2100; 2100; 2100;2100; ...  %sdc down 1-4
    2100; 2100;2100; 2100; 2100;2100; 2100; 2100;2100;]; %sdc up 1-3

dat.pt_2d = [
    100 12.5; 100 12.5; 137.5 12.5; 137.5 12.5; 175 12.5; 175 12.5; ... %cox down 1-3
    212.5 12.5; 212.5 12.5; 250 12.5; 250 12.5; 287.5 12.5; 287.5 12.5; ... %cox down 4-6
    118.75 45; 118.75 45; 156.25 45; 156.25 45; 193.75 45; 193.75 45; 231.25 45; 231.25 45;  268.75 45; 268.75 45;  %cox up 1-5
    312 4; 310 4; 314 4; 330 4; 328 4; 332 4; 348 4; 346 4; 350 4; 366 4; 364 4; 368 4; ... %sdc down 1-4
    321 17.6; 319 17.6; 323 17.6; 339 17.6; 337 17.6; 341 17.6; 357 17.6; 355 17.6; 359 17.6; ... %sdc up 1-2
    ]*1e-3; 
dat.dim1 = [12.45; 4.5; 12.45; 4.5; 12.45; 4.5; 12.45; 4.5; 12.45; 4.5; 12.45; 4.5; ...
    12.45; 4.5; 12.45; 4.5; 12.45; 4.5; 12.45; 4.5; 12.45; 4.5;...
    4;1.3;1.3; 4;1.3;1.3; 4;1.3;1.3; 4;1.3;1.3;  4;1.3;1.3; 4;1.3;1.3; 4;1.3;1.3;]*1e-3;
dat.dim2 = [11.65; 0; 11.65; 0; 11.65; 0; 11.65; 0; 11.65; 0; 11.65; 0; ...
    11.65; 0; 11.65; 0; 11.65; 0; 11.65; 0; 11.65; 0; ...
    3.4;0;0; 3.4;0;0; 3.4;0;0;  3.4;0;0; 3.4;0;0;  3.4;0;0; 3.4;0;0;]*1e-3;
% dat.Rpul = [1.18e-3;  1.05e-3; 1.18e-3;  1.05e-3; 1.18e-3;  1.05e-3; ...
%     1.18e-3;  1.05e-3; 1.18e-3;  1.05e-3; 1.18e-3;  1.05e-3; ...
%     1.2e-3; 3.2e-3; 3.2e-3];
dta.S = pi*(dat.dim1.^2-dat.dim2.^2);
dat.Rpul = 1./(sig_cu.*dta.S);
dat.len = 5.0;

Nc = size(dat.pt_2d,1);
mur = ones(Nc,Nf)*murtmp;
dat.sig = 1./(dat.Rpul.*dta.S);


shp_id = 2100*ones(Nc,1);
pt_3d_start = [dat.pt_2d zeros(Nc,1)];
pt_3d_end = [dat.pt_2d dat.len*ones(Nc,1)];
dv = ones(Nc,1)*[0 0 1];
len = dat.len*ones(Nc,1);

[Rmtx, Lmtx] = para_main_fila(pt_3d_start, pt_3d_end, dv, dat.dim1, len);
dat.Lmtx = Lmtx;

[Rself, Lself] = para_self_multi_frq(shp_id, dat.dim1, dat.dim2, ...
    len, dat.Rpul, dat.sig, mur, frq);
dat.Rself = Rself;
dat.Lself = Lself;

dat_skin{k,1} = dat;



%% save the data

save(sname, 'dat_skin');


