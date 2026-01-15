% clear
% 
% load('HWBigTW_MOR.mat');
% load('tower_1sdc_simple.mat');
%load('case_2_3_sdc_no_mag.mat');
% load('case_2_3_sdc_new.mat');
% load('pv_plant.mat');
%load('hw_2x3_cable');
%  load('TubeTower40m.mat');
% load('WuHan_45mmNew');

% load('version2.mat')
% load('WT_paper_SingleCable.mat');
% load('WT_paper_4.mat');
% load('WT_APL.mat');
% load('WT_APL_New.mat')
% load('WT_Test.mat')
% load('V1_500.mat');
% load('WT_APL_6COX.mat')

% fname = 'WT_APL';

sig_soil = 3.5e-3;

sig_soil = 0.01;
soil_epr = 10;
frq_cut = 2e6;

dl = 10;
gnd_off = 0.1;

flag_sol = 2;
% flag_sol 1  fix frequency
% flag_sol 2  multi frequencies + vf
% flag_sol 3  node reduced + multi frequencies +vf

flag_mesh = 3; % 2 - calculate all group  3 - calculate the group using NN
flag_gnd = 1;
flag_p = 1;
flag_mor = 0;
% fpath='';
% fname = 'WT_APL_NoLm';

Nc = size(pt_start,1);
%grp_id=zeros(Nc,1);
% grp_type=grp_id;
% grp_id
% pt_2d=zeros(Nc,2);
% fname = 'case_2_3_sdc_new';
%fname = 'hw_2x3_cable';
% fname = 'case_2_3_sdc_new';
% fname = 'pv_plant';
% fname = 'TubeTower40m';
% fname = 'WT_paper_1';
%fname = 'case_2_3_sdc_no_mag';
% bran_out=[];
%nod_out=nod_output;
flag_sr = 0;

fname = 'tmp_not_used';

if ~exist('epr') 
    Nc = size(pt_start,1);
    epr = zeros(Nc,1);
end

if frq_cut>1e6
    Nfit = max(Nfit,5);
end
Nfit = 2;
timeSta = tic;
main_prog_air(pt_start,pt_end,re, shape,dim1,dim2, R_pul,sig,mur,epr, ...
    grp_id, grp_type, pt_2d, ...
    bran_name,nod_start,nod_end,nod_sr,nod_gnd,bran_out,nod_out, ...
    sig_soil,soil_epr, gnd_off, frq_cut, Nfit,dl, fpath,fname, ...
    flag_p,flag_sol,flag_mesh,flag_gnd ,flag_mor, flag_sr)

ttotal = toc(timeSta)/60;
disp(['Total Simulation Time: ',num2str(ttotal),' min'])


