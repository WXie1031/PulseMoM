function [R, Lin] = zs_plate_sonnet(t, sig, mur, frq)
%  Function:       zs_plate_sonnet
%  Description:    Calculate R and L of of the thin plate using
%                  surface impedance method. R and L are the internal 
%                  resistance and inductance of specified frequency. The
%                  formulat is used Sonnet as introduced in "Microstrip
%                  conductor loss models for electromagnetic anaylsis". In
%                  this version, use uniform two-layer model
%
%  Calls:          
%
%  Input:          t      --  thickness of the plate (N*1) (m)
%                  sig    --  conductivity of conductors (N*1) (S/m)
%                  mur    --  relative permeability
%                  frq    --  frequency (1*Nf)
%  Output:         R    --  R vector (have multiplied the surface area)
%                  L    --  L vector (have multiplied the surface area)
%  Others:
%  Author:         Chen Hongcai
%  Email :         hc.chen@live.com
%  Date:           2016-05-03
%  History:        

mu0 = 4*pi*1e-7;
mu = mu0*mur;

%ep0 = 8.85*1e-12;
%ep = ep0*epr;
w0 = 2*pi*frq;

Nc = length(t);
Nf = length(frq);

t = abs(t);

% S = wid.*hig/2;
Rdc = 1./(sig.*t/2); % two layer - this method should be enhenced in future
Rrf = sqrt(pi.*mu./sig);

Zs = zeros(Nc,Nf);
R = zeros(Nc,Nf);
Lin = zeros(Nc,Nf);
for ik = 1:Nc

    Zs(ik,1:Nf) = (1-1j).*Rrf(ik).*sqrt(frq).*cot( (1-1j).*Rrf.*sqrt(frq)./Rdc(ik)); 
    Zs(ik,1:Nf) = Zs(ik,1:Nf)/2; % uniform layer
    
    R(ik,1:Nf) = real(Zs(ik,1:Nf));
    Lin(ik,1:Nf) = imag(Zs(ik,1:Nf))./w0;
end


end
    
    
