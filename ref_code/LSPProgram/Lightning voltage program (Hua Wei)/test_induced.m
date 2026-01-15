% pt_start = [200.1,0,6;500.1,0,6;1500.1,0,6];
% pt_end = [199.9,0,6;499.9,0,6;1499.1,0,6];

pt_start = [100.1,0,0;100.1,0,6];
pt_end = [99.9,0,0;99.9,0,6];


Nc=size(pt_start,1);

pt_hit = [0 0];     % position of channel

%%%%%%%%%%%%%%%%%% Heidler Coefficient
Imax=1e4;

k0=0.93;
% tau1=0.454;
% tau2=143;
tau1=19*1e-6;
tau2=485*1e-6;
n=10;


alpha1=3e4;
alpha2=1e7;

i_sr_type=2; %% 1 for Heidler ;2 for Double Exp

%%%%%%%%%%%%%%%%
h_ch = 2500; % Height of return stroke
ve=1.1e8;

t_ob=30000e-9;
% Nt=10000;   % time steps

flag_type=1; % 1 for TL; 2 for MTLL; 3 for MTLE;

%%%%%%%%%%%%%%%%
erg=10;
sigma_g=0.01;
ground_type=2; % 1 for perfect ground; 2 for lossy ground;

%%%%%%%%%%%%

dt_out=4.1e-9;


% [Tout, Uout2] = main_Lightning_induced_Vol_Sr(pt_start, pt_end, Nc, pt_hit, Imax, k0, tau1, tau2, n, alpha1, alpha2, ...
%                         i_sr_type, h_ch, t_ob, flag_type, erg, sigma_g, ground_type, dt_out,ve);