
ep_lyr = [1; 10; 10;];
mur_lyr = [1;1;1];
sig_lyr = [0;1/50;1/20];
d_lyr = [0;0.6;0];
f0 = 1;

[gRTE_fwd,gRTE_bwd, gRTM_fwd,gRTM_bwd, ...
    RTE_fwd,RTE_bwd,RTM_fwd,RTM_bwd] = dgf_bdy_gcof...
    (ep_lyr,mur_lyr,sig_lyr,d_lyr, f0);

mu0 = 4e-7*pi;
ep0 = 8.854187817e-12;
w0 = 2*pi*f0;

epc_lyr = ep_lyr*ep0 + 1j.*sig_lyr./w0; 
kz = w0*sqrt(epc_lyr.*mu0.*mur_lyr);


id_lyr = 2;
[gRTE_fwd_ilyr,gRTE_bwd_ilyr, gRTM_fwd_ilyr,gRTM_bwd_ilyr] = ...
    dgf_bdy_gcof_ilyr(gRTE_fwd,gRTE_bwd, gRTM_fwd,gRTM_bwd,id_lyr);
gRTM_fwd_ilyr

ep_lyr = [1; 10;];
mur_lyr = [1;1];
sig_lyr = [0;1/29.1];
d_lyr = [0;0];
f0 = 1;

[gRTE_fwd,gRTE_bwd, gRTM_fwd,gRTM_bwd, ...
    RTE_fwd,RTE_bwd,RTM_fwd,RTM_bwd] = dgf_bdy_gcof...
    (ep_lyr,mur_lyr,sig_lyr,d_lyr, f0);
gRTM_fwd


