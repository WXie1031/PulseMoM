function TE = multi_shield_t_e_iw(e_ang, zbdy, sig_lyr,mur_lyr,epr_lyr,w0)
%  Function:       multi_shield_t_e_pw
%  Description:    Calculate transmission coefficient of multilayer 
%                  shield for E field. Impinging field assumption. 
%                  Use wave impedance
%                  
%                            region  0   1    2          N      N+1
%                  wave -> left medium | L1 | L2 | ... | LM | right medium
%                            interface 1    2    3     N   N+1
%  Calls:          
%  Input:          e_ang   --  angle of incident field
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


Nbdy = size(zbdy,1);
Nlyr = Nbdy+1;
d_lyr = [1e6; zbdy(1:Nbdy-1)-zbdy(2:Nbdy); 1e6;];

%% media parameters of layers
mu0 = 4e-7*pi;
ep0 = 8.854187817e-12;

mu_lyr = mu0*mur_lyr;
epr_lyr = epr_lyr - 1j.*sig_lyr./(w0*ep0);
ep_lyr = ep0*epr_lyr;

% characteristic impedance of the transmission line
kn = w0*sqrt(ep_lyr.*mu_lyr);

% % % % % % % % % % % % % % 
% characteristic impedance
% % % % % % % % % % % % % % 
ita = sqrt(mu_lyr./ep_lyr);
Ze = ita*cos(e_ang);


% forward layer recursion
gZe_ij = zeros(Nbdy,1);
gZe_ij(Nbdy) = Ze(Nbdy+1);
for ih = Nbdy-1:-1:1
    gZe_ij(ih) = dgf_bdy_gz(Ze(ih+1),Ze(ih+2),gZe_ij(ih+1), kn(ih+1), d_lyr(ih+1) );
end


pn = 2^(Nlyr-1) * prod(Ze(1:Nlyr-1)) ./ prod(Ze(1:Nlyr-1)+Ze(2:Nlyr));

qn = (Ze(2:Nlyr-1)-Ze(1:Nlyr-2))./(Ze(2:Nlyr-1)+Ze(1:Nlyr-2)) .* ...
    (Ze(2:Nlyr-1)-gZe_ij(2:Nbdy))./(Ze(2:Nlyr-1)+gZe_ij(2:Nbdy));

TE = pn ./ prod(1-qn.*exp(-2*kn(2:Nlyr-1).*d_lyr(2:Nlyr-1))).*prod(exp(-kn(2:Nlyr-1).*d_lyr(2:Nlyr-1))) ;



end


