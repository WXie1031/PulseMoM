function [R, Lin, Zcir] = zs_cir_feko(r_out, r_in, sig, mur, len, frq)
%  Function:       zs_cir_feko
%  Description:    Calculate R and L of circular conductors using bessel
%                  function. R and L are the internal resistance and
%                  inductance of specified frequency.The formula is from
%                  FEKO user guide
%
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
%  Date:           2015-06-16
%  History:        Corrent the Lin in case of r_in<s_dep when the result is
%                  negtive. In this case, R should be no smaller than Rdc.
%                  2016-11-12

mu0 = 4*pi*1e-7;
mu = mu0*mur;

w = 2*pi*frq;

Na = length(r_out);
Nf = length(frq);
Zcir = zeros(Na,Nf);
R = zeros(Na,Nf);
Lin = zeros(Na,Nf);
% S = pi*(r_out.^2-r_in.^2);
% Rdc = 1/(sig.*S).*len;

% Nth = 1000;   % threshold for use of Bessel function

if frq < 1e7
    for ik = 1:Na
        
        if r_in(ik) == 0
            
            s_dep = sqrt(1/(pi.*frq*mu*sig));
            % self impedance for solid
            Ro = (1-1j)*r_out(ik)/sig(ik) ;
            
            I0r = besseli(0,Ro);
            I1r = besseli(1,Ro);
            
            % self impedance
            Zcir(ik,1:Nf) = (1-1j)./(2*pi*r_out(ik)*sig(ik).*s_dep) ...
                .* I0r./I1r .*len(ik);
            
        end
        
        R(ik,1:Nf) = real(Zcir(ik,1:Nf));
        Lin(ik,1:Nf) = imag(Zcir(ik,1:Nf))./w;
    end
    
else
    
    R = 1./(2*r_out).*sqrt(mu./(pi*sig)).*sqrt(frq) .*len;
    Lin = 1./(4*pi*r_out).*sqrt(mu./(pi*sig))./sqrt(frq) .*len;
    
end


end



