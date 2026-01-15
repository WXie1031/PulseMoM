function [R, Lin] = zs_plate_du(wid,hig, d, sig, mur, frq)
%  Function:       zs_plate_du
%  Description:    Calculate R and L of of the thin plate using
%                  surface impedance method. R and L are the internal 
%                  resistance and inductance of specified frequency. The
%                  formulat is proposed by Du using double exponential 
%                  distribution.
%                  The formula is not accurate when frequency is very low.
%
%  Calls:          
%
%  Input:          wid    --  length of thin plate (N*1) (m)
%                  hig    --  width of thin plate (N*1) (m)
%                  d      --  DC resistancee of conductors
%                  sig    --  conductivity of conductors (N*1) (S/m)
%                  mur    --  relative permeability
%                  frq    --  frequency (1*Nf)
%  Output:         R    --  R vector (have multiplied the surface area)
%                  L    --  L vector (have multiplied the surface area)
%  Others:
%  Author:         Chen Hongcai
%  Email :         hc.chen@live.com
%  Date:           2016-05-23
%  History:        

mu0 = 4*pi*1e-7;
mu = mu0*mur;

%ep0 = 8.85*1e-12;
%ep = ep0*epr;
w0 = 2*pi*frq;

Nc = length(wid);
Nf = length(frq);

% S = wid.*hig/2;
a0 = -sqrt(1j*2*pi.*frq.*mu.*sig);

Zs = zeros(Nc,Nf);
R = zeros(Nc,Nf);
Lin = zeros(Nc,Nf);
for ik = 1:Nc

    Zs(ik,1:Nf) = 1./sig .* a0.*sinh(a0.*d)./(2*(cosh(a0.*d)-1)); 
    
    R(ik,1:Nf) = real(Zs(ik,1:Nf));
    Lin(ik,1:Nf) = imag(Zs(ik,1:Nf))./w0;
end


end
    
    
