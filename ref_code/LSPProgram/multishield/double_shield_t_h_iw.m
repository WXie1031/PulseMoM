function TH = double_shield_t_h_iw(d1,sig1,mur1,epr1, d2,sig2,mur2,epr2, w0)
%  Function:       double_shield_t_h_iw
%  Description:    Calculate transmission coefficient of multilayer 
%                  material for H field. Impinging field assumption. 
%                  Use wave impedance
%                  
%                            region  0   1    2          N      N+1
%                  wave -> left medium | L1 | L2 | ... | LM | right medium
%                            interface 1    2    3     N   N+1
%  Calls:          
%  Input:          h_ang   --  angle of incident field
%                  zbdy    --  boundary of each layer (N-1)x1
%                  sig_lyr --  conductivity of the layers (Nx1)
%                  epr_lyr --  relative permitivity of layers (Nx1)
%                  mur_lyr --  relative permeability of layers (Nx1)
%                  w0      --  frequencies (1xN)
%  Output:         TE  --  transmission coefficient of the multilayer shield
%  Others:         
%  Author:         Chen Hongcai
%  Email :         hc.chen@live.com
%  Date:           2017-12-14


%% media parameters of layers
mu0 = 4e-7*pi;
ep0 = 8.854187817e-12;

epr1 = epr1 - 1j.*sig1./(w0*ep0);
epr2 = epr2 - 1j.*sig2./(w0*ep0);

% characteristic impedance of the transmission line
k0 = w0*sqrt(ep0.*mu0);

% % % % % % % % % % % % % % 
% characteristic impedance
% % % % % % % % % % % % % %
ita0 = sqrt(mu0./ep0);
ita1 = sqrt(mu0*mur1./(ep0*epr1));
ita2 = sqrt(mu0*mur2./(ep0*epr2));

ita_nf = 1j*k0*ita0/2;



s_dep1 = sqrt(2/(w0*mu0*mur1*sig1));
s_dep2 = sqrt(2/(w0*mu0*mur2*sig2));

p = exp( (1+1j)*d1/s_dep1 );
q = exp( (1+1j)*d2/s_dep2 );

TH = 8*abs(ita_nf)*abs(ita1)*abs(ita2)./abs( (ita_nf+ita1)*(ita_nf+ita2)*(ita1+ita2)*p*q ...
    - (ita_nf+ita1)*(ita_nf-ita2)*(ita1-ita2)*p/q ...
    + (ita_nf-ita1)*(ita_nf+ita2)*(ita1-ita2)/p*q ...
    - (ita_nf-ita1)*(ita_nf-ita2)*(ita1+ita2)/p/q );


end


