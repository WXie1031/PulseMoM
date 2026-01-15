function [Rgrid, Lgrid] = grid_self_Wait_h(Rgrid, Lgrid, pt_start_grid, pt_end_grid, ...
    dv_grid, re_grid, len_grid, sig_soil, epr_soil, frq)
%  Function:       grid_self_Wait_h
%  Description:    Calculate self R, L of horizontal grid conductors using
%                  Wait's model in paper "External Impedance and Admittance
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
if isempty(Rgrid)
    Rgrid=zeros(Nc,Nf);
end
if isempty(Lgrid)
    Lgrid=zeros(Nc,Nf);
end

z_axis = [0 0 1];
z_axis = ones(Nc,1)*z_axis;
id_grd_h = find(abs(dot(dv_grid,z_axis,2))<1e-6);
Nch = length(id_grd_h);

depth = abs(pt_end_grid(id_grd_h,3));

k_cof = sqrt(ep_soil*mu0*wm.^2 - 1j.*wm*mu0*sig_soil);
d_cof = 2*1j*depth*k_cof;

delta_cof = 1./(besselk(0,1j*re_grid(id_grd_h)*k_cof)) .* ...
    ( besselk(0,d_cof) + 2./d_cof.*besselk(1,d_cof) ...
    - 2./(d_cof.^2).*(1+d_cof).*exp(-d_cof) );

wm_tmp = ones(Nch,1)*wm;
Zg = 1j*mu0/(2*pi)*wm_tmp.*(1+delta_cof).*log(-1j*1.12./(re_grid(id_grd_h)*k_cof));


for ik = 1:Nch
    Rgrid(id_grd_h(ik),1:Nf) = real(Zg(ik,:))*len_grid(id_grd_h(ik));
    Lgrid(id_grd_h(ik),1:Nf) = imag(Zg(ik,:))./wm*len_grid(id_grd_h(ik));
end


end


