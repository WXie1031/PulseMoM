function [Rsg,Lsg,Zg, Gsg,Csg,Yg] = grid_tline_h(depth, re, sig, mur, sig_soil,epr_soil, frq)
%  Function:       grid_tline_h
%  Description:    Calculate mutual earth rerurn impedance for underground
%                  conductor using exact formula with numerical integration.
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

Nc = size(depth,1);
Nf = length(frq);

Rsg = zeros(Nc,Nf);
Lsg = zeros(Nc,Nf);
Zg = zeros(Nc,Nf);
Gsg = zeros(Nc,Nf);
Csg = zeros(Nc,Nf);
Yg = zeros(Nc,Nf);


kg = sig_soil + 1j.*wm.*ep0*epr_soil;
rg = sqrt( 1j.*wm.*mu0.*kg );

for ik = 1:Nc
    
    [R, Lin] = zin_cir_ac(re(ik), 0, sig(ik), mur(ik), 1, frq);
    Zin = R+1j*wm.*Lin;
    
    D = sqrt(2*re(ik)*depth(ik));
    
    for ig = 1:Nf
        K = fminsearch(@(K) grid_tline_h_sub(Zin(ig), D, kg(ig), rg(ig), wm(ig), K),...
            [1e-4+1e-8i*wm(ig)], ...
            optimset('TolFun',1e-8,'MaxIter',10000,'MaxFunEvals',10000));
        
        Zg(ik,ig) = Zin(ig) + 1j*wm(ig)*mu0/(2*pi).* log(1.85./(sqrt(K.^2+rg(ik).^2).*D) );
        Yg(ik,ig) = pi*kg(ig) / log(1.12/(K*D));
    end
    
    %Zc(ik,:) = sqrt(Zg(ik,:)./Yg(ik,:));
    
    Rsg(ik,1:Nf) = real(Zg(ik,:));
    Lsg(ik,1:Nf) = imag(Zg(ik,:))./wm;
    
    Gsg(ik,1:Nf) = real(Yg(ik,:));
    Csg(ik,1:Nf) = imag(Yg(ik,:))./wm;
    
    for ig = 2:Nf
        Lsg(ik,ig) = min(0.95*Lsg(ik,ig-1), Lsg(ik,ig));
    %    Gsg(ik,ig) = min(0.95*Gsg(ik,ig-1), Gsg(ik,ig));
    end
    
%     for ig = Nf:-1:2
%         Csg(ik,ig-1) = min(0.95*Csg(ik,ig), Csg(ik,ig-1));
%     end
end


end


