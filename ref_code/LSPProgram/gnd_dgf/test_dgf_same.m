
%addpath('..\dgf-strata-main\test\examples\ling_jin_2000')

mu0 = 4e-7*pi;
ep0 = 8.854187817e-12;

%% configuration of the multi-layer structure
%% case 1
w0= 2*pi*30e9;
k0 = w0*sqrt(ep0.*mu0);
% xf = logspace(log10(1e-3/k0),log10(100/k0),100);
% xf = [logspace(log10(1e-3/k0),log10(9/k0),100) linspace(10/k0,(100/k0),200)];
T = textread('GA.txt');
xf = T(:,1);
% xf = T((T(:,1)>=1),1);
Nxf = length(xf);
pt_s = [0,0,-1.4]*1e-3;
pt_f = [xf./k0,zeros(Nxf,1),-1.4*ones(Nxf,1)*1e-3];

epr_lyr = [ 1; 2.1; 12.5; 9.8; 8.6; 1];

mur_lyr = [1; 1; 1; 1; 1; 1;];
sig_lyr = [0; 0; 0; 0; 0; 1e20];

% epr_lyr = [ 1; 1; 1; 1; 1; 1];
% sig_lyr = [0; 0; 0; 0; 0; 0];


zbdy = [0; -0.7; -1; -1.5; -1.8; ]*1e-3; 


%% case 2 
% lumda = 633e-9;
% f0 = vc/lumda;
% w0= 2*pi*f0;
% 
% pt_s = [0,0,750e-9;];
% 
% xf = [1:1000]'*1e-9;
% Nzf = length(xf);
% pt_f = [zeros(Nzf,1),zeros(Nzf,1),xf];
% 
% epr_lyr = [ 1; 2;   10;  1; ];
% mur_lyr = [ 1; 0.8; 1.2; 1; ];
% sig_lyr = [ 0; 0;   0;   0; ];
% 
% zbdy = [0; -500e-9; -1000e-9; ];


%% calculate the transmission Green's functions

dv_s=[1 0 0];
Nf = size(pt_f,1);
dv_f=ones(Nf,1)*[1 0 0];
% tic
% dgf_main_sub_same(pt_s,dv_s, pt_f,dv_f, zbdy,epr_lyr,mur_lyr,sig_lyr, w0);
% toc
tic
[Gxx,Gzx,Gzz,Gphi] = dgf_main(pt_s,dv_s, pt_f,dv_f, zbdy,epr_lyr,mur_lyr,sig_lyr, w0);
toc
% tic
% dgf_main_sub_same(pt_s,dv_s, pt_f,dv_f, zbdy,epr_lyr,mur_lyr,sig_lyr, w0);
% toc

% tic
% for ik = 1:Nxf
%     
% [Gxx(1,ik),Gzx(1,ik),Gzz(1,ik),Gphi(1,ik)] = dgf_main...
%     (pt_s,dv_s, pt_f(ik,:),dv_f(ik,:), zbdy,epr_lyr,mur_lyr,sig_lyr, w0);
% end
% toc

rr = max(1e-9, sqrt((pt_f(:,1)-pt_s(:,1)).^2 + (pt_f(:,2)-pt_s(:,2)).^2 + (pt_f(:,3)-pt_s(:,3)).^2 ));
Gxx_air = exp(-1j*k0*abs(rr))./(4*pi*rr);


figure(3)

loglog(xf,abs(Gxx))
hold on
loglog(xf,abs(Gzx))
loglog(xf,abs(Gzz))
loglog(xf,abs(Gphi))
% loglog(xf,abs(Gxx_air),'r')
legend('Gxx','Gzx','Gzz','Gphi');

% 
% figure(5)
% 
% loglog(xf,real(Gxx))
% hold on
% loglog(xf,imag(Gxx))
% loglog(xf,real(Gzx))
% loglog(xf,imag(Gzx))
% loglog(xf,real(Gzz))
% loglog(xf,imag(Gzz))
% loglog(xf,real(Gphi))
% loglog(xf,imag(Gphi))
% loglog(xf,real(Gxx_air),'r')
% loglog(xf,imag(Gxx_air),'b')


% figure(3)
% loglog((xf),(abs(T(:,5))),'k','LineWidth',1)
% hold on
% loglog((xf),(abs(T(:,6))),'k','LineWidth',1)

% load('Gxx_exp1.mat')
% 
% loglog((Gxx_exp1(:,1)),(Gxx_exp1(:,2)),'k','LineWidth',1)
% 
% load('Gzx_exp1.mat')
% loglog((xf),(abs(T(:,6))),'k','LineWidth',1)
% 
% load('Gzz_exp1.mat')
% loglog((xf),(abs(T(:,6))),'k','LineWidth',1)
% 
% load('Gphi_exp1.mat')
% loglog((xf),(abs(T(:,6))),'k','LineWidth',1)







