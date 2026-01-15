function [Rmg,Lmg, Zg, Rsg,Lsg] = zg_tline_ol_b2(Rmg, Lmg, pt_2d,re, len, ...
    sig_g1,epr_g1,dg1, sig_g2,epr_g2, frq)
%  Function:       zg_tline_ol_b2
%  Description:    Calculate mutual earth rerurn impedance for overhead
%                  lines using exact formula with numerical integration.
%                  b2 means 2 layer ground.
%                  Sunde's model in paper "Homogenous Earth Approximation 
%                  of Two-Layer Earth Stuctures: An Equivalent Resistivity 
%                  Approach".  Change the sig in the paper with rg
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

rg1 = sqrt( 1j.*wm.*mu0.*(sig_g1 + 1j.*wm.*ep0*epr_g1) );
rg2 = sqrt( 1j.*wm.*mu0.*(sig_g2 + 1j.*wm.*ep0*epr_g2) );
rg_eq = rg1.* ( (rg1+rg2)-(rg1-rg2).*exp(-2*rg1.*dg1) ) ./ ...
    ( (rg1+rg2)+(rg1-rg2).*exp(-2*rg1.*dg1) );


for ig = 1:Nc
    
    if pt_2d(ig,2)>0
        id_ol = find(pt_2d(:,2)>=0);
        Nol = length(id_ol);
        
        H1 = pt_2d(ig,2) + pt_2d(id_ol,2);
        H2 = pt_2d(ig,2) - pt_2d(id_ol,2);
        dx = abs(pt_2d(ig,1)-pt_2d(id_ol,1));
        
        D = sqrt( H1.^2 + dx.^2 );
        d = max( sqrt(H2.^2 + dx.^2), re(id_ol) );
        
        int_tmp = zeros(Nol,Nfrq);
        for ik = 1:Nol
            int_tmp(ik,:) = integral(@(lamda) ...
                (exp( -H1(ik)*lamda ) .* cos(lamda.*dx(ik)) ...
                ./ (lamda+sqrt(lamda.^2+rg_eq.^2))) ,0,Inf,'ArrayValued',true);
        end
        
        wm_tmp = ones(Nol,1)*wm;
        Ztmp = 1j*mu0/(2*pi)*wm_tmp.* ( log(D./d)*ones(1,Nfrq) ...
            + 2*int_tmp );
        
        Zg(ik,id_ol,1:Nfrq) = Ztmp;
        
        Rmg(ig,id_ol,1:Nfrq) = real(Ztmp(id_ol,1:Nfrq))*len;
        Lmg(ig,id_ol,1:Nfrq) = imag(Ztmp(id_ol,1:Nfrq))./wm_tmp*len;
        
        Rsg(ig,1:Nfrq) = Rmg(ig,ig,1:Nfrq);
        Lsg(ig,1:Nfrq) = Lmg(ig,ig,1:Nfrq);
    end
end



end


