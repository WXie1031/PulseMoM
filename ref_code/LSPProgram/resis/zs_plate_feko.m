function [R, Lin] = zs_plate_feko(wid,hig, d, sig, mur, epr, frq)
%  Function:       zs_plate_feko
%  Description:    Calculate R and L of of the thin plate using
%                  surface impedance method. R and L are the internal 
%                  resistance and inductance of specified frequency. The
%                  formula is from FEKO user guide
%
%  Calls:          
%
%  Input:          wid    --  length of thin plate (N*1) (m)
%                  hig    --  width of thin plate (N*1) (m)
%                  d      --  DC resistancee of conductors
%                  sig    --  conductivity of conductors (N*1) (S/m)
%                  mur    --  relative permeability
%                  epr    --  relative permittivity
%                  frq    --  frequency (1*Nf)
%  Output:         R    --  R vector (have multiplied the surface area)
%                  L    --  L vector (have multiplied the surface area)
%  Others:
%  Author:         Chen Hongcai
%  Email :         hc.chen@live.com
%  Date:           2016-05-03
%  History:        

mu0 = 4*pi*1e-7;
ep0 = 8.85*1e-12;

w0 = 2*pi*frq;

mu = mu0*mur;
ep = ep0*epr;

Nc = length(wid);
Nf = length(frq);

%S = wid.*hig;

Zs = zeros(Nc,Nf);
R = zeros(Nc,Nf);
Lin = zeros(Nc,Nf);
for ik = 1:Nc
    
    sig_w = sig(ik)./(1j*w0);
    
    kc = w0 .* sqrt( mu(ik).*( ep(ik)+sig_w ) );
    
    % wave impedance in the air
    Z0 = sqrt( mu0./( ep0+sig_w ) ); 
    % wave impedance in the plate
    Zw  = sqrt( mu(ik)./( ep(ik)+sig_w) );
    Zref = (Z0-Zw)./(Z0+Zw);
    % here use d/2 is the two layer approach
    Zs(ik,1:Nf) = Zw.*( 1 + Zref.*exp(-1j*2*kc.*d/2) ) ...
        ./ ( 1 - Zref.*exp(-1j*2.*kc.*d/2) + (Zref-1).*exp(-1j.*kc.*d/2) );
    Zs(ik,1:Nf) = Zs(ik,1:Nf)/2; % /2 is uniform two layer
    
    R(ik,1:Nf) = real(Zs(ik,1:Nf));
    Lin(ik,1:Nf) = imag(Zs(ik,1:Nf))./w0;
end


end
    
    
