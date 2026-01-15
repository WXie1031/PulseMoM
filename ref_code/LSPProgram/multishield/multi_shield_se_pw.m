function TH = multi_shield_se_pw(zbdy, sig_lyr, mur_lyr, epr_lyr,w0)
%  Function:       multi_shield_se
%  Description:    Calculate transmission coefficient of multilayer shield.
%                  Plane wave assumption. Use characteristic impedance.
%                  Ze = k/(w*ep)  Zh = (w*mu)/k 
%                  
%                            region  0   1    2          N      N+1
%                  wave -> left medium | L1 | L2 | ... | LM | right medium
%                            interface 1    2    3     N   N+1
%  Calls:          
%  Input:          zbdy    --  boundary of each layer (N-1)x1
%                  sig_lyr --  conductivity of the layers (Nx1)
%                  epr_lyr --  relative permitivity of layers (Nx1)
%                  mur_lyr --  relative permeability of layers (Nx1)
%                  w0      --  frequencies (1xN)
%  Output:         SF   --  shileding effectiveness (1xN)
%  Others:         
%  Author:         Chen Hongcai
%  Email :         hc.chen@live.com
%  Date:           2017-12-03


%% media parameters of layers
mu0 = 4e-7*pi;
ep0 = 8.854187817e-12;

mu_lyr = mu0*mur_lyr;
epr_lyr = epr_lyr - 1j.*sig_lyr./(w0*ep0);
ep_lyr = ep0*epr_lyr;

kn = w0*sqrt(ep_lyr.*mu_lyr);

Nbdy = size(zbdy,1);
Nlyr = Nbdy+1;
d_lyr = [1e6; zbdy(1:Nbdy-1)-zbdy(2:Nbdy); 1e6;];


gZe_ij = zeros(Nbdy,1);
gZh_ij = zeros(Nbdy,1);

% characteristic impedance of the transmission line
Ze = zeros(Nlyr,1);
Zh = zeros(Nlyr,1);
for ih = 1:Nlyr
    Ze(ih) = kn(ih)./(w0*ep_lyr(ih));
    Zh(ih) = (w0*mu_lyr(ih))./kn(ih);
end

% Ze = sqrt(1j*w0*mu_lyr./(sig_lyr+1j*w0*ep_lyr));
% Zh = sqrt(1j*w0*mu_lyr./(sig_lyr+1j*w0*ep_lyr));

% forward layer recursion
gZe_ij(Nbdy) = Ze(Nbdy+1);
gZh_ij(Nbdy) = Zh(Nbdy+1);
for ih = Nbdy-1:-1:1
    gZe_ij(ih) = dgf_bdy_gz(Ze(ih+1),Ze(ih+2),gZe_ij(ih+1), kn(ih+1), d_lyr(ih+1) );
    gZh_ij(ih) = dgf_bdy_gz(Zh(ih+1),Zh(ih+2),gZh_ij(ih+1), kn(ih+1), d_lyr(ih+1) );
end


pn = 2^(Nlyr-1) * prod(Zh(1:Nlyr-1)) ./ prod(Zh(1:Nlyr-1)+Zh(2:Nlyr));

qn = (Zh(2:Nlyr-1)-Zh(1:Nlyr-2))./(Zh(2:Nlyr-1)+Zh(1:Nlyr-2)) .* ...
    (Zh(2:Nlyr-1)-gZh_ij(2:Nbdy))./(Zh(2:Nlyr-1)+gZh_ij(2:Nbdy));

TH = pn ./ prod(1-qn.*exp(-2*kn(2:Nlyr-1).*d_lyr(2:Nlyr-1))).*prod(exp(-kn(2:Nlyr-1).*d_lyr(2:Nlyr-1))) ;

% pn = 2^(Nlyr-1) * prod(Ze(1:Nlyr-1)) ./ prod(Ze(1:Nlyr-1)+Ze(2:Nlyr));
% 
% qn = (Ze(2:Nlyr-1)-Ze(1:Nlyr-2))./(Ze(2:Nlyr-1)+Ze(1:Nlyr-2)) .* ...
%     (Ze(2:Nlyr-1)-gZe_ij(2:Nbdy))./(Ze(2:Nlyr-1)+gZe_ij(2:Nbdy));
% TE = pn ./ prod(1-qn.*exp(-2*rn(2:Nlyr-1).*d_lyr(2:Nlyr-1))).*prod(exp(-rn(2:Nlyr-1).*d_lyr(2:Nlyr-1))) ;


end


