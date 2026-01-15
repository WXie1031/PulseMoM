function [Gxx,Gzx,Gzz,Gphi] = dgf_main(pt_s,dv_s, pt_f,dv_f, ...
    zbdy, epr_lyr,mur_lyr,sig_lyr, w0)
%  Function:       dgf_main
%  Description:
%
%                  left medium | L1 | L2 | ... | LM | right medium
%                    interface 1    2    3     M   M+1
%
%                  The left and the right medium is related with the
%                  direction of the waves. If the wave is in the right
%                  direction, 'left medium' is the first layer, and the
%                  'right medium' is the last medium. In this situation,
%                  the relection coefficient of the 'right medium' is 0.
%  Calls:
%
%  Input:          pt_s    --   source point Nx3
%                  pt_f    --   field point  Nx3
%                  dv_s    --   direction vector of source points Nx3
%                  dv_f    --   direction vector of field points Nx3
%                  zbdy    --   the z-axis of the boundaries (Nbdyx1)
%                  epr_lyr --   relative permittivity of each layer (Nlyrx1)
%                  mur_lyr --   relative permeability of each layer (Nlyrx1)
%                  sig_lyr --   conductivity of each layer (Nlyrx1)
%                  w0      --   frequency
%
%  Output:         Rcof  --  reflection coefficients (N-1)x1
%
%  Others:
%  Author:         Chen Hongcai
%  Email :         hc.chen@live.com
%  Date:           2017-03-27


flag_sol = 1;
flag_int_type = 1;  % choose integration path
Nint = 15;  % order of the Guassion integration


%% media parameters of layers
vc = 3e8;
mu0 = 4e-7*pi;
ep0 = 8.854187817e-12;

mu_lyr = mu0*mur_lyr;
% Convert relative permittivity to complex permittivity (including conductivity)
epr_lyr_complex = epr_lyr - 1j.*sig_lyr./(w0*ep0);
ep_lyr = ep0*epr_lyr_complex;

kn = w0*sqrt(ep_lyr.*mu_lyr);
k0 = w0*sqrt(ep0.*mu0);


Nbdy = size(zbdy,1);
Nlyr = Nbdy+1;
d_lyr = [1e6; zbdy(1:Nbdy-1)-zbdy(2:Nbdy); 1e6;];


%% kp setting and integration path
id_kn = sig_lyr<1e4;
kp_max = min( 1.2*max(abs(kn(id_kn))), max(abs(kn(id_kn)))+k0 );

% ppw = 40;
ppw = 10;
lumda = 2*pi*vc/w0;
dkp = min(pi/(lumda*ppw),kp_max/512);
% dkp=20;  % Commented out: hard-coded value overrides calculated dkp


kp_end = min(1e6,1e5*dkp);


kp = 0:dkp:kp_end;
Nkp = length(kp);

kp_im = zeros(1,Nkp);
% Elliptical integration path to avoid branch cuts
%kp_max = max(abs(kn(id_kn)))+k0*0.2 ;

Nkp_imp = size(1:dkp:min(kp_end,kp_max),2);
a = kp_max/2;
b = 1e-3*kp_max;

% Improved elliptical path: use proper ellipse equation
% Original commented code: kp_im(1:Nkp_imp) = b .* sqrt(1-((1:dkp:min(kp_end,kp_max))-a).^2./a.^2);
% Previous sin-based approach creates discontinuity. Use proper ellipse instead.
if Nkp_imp > 1
    % Create kp values for the elliptical segment
    kp_real_seg = (0:dkp:min(kp_end,kp_max));
    kp_real_seg = kp_real_seg(1:min(Nkp_imp, length(kp_real_seg)));
    Nkp_imp_actual = length(kp_real_seg);
    
    % Ellipse: (kp_real - a)^2/a^2 + kp_im^2/b^2 = 1
    % kp_im = b * sqrt(1 - ((kp_real - a)/a)^2)
    if Nkp_imp_actual > 1
        kp_im_seg = b .* sqrt(max(0, 1 - ((kp_real_seg - a)./a).^2));
        kp_im(1:Nkp_imp_actual) = kp_im_seg;
        % Ensure smooth transition: start and end at real axis
        kp_im(1) = 0;
        if kp_real_seg(end) <= kp_max
            kp_im(Nkp_imp_actual) = 0;
        end
    else
        % Fallback for single point
        kp_im(1) = 0;
    end
else
    % Fallback to sin if Nkp_imp <= 1
    kp_im(1:Nkp_imp) = b .* sin(pi*(0:Nkp_imp-1)/max(1,Nkp_imp-1));
end
kp = kp+1j*kp_im;



%% integration of source(point)-field(vector)
% calculate the reflection coefficients before the integral.
% this part is has no relation with z that it will be calculated onece.

[gau_nod, gau_wei] = gauss_int_coef(Nint);

Nkp_tot = (Nkp-1)*Nint;
kp_tot = zeros(1,Nkp_tot);
% calculate the kz
for ig = 1:Nkp-1
    dkp_int = (kp(ig+1)-kp(ig))/2*gau_nod;
    kp_tot((ig-1)*Nint+(1:Nint)) = dkp_int + (kp(ig)+kp(ig+1))/2;
end
kz_tot = sqrt(repmat(kn.^2,1,Nkp_tot) - repmat(kp_tot.^2,Nlyr,1));
% Improved branch selection: ensure Im(kz) < 0 for proper wave decay
% This is more accurate than using angle, especially for lossy media
id_kz = imag(kz_tot) > 0;
kz_tot(id_kz) = -kz_tot(id_kz);


GRe_ij = zeros(Nbdy,Nkp_tot);
GRh_ij = zeros(Nbdy,Nkp_tot);
GRe_ji = zeros(Nbdy,Nkp_tot);
GRh_ji = zeros(Nbdy,Nkp_tot);
De = zeros(Nlyr,Nkp_tot);
Dh = zeros(Nlyr,Nkp_tot);

% characteristic impedance of the transmission line
Ze = zeros(Nlyr,Nkp_tot);
Zh = zeros(Nlyr,Nkp_tot);
for ih = 1:Nlyr
    Ze(ih,1:Nkp_tot) = kz_tot(ih,1:Nkp_tot)./(w0*ep_lyr(ih));
    Zh(ih,1:Nkp_tot) = (w0*mu_lyr(ih))./kz_tot(ih,1:Nkp_tot);
end

% reflection coefficients of each layer (all layers are calculated)
Re_ij = (Ze(1:Nlyr-1,1:Nkp_tot)-Ze(2:Nlyr,1:Nkp_tot))./(Ze(2:Nlyr,1:Nkp_tot)+Ze(1:Nlyr-1,1:Nkp_tot));
Re_ji = -Re_ij;

Rh_ij = (Zh(1:Nlyr-1,1:Nkp_tot)-Zh(2:Nlyr,1:Nkp_tot))./(Zh(2:Nlyr,1:Nkp_tot)+Zh(1:Nlyr-1,1:Nkp_tot));
Rh_ji = -Rh_ij;

% Ze_inf = -j*kp/(w0*ep_lyr), while, the -j*kp/w0 is cancelled in Rij_inf
Ze_inf = 1./ep_lyr;
Zh_inf = mur_lyr;
Re_ij_inf = (Ze_inf(1:Nlyr-1)-Ze_inf(2:Nlyr))./(Ze_inf(2:Nlyr)+Ze_inf(1:Nlyr-1));
Re_ji_inf = -Re_ij_inf;
Rh_ij_inf = (Zh_inf(1:Nlyr-1)-Zh_inf(2:Nlyr))./(Zh_inf(2:Nlyr)+Zh_inf(1:Nlyr-1));
Rh_ji_inf = -Rh_ij_inf;

% Re_ij_inf = Re_ij(:,end);
% Re_ji_inf = Re_ji(:,end);
% Rh_ij_inf = Rh_ij(:,end);
% Rh_ji_inf = Rh_ji(:,end);

% forward layer recursion
GRe_ij(Nbdy,1:Nkp_tot) = Re_ij(Nbdy,1:Nkp_tot);
GRh_ij(Nbdy,1:Nkp_tot) = Rh_ij(Nbdy,1:Nkp_tot);
for ih = Nbdy-1:-1:1
    GRe_ij(ih,1:Nkp_tot) = dgf_bdy_gcof_e(Re_ij(ih,1:Nkp_tot),GRe_ij(ih+1,1:Nkp_tot), kz_tot(ih+1,1:Nkp_tot), d_lyr(ih+1) );
    GRh_ij(ih,1:Nkp_tot) = dgf_bdy_gcof_e(Rh_ij(ih,1:Nkp_tot),GRh_ij(ih+1,1:Nkp_tot), kz_tot(ih+1,1:Nkp_tot), d_lyr(ih+1) );
end

% backward layer recursion
GRe_ji(1,1:Nkp_tot) = Re_ji(1,1:Nkp_tot);
GRh_ji(1,1:Nkp_tot) = Rh_ji(1,1:Nkp_tot);
for ih = 2:Nbdy
    GRe_ji(ih,1:Nkp_tot) = dgf_bdy_gcof_e(Re_ji(ih,1:Nkp_tot),GRe_ji(ih-1,1:Nkp_tot), kz_tot(ih,1:Nkp_tot), d_lyr(ih) );
    GRh_ji(ih,1:Nkp_tot) = dgf_bdy_gcof_e(Rh_ji(ih,1:Nkp_tot),GRh_ji(ih-1,1:Nkp_tot), kz_tot(ih,1:Nkp_tot), d_lyr(ih) );
end

% d_lyr(1) and d_lyr(Nlyr) are infinite due to on boundary exsit
% here a very large thickness 1e6 is used to represent that.
% Meanwhile, set De(1,1:Np) = 1; and De(Nlyr,1:Np)=1;
De(1,1:Nkp_tot) = 1;
De(Nlyr,1:Nkp_tot) = 1;
De(2:Nlyr-1,1:Nkp_tot) = 1 - GRe_ij(2:Nbdy,1:Nkp_tot).*GRe_ji(1:Nbdy-1,1:Nkp_tot)...
    .*exp(-2*1j*kz_tot(2:Nlyr-1,1:Nkp_tot).*repmat(d_lyr(2:Nlyr-1),1,Nkp_tot));
id_inf = isinf(De) | (isnan(De));
De(id_inf) = 1;

% d_lyr(1) is infinite due to on boundary exsit on the left
% d_lyr(Nlyr) is also infinite due to no boundary exsit on the right
Dh(1,1:Nkp_tot) = 1;
Dh(Nlyr,1:Nkp_tot) = 1;
Dh(2:Nlyr-1,1:Nkp_tot) = 1 - GRh_ij(2:Nbdy,1:Nkp_tot).*GRh_ji(1:Nbdy-1,1:Nkp_tot)...
    .*exp(-2*1j*kz_tot(2:Nlyr-1,1:Nkp_tot).*repmat(d_lyr(2:Nlyr-1),1,Nkp_tot));
id_inf = isinf(Dh) | (isnan(Dh)) ;
Dh(id_inf) = 1;


%% calculate the integration
zs = pt_s(:,3);
zf = pt_f(:,3);
Ns = size(pt_s,1);
Nf = size(pt_f,1);
id_f = dgf_locate_lyr(zf(1:Nf), zbdy);

Gxx = zeros(Ns,Nf);
Gzx = zeros(Ns,Nf);
Gzz = zeros(Ns,Nf);
Gphi = zeros(Ns,Nf);
for ik = 1:Ns
    
    id_s = dgf_locate_lyr(zs(ik), zbdy);
    
    
    dz1 = zeros(Nf,1);
    dz2 = zeros(Nf,1);
    dz3 = zeros(Nf,1);
    dz4 = zeros(Nf,1);
    
    indxx = dv_s(ik,:);
    indzx = dv_s(ik,:);
    indzz = dv_s(ik,:);
    indphi = dv_s(ik,:);
    indxx=1:Nf;
    indzx=1:Nf;
    indzz=1:Nf;
    indphi=1:Nf;
    
    dz0 = abs(zs(ik)-zf(1:Nf));
    
    % source and field in the same layer
    idf_same = (id_f==id_s);

    
    % field in the adjcent front layer of source (id_f = id_s - 1)
    idf_adj_fro = (id_f+1==id_s);
    Ntmp = sum(idf_adj_fro);
    if Ntmp>0
    % Use actual field point z coordinates, not boundary values
    zf_adj_fro = zf(idf_adj_fro);
    
    dz1(idf_adj_fro) = 2*zbdy(id_s-1) - (zf_adj_fro+zs(ik));
    dz2(idf_adj_fro) = (zf_adj_fro+zs(ik)) - 2*zbdy(id_s);
    dz3(idf_adj_fro) = 2*d_lyr(id_s) + (zf_adj_fro-zs(ik));
    dz4(idf_adj_fro) = 2*d_lyr(id_s) - (zf_adj_fro-zs(ik));
    end
    
    % field in the adjcent back layer of source (id_f = id_s + 1)
    idf_adj_bak = (id_f-1==id_s);
    Ntmp = sum(idf_adj_bak);
    if Ntmp>0
    % Use actual field point z coordinates, not boundary values
    zf_adj_bak = zf(idf_adj_bak);
    
    dz1(idf_adj_bak) = 2*zbdy(id_s-1) - (zf_adj_bak+zs(ik));
    dz2(idf_adj_bak) = (zf_adj_bak+zs(ik)) - 2*zbdy(id_s);
    dz3(idf_adj_bak) = 2*d_lyr(id_s) + (zf_adj_bak-zs(ik));
    dz4(idf_adj_bak) = 2*d_lyr(id_s) - (zf_adj_bak-zs(ik));
    end
    
    % field in the front layer of source (m<n, excluding adjacent layer)
    idf_oth_fro = (id_f<id_s) & ~idf_adj_fro;
    Ntmp = sum(idf_oth_fro);
    if Ntmp>0
    zf_tmp = repmat(zbdy(id_s-1),Ntmp,1);
    dz0(idf_oth_fro) = abs(zs(ik)-zf_tmp);
    
    dz1(idf_oth_fro) = 2*zbdy(id_s-1) - (zf_tmp+zs(ik));
    dz2(idf_oth_fro) = (zf_tmp+zs(ik)) - 2*zbdy(id_s);
    dz3(idf_oth_fro) = 2*d_lyr(id_s) + (zf_tmp-zs(ik));
    dz4(idf_oth_fro) = 2*d_lyr(id_s) - (zf_tmp-zs(ik));
    end
    
    % field in the back layer of source (m>n, excluding adjacent layer)
    idf_oth_bak = (id_f>id_s) & ~idf_adj_bak;
    Ntmp = sum(idf_oth_bak);
    if Ntmp>0
    zf_tmp = repmat(zbdy(id_s),Ntmp,1);
    dz0(idf_oth_bak) = abs(zs(ik)-zf_tmp);
    
    dz1(idf_oth_bak) = 2*zbdy(id_s-1) - (zf_tmp+zs(ik));
    dz2(idf_oth_bak) = (zf_tmp+zs(ik)) - 2*zbdy(id_s);
    dz3(idf_oth_bak) = 2*d_lyr(id_s) + (zf_tmp-zs(ik));
    dz4(idf_oth_bak) = 2*d_lyr(id_s) - (zf_tmp-zs(ik));
    end
%     
    
    % Process same layer case
    if any(idf_same)
        if flag_sol == 1
            [Gxx(ik,idf_same),Gzx(ik,idf_same),Gzz(ik,idf_same),Gphi(ik,idf_same)] = ...
                dgf_main_sub_same(Zh,Dh,GRh_ij,GRh_ji, Ze,De,GRe_ij,GRe_ji, ...
                Rh_ij_inf,Rh_ji_inf,Re_ij_inf,Re_ji_inf, kz_tot,kp_tot, kp,dkp, ...
                zbdy, d_lyr, epr_lyr_complex,mur_lyr, pt_s,dv_s, pt_f(idf_same,1:3),dv_f(idf_same,1:3),...
                gau_nod,gau_wei, w0);
        elseif flag_sol == 2
            [Gxx(ik,idf_same),Gzx(ik,idf_same),Gzz(ik,idf_same),Gphi(ik,idf_same)] = ...
                dgf_main_num_same(Zh,Dh,GRh_ij,GRh_ji, Ze,De,GRe_ij,GRe_ji, kz_tot,kp_tot, kp,dkp, ...
                zbdy, epr_lyr_complex,mur_lyr, pt_s,dv_s, pt_f(idf_same,1:3),dv_f(idf_same,1:3),...
                dz0(idf_same),dz1(idf_same),dz2(idf_same),dz3(idf_same),dz4(idf_same), ...
                gau_nod,gau_wei, w0);
        end
    end
    
    % Process adjacent front layer case (field point in layer id_s-1)
    if any(idf_adj_fro)
        [Gxx(ik,idf_adj_fro),Gzx(ik,idf_adj_fro),Gzz(ik,idf_adj_fro),Gphi(ik,idf_adj_fro)] = ...
            dgf_main_sub_diff_fro(Zh,Dh,GRh_ij,GRh_ji, Ze,De,GRe_ij,GRe_ji, ...
            kz_tot,kp_tot, kp,dkp, zbdy, d_lyr, epr_lyr_complex,mur_lyr, ...
            pt_s,dv_s, pt_f(idf_adj_fro,1:3),dv_f(idf_adj_fro,1:3), ...
            dz0(idf_adj_fro),dz1(idf_adj_fro),dz2(idf_adj_fro),dz3(idf_adj_fro),dz4(idf_adj_fro),...
            gau_nod,gau_wei, w0);
    end
    
    % Process adjacent back layer case (field point in layer id_s+1)
    if any(idf_adj_bak)
        [Gxx(ik,idf_adj_bak),Gzx(ik,idf_adj_bak),Gzz(ik,idf_adj_bak),Gphi(ik,idf_adj_bak)] = ...
            dgf_main_sub_diff_bak(Zh,Dh,GRh_ij,GRh_ji, Ze,De,GRe_ij,GRe_ji, ...
            kz_tot,kp_tot, kp,dkp, zbdy, d_lyr, epr_lyr_complex,mur_lyr, ...
            pt_s,dv_s, pt_f(idf_adj_bak,1:3),dv_f(idf_adj_bak,1:3), ...
            dz0(idf_adj_bak),dz1(idf_adj_bak),dz2(idf_adj_bak),dz3(idf_adj_bak),dz4(idf_adj_bak),...
            gau_nod,gau_wei, w0);
    end
    
    % Process other front layers (field point in layers < id_s-1)
    if any(idf_oth_fro)
        [Gxx(ik,idf_oth_fro),Gzx(ik,idf_oth_fro),Gzz(ik,idf_oth_fro),Gphi(ik,idf_oth_fro)] = ...
            dgf_main_sub_diff_fro(Zh,Dh,GRh_ij,GRh_ji, Ze,De,GRe_ij,GRe_ji, ...
            kz_tot,kp_tot, kp,dkp, zbdy, d_lyr, epr_lyr_complex,mur_lyr, ...
            pt_s,dv_s, pt_f(idf_oth_fro,1:3),dv_f(idf_oth_fro,1:3), ...
            dz0(idf_oth_fro),dz1(idf_oth_fro),dz2(idf_oth_fro),dz3(idf_oth_fro),dz4(idf_oth_fro),...
            gau_nod,gau_wei, w0);
    end
    
    % Process other back layers (field point in layers > id_s+1)
    if any(idf_oth_bak)
        [Gxx(ik,idf_oth_bak),Gzx(ik,idf_oth_bak),Gzz(ik,idf_oth_bak),Gphi(ik,idf_oth_bak)] = ...
            dgf_main_sub_diff_bak(Zh,Dh,GRh_ij,GRh_ji, Ze,De,GRe_ij,GRe_ji, ...
            kz_tot,kp_tot, kp,dkp, zbdy, d_lyr, epr_lyr_complex,mur_lyr, ...
            pt_s,dv_s, pt_f(idf_oth_bak,1:3),dv_f(idf_oth_bak,1:3), ...
            dz0(idf_oth_bak),dz1(idf_oth_bak),dz2(idf_oth_bak),dz3(idf_oth_bak),dz4(idf_oth_bak),...
            gau_nod,gau_wei, w0);
    end
    
end



end


