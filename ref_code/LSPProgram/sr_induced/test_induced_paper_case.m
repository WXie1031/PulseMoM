clear
clc

%% 1. geometry
pt_start = [
    0 0   7.5;
    0 341 7.5;
    0 0   5.7;
    0 341 5.7;
    0 0   0;
    0 341 0;
    0 682 0;];

pt_end = [
    0 341 7.5;
    0 682 7.5;
    0 341 5.7;
    0 682 5.7;
    0 0   5.7;
    0 341 5.7;
    0 682 5.7;];

Nc = size(pt_start,1);
% figure(56)
% hold on
% for ik=1:Nc
%     plot3( [pt_start(ik,1); pt_end(ik,1)], [pt_start(ik,2); pt_end(ik,2)],[pt_start(ik,3); pt_end(ik,3)])
% end
% hold off
% axis equal



re = [5;
    5;
    5;
    5;
    2;
    2;
    2;]*1e-3;
shp_id = 2100*ones(Nc,1);

dim1 = re;
dim2 = zeros(Nc,1);

mur = ones(Nc,1);
sig = [5.8e7;
    5.8e7;
    5.8e7;
    5.8e7;
    5.8e7;
    5.8e7;
    5.8e7;];
S = pi*re.^2;
R_pul = 1./(sig.*S);

grp_id = zeros(Nc,1);
grp_type = zeros(Nc,1);
pt_2d = ones(Nc,1)*[0 0];


%% 2. setting for solver
soil_sig = 3.5e-3;
soil_epr = 10;

gnd_off = 0.1;

frq_cut = 500e3;

Nfit = 2;
dl =20;

fpath = 'F:\Spice_Project\Induced_Paper\';
fname = 'induced_paper';


flag_p    = 1;
flag_sol  = 2;
flag_mesh = 1;
flag_gnd  = 1;
flag_sr   = 2;


%% 3. branch and node names
% [nod_start, nod_end, nod_P] = sol_peec_brn_nod_create( ...
%     pt_start,pt_end, [], [],[], [],[], p_flag);
bran_name = [];
for ik=1:Nc
    bran_name = [bran_name; ['B%d',num2str(ik+10000)]];
end
nod_start = ['NP00001';
    'NP00009';
    'NN00001';
    'NN00009';
    'NG00001';
    'NG00009';
    'NG00015';
    ];
nod_end = ['NP00009';
    'NP00015';
    'NN00009';
    'NN00015';
    'NN00001';
    'NN00009';
    'NN00015';
    ];

bran_out = [];

nod_out = ['NP00001';
    'NP00009';
    'NP00015';
    'NN00001';
    'NN00009';
    'NN00015';
    'NG00001';
    'NG00009';
    'NG00015';
    ];


main_prog_air(pt_start,pt_end,re, shp_id,dim1,dim2, R_pul,sig,mur, ...
    grp_id, grp_type, pt_2d,...
    bran_name,nod_start,nod_end,bran_out,nod_out, ...
    soil_sig,soil_epr, gnd_off, frq_cut, Nfit,dl, fpath,fname, ...
    flag_p,flag_sol,flag_mesh,flag_gnd,flag_sr);






