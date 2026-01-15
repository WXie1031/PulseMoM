function [Rmg,Lmg, Zg, Rsg,Lsg] = zg_tline_ol_b1(Rmg, Lmg, pt_2d,re, len, ...
    sig_soil, epr_soil, frq)
%  Function:       zg_tline_ol_b1
%  Description:    Calculate mutual earth rerurn impedance for overhead
%                  lines using exact formula with numerical integration.
%                  b1 means 1 layer ground.
%                  Sunde's model in paper "External Impedance and Admittance
%                  of Burid Horizontal Wires for Transient Studies Using
%                  Transmission Line Analysis"
%  Calls:
%
%  Input:          Hs    --  height of source conductors (N*2) (m)
%                  pt_end    --  end point of conductors (N*2) (m)
%                  re        --  equivalent radius (N*1)
%                  len       --  length of conductors (N*1)
%                  sig_soil  --  soil conductivity (S/m)
%                  ep_soil   --  relative permittivity of soil
%                  frq       --  frequency squeese
%  Output:         Rgrid --  R matrix
%                  Lgrid --  L matrix
%  Others:
%  Author:         Chen Hongcai
%  Email :         hc.chen@live.com
%  Date:           2016-06-17

mu0 = 4*pi*1e-7;
ep0 = 8.85e-12;

wm = 2*pi*frq;

Nc = size(pt_2d,1);
Nfrq = length(frq);

if isempty(Rmg)
    Rmg=zeros(Nc,Nc,Nfrq);
end
if isempty(Lmg)
    Lmg=zeros(Nc,Nc,Nfrq);
end
Zg = zeros(Nc,Nc,Nfrq);
Rsg = zeros(Nc,Nfrq);
Lsg = zeros(Nc,Nfrq);

rg = sqrt( 1j.*wm.*mu0.*(sig_soil + 1j.*wm.*ep0*epr_soil) );

for ig = 1:Nc
    
    if pt_2d(ig,2)>0
        id_ol = find(pt_2d(:,2)>=0);
        Nol = length(id_ol);
        
        H1 = pt_2d(ig,2)+pt_2d(id_ol,2);
        H2 = pt_2d(ig,2)-pt_2d(id_ol,2);
        dx = abs(pt_2d(ig,1)-pt_2d(id_ol,1));
        
        D = sqrt( H1.^2 + dx.^2 );
        d = max( sqrt(H2.^2 + dx.^2), re(id_ol));

        int_tmp = zeros(Nol,Nfrq);
        for ik = 1:Nol
            int_tmp(ik,:) = integral(@(lamda) ...
                ( exp(-H1(ik)*lamda).*cos(lamda.*dx(ik)) ...
                ./ (lamda+sqrt(lamda.^2+rg.^2))) ,0,Inf,'ArrayValued',true);
        end
        
        wm_tmp = ones(Nol,1)*wm;
        Ztmp = 1j*mu0/(2*pi)*wm_tmp.* ( log(D./d)*ones(1,Nfrq) ...
            + 2*int_tmp );
        
        Zg(ik,id_ol,1:Nfrq) = Ztmp;
        
        Rmg(ig,id_ol,1:Nfrq) = real(Ztmp)*len;
        Lmg(ig,id_ol,1:Nfrq) = imag(Ztmp)./wm_tmp*len;

        Rsg(ig,1:Nfrq) = Rmg(ig,ig,1:Nfrq);
        Lsg(ig,1:Nfrq) = Lmg(ig,ig,1:Nfrq);
    end
end



end


