function [Rsg, Lsg, Zsg] = zg_tline_log_ol_b2(pt_2d,re, len, ...
    sig_g1,epr_g1,dg1, sig_g2,epr_g2, frq)
%  Function:       zg_tline_log_ol_b2
%  Description:    Calculate self earth rerurn impedance for overhead
%                  lines using log approximation.
%                  b1 means 2 layer ground.
%                  Sunde's model in paper "Comparison of Image and 
%                  Transmission Line Models of Energized Horizontal Wire 
%                  Above Tow-Layer Soil"
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
Nf = length(frq);

Zsg = zeros(Nc,Nf);

rg1 = sqrt( 1j.*wm.*mu0.*(sig_g1 + 1j.*wm.*ep0*epr_g1) );
rg2 = sqrt( 1j.*wm.*mu0.*(sig_g2 + 1j.*wm.*ep0*epr_g2) );
rg_eq = rg1.* ( (rg1+rg2)-(rg1-rg2).*exp(-2*rg1.*dg1) ) ./ ...
    ( (rg1+rg2)+(rg1-rg2).*exp(-2*rg1.*dg1) );


id_ol = find(pt_2d(:,2)>0);
Nol = length(id_ol);

wm_tmp = ones(Nol,1)*wm;
rg_tmp = ones(Nol,1)*rg_eq;

Zsg(id_ol,1:Nf) = 1j*mu0/(2*pi)*wm_tmp.* ( log(2*pt_2d(id_ol,2)./re(id_ol))*ones(1,Nf) + ...
    log( 1 + 1./(rg_tmp.*(pt_2d(id_ol,2)*ones(1,Nf)))) );
        

Rsg(id_ol,1:Nf) = real(Zsg(id_ol,1:Nf))*len;
Lsg(id_ol,1:Nf) = imag(Zsg(id_ol,1:Nf))./wm_tmp*len;


end


