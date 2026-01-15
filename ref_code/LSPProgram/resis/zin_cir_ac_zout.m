function Zcir = zin_cir_ac_zout(r_out, r_in, sig, mur, len, frq)
%  Function:       induct_cir_ac
%  Description:    Calculate R and L of circular conductors using bessel
%                  function. R and L are the internal resistance and
%                  inductance of specified frequency.
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
%                  2015-06-25

mu0 = 4*pi*1e-7;
mu = mu0*mur;

w = 2*pi*frq;

Na = length(r_out);
Nf = length(frq);
Zcir = zeros(Na,Nf);

% S = pi*(r_out.^2-r_in.^2);
% Rdc = 1/(sig.*S).*len;

% Nth = 1000;   % threshold for use of Bessel function

if frq < 1e7
    for ik = 1 : Na
        
       if r_in(ik) == 0
            % self impedance for solid
            Ro = sqrt(1j*w*sig(ik)*mu(ik)) .* r_out(ik);
            
            if  r_out(ik)>5
                I0r = besseli_sub(0,Ro);
                I1r = besseli_sub(1,Ro);
            else
                I0r = besseli(0,Ro);
                I1r = besseli(1,Ro);
            end

            % self impedance
            Zcir(ik,1:Nf) = 1j*w*mu(ik)./(2*pi*Ro) .* (I0r./I1r) .*len(ik);
            
        else
            % self impedance for ring
            
            Ri = sqrt(1j*w*sig(ik)*mu(ik)) .* r_in(ik); % inner radius
            Ro = sqrt(1j*w*sig(ik)*mu(ik)) .* r_out(ik); % outer radius
            
            if  r_out(ik)>5
                % approximation method
                I0o = besseli_sub(0,Ro);
                I1i = besseli_sub(1,Ri);
                I1o = besseli_sub(1,Ro);
                K0o = besselk_sub(0,Ro);
                K1i = besselk_sub(1,Ri);
                K1o = besselk_sub(1,Ro);
            else
                I0o = besseli(0,Ro);
                I1i = besseli(1,Ri);
                I1o = besseli(1,Ro);
                K0o = besselk(0,Ro);
                K1i = besselk(1,Ri);
                K1o = besselk(1,Ro);
            end
            
            cof1 = 1;
            cof2 = 1;
            
%             cof1 = exp(Ro-Ri);
%             cof2 = 1./cof1;
            
            % self impedance
            Zcir(ik,1:Nf) = -1j*w*mu(ik)./(2*pi*Ro) .* ...
                ( (cof1.*I0o.*K1i+cof2.*K0o.*I1i)./ ...
                (cof2.*K1o.*I1i-cof1.*I1o.*K1i) ) .*len(ik);
        end
    end

end


end



