function [Rmg,Lmg, Zg] = zg_tline_ou_b1(Rmg, Lmg, pt_2d,re, len, ...
    sig_soil, epr_soil, frq)
%  Function:       zg_tline_ou_b1
%  Description:    Calculate mutual earth rerurn impedance between underground
%                  conductor and overhead line using exact formula with
%                  numerical integration.
%                  b1 means 1 layer ground.
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
    Rmg = zeros(Nc,Nc,Nfrq);
end
if isempty(Lmg)
    Lmg = zeros(Nc,Nc,Nfrq);
end
Zg = zeros(Nc,Nc,Nfrq);

rg = sqrt( 1j.*wm.*mu0.*(sig_soil + 1j.*wm.*ep0*epr_soil) );

for ig = 1:Nc
    
    if pt_2d(ig,2)>0
        id_ou = find(pt_2d(:,2)<0);
        
        Ho = abs(pt_2d(ig,2));      % overhead line depth
        Hu = abs(pt_2d(id_ou,2));   % underground conductor depth
        
        Nou = length(id_ou);
        Ho = Ho*ones(Nou,1);
    elseif pt_2d(ig,2)<0
        id_ou = find(pt_2d(:,2)>0);
        
        Ho = abs(pt_2d(id_ou,2));   % overhead line depth
        Hu = abs(pt_2d(ig,2));      % underground conductor depth
        
        Nou = length(id_ou);
        Hu = Hu*ones(Nou,1);
    end
    
    dx = abs(pt_2d(ig,1)-pt_2d(id_ou,1));
    
    int_tmp = zeros(Nou,Nfrq);
    for ik = 1:Nou
        int_tmp(ik,:) = integral(@(lamda) ... 
            ( exp(-Hu(ik)*sqrt(lamda.^2+rg.^2)-Ho(ik)*lamda).*cos(lamda.*dx(ik)) ...
            ./ (lamda+sqrt(lamda.^2+rg.^2)) ) ,0,Inf,'ArrayValued',true);
    end
    
    wm_tmp = ones(Nou,1)*wm;
    Ztmp = 1j*mu0/pi*wm_tmp.* int_tmp;
    
    Zg(ik,id_ou,1:Nfrq) = Ztmp;
    
    Rmg(ig,id_ou,1:Nfrq) = real(Ztmp)*len;
    Lmg(ig,id_ou,1:Nfrq) = imag(Ztmp)./wm_tmp*len;
    
end



end


