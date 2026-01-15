function [R, Lin, Zin] = zs_tube_in_rtn(r_out,r_in,sig,mur, len, frq)
%  Function:       zs_tube_in_rtn
%  Description:    Calculate R and L of circular conductors using bessel
%                  function. Corresponding to Zaa in "The Electromagnetic
%                  Theory of Coaxial Transmission Lines and Cylindrical
%                  Shields"  ---- surface impedance with external return
%                  This is for the situation that current in coaxial
%                  structure is different.
%  Calls:          besseli_sub
%                  besselk_sub
%
%  Input:          r_out     --  outer radius of conductors (N*1) (m)
%                  r_in      --  inner radius of conductors (N*1) (m)
%                  sig       --  conductivity of conductors (N*1) (S/m)
%                  mur       --  relative permeability
%                  len       --  length of conductors (N*1)
%                  f0        --  frequency
%  Output:         R    --  R vector (have multiplied length)
%                  L    --  L vector (have multiplied length)
%  Others:
%  Author:         Chen Hongcai
%  Email :         hc.chen@live.com
%  Date:           2016-11-23
%  History:


mu0 = 4*pi*1e-7;
mu = mu0*mur;

w = 2*pi*frq;

Na = length(r_out);
Nf = length(frq);
R = zeros(Na,Nf);
Lin = zeros(Na,Nf);
Zin = zeros(Na,Nf);

% S = pi*(r_out.^2-r_in.^2);
% Rdc = 1/(sig.*S).*len;
% s_dep = 1./sqrt(pi*frq(ind)*mu(ik)*sig(ik));


for ik = 1 : Na
    if r_in(ik) == 0
        % self impedance for solid
        Ro = sqrt(1j*w*sig(ik)*mu(ik)) .* r_out(ik);
        
        I0r = besseli(0,Ro);
        I1r = besseli(1,Ro);
        
        % self impedance
        Zin(ik,1:Nf) = 1j*w*mu(ik)./(2*pi*Ro) .* (I0r./I1r) .*len(ik);
        
        % for very large arguments
        ind = find(isinf(Zin(ik,:))|isnan(Zin(ik,:)), 1);
        if ~isempty(ind)
            Rtmp = ( 1./(2*r_out(ik)).*sqrt(mu(ik)*frq(end)./(pi*sig(ik)))+1/(4*pi*sig(ik)*r_out(ik)^2) ).* len(ik);
            wLtmp = 1./(2*r_out(ik)).*sqrt(mu(ik)*frq(end)./(pi*sig(ik))) .* len(ik);
            Zin(ik,end) = Rtmp + wLtmp;
            
            ind = isinf(Zin(ik,:))|isnan(Zin(ik,:));
            Ztmp = interp1(frq(~ind),Zin(ik,~ind),frq(ind),'pchip');
            Zin(ik,ind) = Ztmp;
        end
        
    else
        % self impedance for ring
        Ri = sqrt(1j*w*sig(ik)*mu(ik)) .* r_in(ik); % inner radius
        Ro = sqrt(1j*w*sig(ik)*mu(ik)) .* r_out(ik); % outer radius
        
        I0o = besseli(0,Ro);
        I1i = besseli(1,Ri);
        I1o = besseli(1,Ro);
        K0o = besselk(0,Ro);
        K1i = besselk(1,Ri);
        K1o = besselk(1,Ro);
        
        % self impedance
        Zin(ik,1:Nf) = 1j*w*mu(ik)./(2*pi*Ro) .* ...
            ( (I0o.*K1i+K0o.*I1i)./(I1o.*K1i-K1o.*I1i) ) .*len(ik);
        
        % for very large arguments
        ind = find(isinf(Zin(ik,:))|isnan(Zin(ik,:)), 1);
        if ~isempty(ind)
            S = pi*(r_out(ik).^2-r_in(ik).^2);
            Rdc = 1/(sig(ik).*S).*len(ik);
            Zin(ik,end) = Rdc*(Ro(end)-Ri(end))*coth(Ro(end)-Ri(end));
            
            ind = isinf(Zin(ik,:))|isnan(Zin(ik,:));
            Ztmp = interp1(frq(~ind),Zin(ik,~ind),frq(ind),'pchip');
            Zin(ik,ind) = Ztmp;
        end
    end
    
    R(ik,:) = real(Zin(ik,:));
    Lin(ik,:) = imag(Zin(ik,:))./w;
end



end



