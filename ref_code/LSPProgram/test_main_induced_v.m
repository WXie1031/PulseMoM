
clear

load('4corner_tower_1sdc.mat');

sig_soil = 0.01;
frq_cut = 1e6;
Nfrq = 50;
dl = 5;
gnd_off = 0.2;

flag_sol = 2;
flag_mesh = 2;
flag_gnd = 2;
flag_p = 1;


Imax=100e3;
tau1=0.454;
tau2=143;
n=10;

Nt=400;
dt=1e-6;
Tmax = Nt*dt;

[i_sr, tus] = sr_heidler(Imax, tau1, tau2, n, Tmax*1e6, dt*1e6, 0);
t_sr = tus*1e-6;
Nt = length(tus);

pt_hit = [50 0];
Ns_ch = 200;
h_ch = 2.5e3;
[Uout1,Er_T,Ez_T] = sr_induced_v_num(pt_hit,h_ch,Ns_ch, pt_start,pt_end, i_sr, t_sr,1);

[Rmtx,Lmtx,Pmtx] = para_main_fila_rlp(p_sta_L,p_end_L,dv_L,re_L,l_L, ...
    p_sta_P,p_end_P,dv_P,re_P,l_P, flag_p);


fpath='D:\ProjectFiles\test_induced_v';
fname='TEST_UI';
spice_air_self_vf_vi(Rmtx,Lmtx,[], [0;0],[0;0], ['B1';'B2'],['N1';'N2';'N3'], ...
    ['N1';'N2'], ['N2';'N3'], 'B1', ['N1';'N2';'N3'], Uout1,t_sr, ['UB1';'UB2'], fpath, fname);


