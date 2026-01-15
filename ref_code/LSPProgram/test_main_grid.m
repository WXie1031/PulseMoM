clear

%load('case_2_3_sdc_new_grid.mat');
% load('4corner_tower_1sdc_grid.mat');
% load('pv_plant_grid.mat');
% load('TubeTower40m_grid.mat');
load('WT_paper_1_grid.mat');
% load('C:\Users\cai\Desktop\Zhang\version2_grid.mat')

soil_sig = 0.01;
soil_epr = 10;
frq_cut = 2e6;
Nfrq = 100;
dl = 5;
gnd_off = 0.05;

flag_grid = 1;

Nfit = 2;

% bran_out_grid=[];
% nod_out_grid=[];

Nc = size(pt_start_grid, 1);
grp_id_grid = zeros(Nc, 1);
grp_type_grid = zeros(Nc, 1);
pt_2d_grid = zeros(Nc, 2);

% fpath='D:\ProjectFiles\4corner_tower_1sdc_1_induced\';
% fpath_grid = 'C:\Users\cai\Desktop\Zhang\';
fpath_grid = [];

fname_grid = 'WT_Rg100';

epr_grid = mur_grid./mur_grid;

% fname_grid = 'WT_paper_1';
main_prog_grid(pt_start_grid,pt_end_grid,re_grid, ...
    shape_grid,dim1_grid,dim2_grid, R_pul_grid,sig_grid,mur_grid,epr_grid, ...
    grp_id_grid, grp_type_grid, pt_2d_grid,...
    bran_name_grid,nod_start_grid,nod_end_grid,bran_out_grid,nod_out_grid, ...
    soil_sig,soil_epr, frq_cut,Nfrq, Nfit,dl, ...
    fpath_grid,fname_grid,flag_grid)



