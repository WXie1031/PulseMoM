function [Rgnd, Lgnd] = grid_self_Sunde_h(Rgnd, Lgnd, pt_start_grid, pt_end_grid, ...
    dv_grid, re_grid, len_grid, sig_soil, epr_soil, frq)
%  Function:       grid_self_Sunde_h
%  Description:    Calculate self R, L of horizontal grid conductors using
%                  Sunde's model in paper "External Impedance and Admittance
%                  of Burid Horizontal Wires for Transient Studies Using
%                  Transmission Line Analysis"
%  Calls:
%
%  Input:          pt_start  --  start point of conductors (N*3) (m)
%                  pt_end    --  end point of conductors (N*3) (m)
%                  dv        --  direction vector of conductors (N*3)
%                  re        --  equivalent radius (N*1)
%                  Rin_pul   --  resistance of conductors (N*1) (ohm/m)
%                  Lin_pul   --  internal L of conductors (N*1) (H/m)
%                  len       --  length of conductors (N*1)
%                  sig_soil  --  soil conductivity (S/m)
%                  ep_soil   --  relative permittivity of soil
%                  frq       --  frequency squeese
%  Output:         Rgrid --  R matrix
%                  Lgrid --  L matrix
%  Others:
%  Author:         Chen Hongcai
%  Email :         hc.chen@live.com
%  Date:           2016-02-07

mu0 = 4*pi*1e-7;
ep0 = 8.85e-12;

ep_soil = ep0*epr_soil;
wm = 2*pi*frq;

Nc = size(dv_grid,1);
Nf = length(frq);
if isempty(Rgnd)
    Rgnd=zeros(Nc,Nf);
end
if isempty(Lgnd)
    Lgnd=zeros(Nc,Nf);
end

z_axis = [0 0 1];
z_axis = ones(Nc,1)*z_axis;
id_grd_h = find(abs(dot(dv_grid,z_axis,2))<1e-6);
Nch = length(id_grd_h);

if Nch>0
    depth = abs(pt_end_grid(id_grd_h,3));
    
    rg = sqrt( 1j.*wm.*mu0.*(sig_soil + 1j.*wm.*ep_soil) );
    
    % % gauss numerical integration coefficients
    % [TT, AA] = gauss_int_coef(n);
    % a = (a2+a1)/2*ones(1,n) + (a2-a1)/2*TT;
    %
    % delta = (a2-a1)*(r2.*r2-r1.*r1)/2;
    
    % I2int = l .* (a2-a1)/2 .* sum(AA.*(I22-I12),2);
    
    int_tmp = zeros(Nch,Nf);
    for ik = 1:Nch
        int_tmp(ik,:) = integral(@(lamda) ...
            (exp(-2*depth(ik)*sqrt(lamda.^2+rg.^2)).*cos(lamda.*re_grid(id_grd_h(ik))) ./ (lamda+sqrt(lamda.^2+rg.^2))) ...
            ,0,Inf,'ArrayValued',true);
    end
    
    wm_tmp = ones(Nch,1)*wm;
    Zg = 1j*mu0/(2*pi)*wm_tmp.* ( besselk(0,re_grid(id_grd_h)*rg) ...
        - besselk(0,sqrt(re_grid(id_grd_h).^2+4*depth.^2)*rg) ...
        + 2*int_tmp );
    
    for ik = 1:Nch
        Rgnd(id_grd_h(ik),1:Nf) = real(Zg(ik,:))*len_grid(id_grd_h(ik));
        Lgnd(id_grd_h(ik),1:Nf) = imag(Zg(ik,:))./wm*len_grid(id_grd_h(ik));
    end

end



end

