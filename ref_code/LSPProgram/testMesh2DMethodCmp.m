
frq = [1 50 100 200 500 1e3 2e3  5e3  10e3 20e3 50e3 100e3  200e3 500e3];
frq = 1e6;

%  shape_cs2D = [2100; 2100; 1003; 1003;];
%  pt_cs2D = [0 80; 0 80; 0 0; 300 0;]*1e-3;
%  dim1_cs2D = [12.45; 4.5; 50; -50]*1e-3;
%  dim2_cs2D = [11.65; 0;  5;  5;]*1e-3;
%  Rin_pul_cs2D = [1.18e-3;  1.05e-3;  0.226123e-3; 0.226123e-3];
% Lin_pul_cs2D = zeros(length(shape_cs2D),1);
%  len_cs2D = 3;
%  
% murtmp = 1;
% Nf = size(frq,2);
% Nc = size(pt_cs2D,1);
% mur = ones(Nc,Nf)*murtmp;
% 
% pre_mtx=[];
% 
% [Rmesh1,Lmesh1, Rself1,Lself1, RmeshMF1,LmeshMF1] = mesh2d_main_complete( ...
%     shape_cs2D, pt_cs2D, dim1_cs2D, dim2_cs2D, Rin_pul_cs2D,mur,len_cs2D, frq, pre_mtx);

% for ik=1:Nf
%     for ig=1:Nc
%         RmeshMF1(ig,ig,ik)=0;
%         LmeshMF1(ig,ig,ik)=0;
%     end
% end

% Nfit = 3;
% [R0fit,L0fit,Rvfit,Lvfit,Zvfit] = main_vectfit_z_mtx3(RmeshMF1,LmeshMF1, frq, Nfit);



shp_id = [2100; 2100; 2100; 2100;2100; 2100;...
    2100; 2100;2100; 2100;2100; 2100; ...
    1003; 1003;];
 pt_2d = [100 12.5; 100 12.5; 100 52.5; 100 52.5; 100 92.5;100 92.5; ...
     140 12.5; 140 12.5; 140 52.5; 140 52.5; 140 92.5;140 92.5; ...
     0 0; 300 0;]*1e-3;
 dim1 = [12.45; 4.5; 12.45; 4.5; 12.45; 4.5;...
     12.45; 4.5; 12.45; 4.5; 12.45; 4.5;...
     50; -50]*1e-3;
 dim2 = [11.65; 0; 11.65; 0; 11.65; 0; ...
     11.65; 0; 11.65; 0; 11.65; 0; ...
     5;  5;]*1e-3;
 Rpul = [1.18e-3;  1.05e-3; 1.18e-3;  1.05e-3; 1.18e-3;  1.05e-3; ... 
     1.18e-3;  1.05e-3; 1.18e-3;  1.05e-3; 1.18e-3;  1.05e-3; ...
     0.226123e-3; 0.226123e-3];

S = pi*(dim1.^2-dim2.^2);
sig = 1./(Rpul.*S);
len = 3;

murtmp = 1;
Nf = size(frq,2);
Nc = size(pt_2d,1);
mur = ones(Nc,Nf)*murtmp;
%  
len = len*ones(Nc,1);
pt_start = [pt_2d, zeros(Nc,1)];
pt_end  = [pt_2d, len]; 
[dv, len] = line_dv(pt_start, pt_end);
re = dim1;
[Rmtx, Lmtx] = para_main_fila(pt_start, pt_end, dv, re, len);
pre_mtx = Rpul + 1j*2*pi*100e3;

[Rmesh2,Lmesh2, Rself2,Lself2, RmeshMF2,LmeshMF2] = main_mesh2d_method_cmp( ...
    shp_id, pt_2d, dim1, dim2, Rpul,sig, mur,len, frq, pre_mtx);

