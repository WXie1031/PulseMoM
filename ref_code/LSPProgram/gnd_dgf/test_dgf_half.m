
mu0 = 4e-7*pi;
ep0 = 8.854187817e-12;

%% configuration of the multi-layer structure
%% case 1
w0= 2*pi*300e6;

w0= 2*pi*1e9;

k0 = w0*sqrt(ep0.*mu0);
lumda = 2*pi/k0;

zf = linspace(0.001,2*lumda,50);
Nxf = length(zf);

pt_s = [0,0,1e-9];
pt_f = [zeros(Nxf,1),0.1*lumda*ones(Nxf,1),zf'];

xx = linspace(0.0,2*lumda,50);
Nx = length(zf);
pt_s = [0,0,1.22];
pt_f = [xx',zeros(Nx,1),1.12*ones(Nx,1)];

epr_lyr = [ 1; 1; ];
mur_lyr = [1; 1; ];
sig_lyr = [0; 0];

zbdy = [0; ]*1e-3; 


%% case 2 
% w0= 2*pi*1e9;
% 
% pt_s = [0,0,-0.25;];
% pt_f = [0,1.0,-0.25;];
% 
% epr_lyr = [ 1; 9; 12; 4; ];
% mur_lyr = [ 1; 1;  1; 1; ];
% sig_lyr = [0; 0.001;  0.05; 0.1;];
% 
% zbdy = [0; -0.3; -0.6; ];


%% calculate the transmission Green's functions

dv_s=[1 0 0];
dv_f=ones(Nxf,1)*[1 0 0];
% tic
% dgf_main_sub_same(pt_s,dv_s, pt_f,dv_f, zbdy,epr_lyr,mur_lyr,sig_lyr, w0);
% toc
tic
[Gxx,Gzx,Gzz,Gphi] = dgf_main(pt_s,dv_s, pt_f,dv_f, zbdy,epr_lyr,mur_lyr,sig_lyr, w0);
toc

rr = max(1e-9, sqrt((pt_f(:,1)-pt_s(:,1)).^2 + (pt_f(:,2)-pt_s(:,2)).^2 + (pt_f(:,3)-pt_s(:,3)).^2 ));
Gxx_air = exp(-1j*k0*abs(rr))./(4*pi*rr);

figure(5)
hold on
plot(zf/lumda,real(Gxx),'r')
plot(zf/lumda,imag(Gxx),'r')
plot(zf/lumda,real(Gzx),'r--')
plot(zf/lumda,imag(Gzx),'r--')
plot(zf/lumda,real(Gzz),'r-.')
plot(zf/lumda,imag(Gzz),'r-.')
plot(zf/lumda,real(Gphi),'k-.')
plot(zf/lumda,imag(Gphi),'k-.')
plot(zf/lumda,real(Gxx_air),'b')
plot(zf/lumda,imag(Gxx_air),'b')
%legend('Gxx','Gzx','Gzz','Gphi');
