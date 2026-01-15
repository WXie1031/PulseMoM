function [Gxx,Gzx,Gzz,Gphi] = dgf_main_sub_same_sub_n ...
    (pt_s,dv_s, pt_f,dv_f, zbdy, epr_lyr,mur_lyr,sig_lyr, w0)
%  Function:       dgf_main_sub_same_sub_i
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
%                  para_lyr --  parameter of each layer (Nx1)
%                  kz   --  wave number
%
%  Output:         Rcof  --  reflection coefficients (N-1)x1
%
%  Others:
%  Author:         Chen Hongcai
%  Email :         hc.chen@live.com
%  Date:           2017-03-27


flag_int_type = 1;  % choose integration path
Nint = 7;  % order of the Guassion integration

err_tol = 1e-10;

%% media parameters of layers
vc = 3e8;
mu0 = 4e-7*pi;
ep0 = 8.854187817e-12;

mu_lyr = mu0*mur_lyr;
ep_lyr = ep0*epr_lyr - 1j.*sig_lyr./w0;

kn = w0*sqrt(ep_lyr.*mu_lyr);
k0 = w0*sqrt(ep0.*mu0);


Nbdy = size(zbdy,1);
Nlyr = Nbdy+1;
d_lyr = [1e6; zbdy(1:Nbdy-1)-zbdy(2:Nbdy); 1e6;];


zs = pt_s(:,3);
zf = pt_f(:,3);
Ns = size(pt_s,1);
Nf = size(pt_f,1);
id_lf = dgf_locate_lyr(zf(1:Nf), zbdy);
mur_lyr_f = ones(Nf,1);
epr_lyr_f = ones(Nf,1);


%% kp setting and integration path
% ppw = 40;
ppw = 20;
lumda = 2*pi*vc/w0;
dkp = pi/(lumda*ppw);

if flag_int_type == 2 % optimal integration path proposed in Lambot's paper
    d = 1e-16; % is defined using the damping fctor of the exponential term
    kp_end = max(sqrt(((-log(d))./(2*d_lyr)).^2+(w0/vc).^2));
else
    kp_end = 1e6;
end



kp = 0:dkp:kp_end;
Nkp = length(kp);

kp_im = zeros(1,Nkp);
if flag_int_type == 1 % Elliptical integration path
    id_kn = sig_lyr<1e4;
    %kp_max = max( 1.2*max(abs(kn(id_kn))), max(abs(kn(id_kn)))+k0 );
    kp_max = max(abs(kn(id_kn)))+k0*0.2 ;
    Nkp_imp = size(1:dkp:min(kp_end,kp_max),2);
    a = kp_max/2;
    b = 1e-3*a;
    
    %kp_im(1:Nkp_imp) = b .* sqrt(1-((1:dkp:min(kp_end,kp_max))-kp_max/2).^2./a.^2);
    kp_im(1:Nkp_imp) = b .* sin(pi*(1:Nkp_imp)/Nkp_imp);
    kp = kp+1j*kp_im;
    
elseif flag_int_type == 2 % optimal integration path proposed in Lambot's paper
    kp_im = kp./sqrt((kp*vc/w0).^2+1);
    kp = kp+1j*kp_im;
end



%% integration of source(point)-field(vector)
% adaptive Gauss-Kronrod quadrature based on a Gauss-Kronrod pair (15th and 7th order formulas).
[gau_nod, gau_wei]=gauss_int_coef(Nint);

Nkp_int = length(gau_nod);

% calculate the reflection coefficients before the integral.
% this part is has no relation with z that it will be calculated onece.
Nkp_tot = (Nkp-1)*Nkp_int;

GRe_ij = zeros(Nbdy,Nkp_tot);
GRh_ij = zeros(Nbdy,Nkp_tot);
GRe_ji = zeros(Nbdy,Nkp_tot);
GRh_ji = zeros(Nbdy,Nkp_tot);
De = zeros(Nlyr,Nkp_tot);
Dh = zeros(Nlyr,Nkp_tot);
kp_tot = zeros(1,Nkp_tot);

% calculate the kz
for ig = 1:Nkp-1
    dkp_int = (kp(ig+1)-kp(ig))/2 * gau_nod;
    kp_tot((ig-1)*Nkp_int+(1:Nkp_int)) = dkp_int + (kp(ig)+kp(ig+1))/2;
end
kz_tot = sqrt(repmat(kn.^2,1,Nkp_tot) - repmat(kp_tot.^2,Nlyr,1));
% Improved branch selection: ensure Im(kz) < 0 for proper wave decay
% This is more accurate than using angle, especially for lossy media
id_kz = imag(kz_tot) > 0;
kz_tot(id_kz) = -kz_tot(id_kz);

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

Ze_inf = 1./ep_lyr;
Zh_inf = mur_lyr;
Re_ij_inf = (Ze_inf(1:Nlyr-1)-Ze_inf(2:Nlyr))./(Ze_inf(2:Nlyr)+Ze_inf(1:Nlyr-1));
Re_ji_inf = -Re_ij_inf;
Rh_ij_inf = (Zh_inf(1:Nlyr-1)-Zh_inf(2:Nlyr))./(Zh_inf(2:Nlyr)+Zh_inf(1:Nlyr-1));
Rh_ji_inf = -Rh_ij_inf;


% forward layer recursion
GRe_ij(Nbdy,1:Nkp_tot) = Re_ij(Nbdy,1:Nkp_tot);
GRh_ij(Nbdy,1:Nkp_tot) = Rh_ij(Nbdy,1:Nkp_tot);
for ih = Nbdy-1:-1:1
    GRe_ij(ih,1:Nkp_tot) = dgf_bdy_gcof(Re_ij(ih,1:Nkp_tot),GRe_ij(ih+1,1:Nkp_tot), kz_tot(ih+1,1:Nkp_tot), d_lyr(ih+1) );
    GRh_ij(ih,1:Nkp_tot) = dgf_bdy_gcof(Rh_ij(ih,1:Nkp_tot),GRh_ij(ih+1,1:Nkp_tot), kz_tot(ih+1,1:Nkp_tot), d_lyr(ih+1) );
end

% backward layer recursion
GRe_ji(1,1:Nkp_tot) = Re_ji(1,1:Nkp_tot);
GRh_ji(1,1:Nkp_tot) = Rh_ji(1,1:Nkp_tot);
for ih = 2:Nbdy
    GRe_ji(ih,1:Nkp_tot) = dgf_bdy_gcof(Re_ji(ih,1:Nkp_tot),GRe_ji(ih-1,1:Nkp_tot), kz_tot(ih,1:Nkp_tot), d_lyr(ih) );
    GRh_ji(ih,1:Nkp_tot) = dgf_bdy_gcof(Rh_ji(ih,1:Nkp_tot),GRh_ji(ih-1,1:Nkp_tot), kz_tot(ih,1:Nkp_tot), d_lyr(ih) );
end

% d_lyr(1) and d_lyr(Nlyr) are infinite due to on boundary exsit
% here a very large thickness 1e6 is used to represent that.
% Meanwhile, set De(1,1:Np) = 1; and De(Nlyr,1:Np)=1;
De(1,1:Nkp_tot) = 1;
De(Nlyr,1:Nkp_tot) = 1;
De(2:Nlyr-1,1:Nkp_tot) = 1 - GRe_ij(2:Nlyr-1,1:Nkp_tot).*GRe_ji(1:Nbdy-1,1:Nkp_tot)...
    .*exp(-2*1j*kz_tot(2:Nlyr-1,1:Nkp_tot).*repmat(d_lyr(2:Nlyr-1),1,Nkp_tot));
id_inf = isinf(De) | (isnan(De));
De(id_inf) = 1;

% d_lyr(1) is infinite due to on boundary exsit on the left
% d_lyr(Nlyr) is also infinite due to no boundary exsit on the right
Dh(1,1:Nkp_tot) = 1;
Dh(Nlyr,1:Nkp_tot) = 1;
Dh(2:Nlyr-1,1:Nkp_tot) = 1 - GRh_ij(2:Nlyr-1,1:Nkp_tot).*GRh_ji(1:Nbdy-1,1:Nkp_tot)...
    .*exp(-2*1j*kz_tot(2:Nlyr-1,1:Nkp_tot).*repmat(d_lyr(2:Nlyr-1),1,Nkp_tot));
id_inf = isinf(Dh) | (isnan(Dh)) ;
Dh(id_inf) = 1;


Gxx_num_new = 0;
Gxx_num = 0;
Gzx_num_new = 0;
Gzx_num = 0;
Gzz_num_new = 0;
Gzz_num = 0;
Gphi_num_new = 0;
Gphi_num = 0;

Gxx = zeros(Ns,Nf);
Gzx = zeros(Ns,Nf);
Gzz = zeros(Ns,Nf);
Gphi = zeros(Ns,Nf);
for ik = 1:Ns
    
    id_ls = dgf_locate_lyr(zs(ik), zbdy);
    
    p = sqrt( (pt_s(ik,1)-pt_f(1:Nf,1)).^2 + (pt_s(ik,2)-pt_f(1:Nf,2)).^2 );
    
    dz0 = abs(zs(ik)-zf(1:Nf));
    
    dz1 = zeros(Nf,1);
    dz2 = zeros(Nf,1);
    dz3 = zeros(Nf,1);
    dz4 = zeros(Nf,1);
    
    indxx = dv_s(ik,:);
    indzx = dv_s(ik,:);
    indzz = dv_s(ik,:);
    indphi = dv_s(ik,:);
    
    if id_ls==1
        idz_lyr1 = (id_lf==1);
        
        dz1 = 2*zbdy(1)-(zs(ik)+zf);
        
        
    elseif id_ls==Nlyr
        
        
    else
        % source and field in the same layer
        idf_same = (id_lf==id_ls);
        
        dz1(idf_same) = 2*zbdy(id_ls-1) - (zf(idf_same)+zs(ik));
        dz2(idf_same) = (zf(idf_same)+zs(ik)) - 2*zbdy(id_ls);
        dz3(idf_same) = 2*d_lyr(id_ls) + (zf(idf_same)-zs(ik));
        dz4(idf_same) = 2*d_lyr(id_ls) - (zf(idf_same)-zs(ik));
        
        % field in the front layer of source
        idf_adj = (id_lf>id_ls);
        
        zf_tmp = zbdy(id_ls);
        
        dz1(idf_adj) = 2*zbdy(ik) - (zf_tmp+zs(ik));
        dz2(idf_adj) = (zf_tmp+zs(ik)) - 2*zbdy(id_ls-1);
        dz3(idf_adj) = 2*d_lyr(id_ls) + (zf_tmp-zs(ik));
        dz4(idf_adj) = 2*d_lyr(id_ls) - (zf_tmp-zs(ik));
        
        % source and field in the back layer of source
        idf_oth = (id_lf<id_ls);
        
        zf_tmp = zbdy(id_ls-1);
        
        dz1(idf_oth) = 2*zbdy(id_ls) - (zf_tmp+zs(ik));
        dz2(idf_oth) = (zf_tmp+zs(ik)) - 2*zbdy(id_ls-1);
        dz3(idf_oth) = 2*d_lyr(id_ls) + (zf_tmp-zs(ik));
        dz4(idf_oth) = 2*d_lyr(id_ls) - (zf_tmp-zs(ik));
    end
    
    
    % Gxx
    for ig = 1:Nkp-1
        ind_int = (ig-1)*Nkp_int+(1:Nkp_int);
        
        kp_int = kp_tot(ind_int);
        kz_int = kz_tot(1:Nlyr,ind_int);
        dkp_seg = (kp(ig+1)-kp(ig))/2;
        
        [Ah0,Ah1,Ah2,Ah3,Ah4] = dgf_kernal_A(dz0,dz1,dz2,dz3,dz4,...
            Dh(id_ls,ind_int), GRh_ij(id_ls,ind_int),GRh_ji(id_ls-1,ind_int), ...
            kz_int(id_ls,1:Nkp_int));
        
        [Asub_h0,Asub_h1,Asub_h2,Asub_h3,Asub_h4] = dgf_kernal_Asub(dz0,dz1,dz2,dz3,dz4,...
            Rh_ij_inf(id_ls), Rh_ji_inf(id_ls-1), kz_int(id_ls,1:Nkp_int));
        
        J0_tmp = besselj(0,repmat(kp_int,Nf,1).*repmat(p,1,Nkp_int));
        
        % substraction term
        Vih_Asub_tmp_sub = repmat(Zh(id_ls,(ig-1)*Nkp_int+(1:Nkp_int)),Nf,1)/2.*(Asub_h0+Asub_h1+Asub_h2+Asub_h3+Asub_h4);
        % original term
        Vih_tmp_sub = repmat(Zh(id_ls,(ig-1)*Nkp_int+(1:Nkp_int)),Nf,1)/2.*(Ah0+Ah1+Ah2+Ah3+Ah4);
        
        Vih_J0_tmp = (Vih_tmp_sub-Vih_Asub_tmp_sub).*J0_tmp.*repmat(kp_int,Nf,1)*dkp_seg;
        
        Gxx_num_new = Gxx_num + sum(Vih_J0_tmp.*repmat(gau_wei,Nf,1) ,2);
        if abs(Gxx_num_new-Gxx_num)<(abs(Gxx_num)*err_tol)
            disp(['Gxx -- computing step:  ',num2str(ig)]);
            break;
        else
            Gxx_num = Gxx_num_new;
        end
    end
    
    
    % Gzx
    for ig = 1:Nkp-1
        kp_int = kp_tot((ig-1)*Nkp_int+(1:Nkp_int));
        kz_int = kz_tot(1:Nlyr,(ig-1)*Nkp_int+(1:Nkp_int));
        dkp_seg = (kp(ig+1)-kp(ig))/2;
        
        [Ah0,Ah1,Ah2,Ah3,Ah4] = dgf_kernal_A(dz0,dz1,dz2,dz3,dz4,...
            Dh(id_ls,(ig-1)*Nkp_int+(1:Nkp_int)), ...
            GRh_ij(id_ls,(ig-1)*Nkp_int+(1:Nkp_int)),...
            GRh_ji(id_ls-1,(ig-1)*Nkp_int+(1:Nkp_int)), ...
            kz_int(id_ls,1:Nkp_int));
        
        [Ae0,Ae1,Ae2,Ae3,Ae4] = dgf_kernal_A(dz0,dz1,dz2,dz3,dz4,...
            De(id_ls,(ig-1)*Nkp_int+(1:Nkp_int)), ...
            GRe_ij(id_ls,(ig-1)*Nkp_int+(1:Nkp_int)),...
            GRe_ji(id_ls-1,(ig-1)*Nkp_int+(1:Nkp_int)), ...
            kz_int(id_ls,1:Nkp_int));
        
        [Bsub_h0,Bsub_h1,Bsub_h2,Bsub_h3,Bsub_h4] = dgf_kernal_Bsub(dz0,dz1,dz2,dz3,dz4,...
            Rh_ij_inf(id_ls),...
            Rh_ji_inf(id_ls-1), ...
            kp_int);
        
        [Bsub_e0,Bsub_e1,Bsub_e2,Bsub_e3,Bsub_e4] = dgf_kernal_Bsub(dz0,dz1,dz2,dz3,dz4,...
            Re_ij_inf(id_ls),...
            Re_ji_inf(id_ls-1), ...
            kp_int);
        
        J1_tmp = besselj(1,repmat(kp_int,Nf,1).*repmat(p,1,Nkp_int));
        
        % substraction term
        Iih_Bsub_tmp_sub = 1/2*(repmat(sign(zf-zs),1,Nkp_int).*Bsub_h0-Bsub_h1+Bsub_h2+Bsub_h3-Bsub_h4);
        Iie_Bsub_tmp_sub = 1/2*(repmat(sign(zf-zs),1,Nkp_int).*Bsub_e0-Bsub_e1+Bsub_e2+Bsub_e3-Bsub_e4);
        % original term
        Iih_J1_tmp = (1/2*(repmat(sign(zf-zs),1,Nkp_int).*Ah0-Ah1+Ah2+Ah3-Ah4)-Iih_Bsub_tmp_sub) ...
            .*J1_tmp.*repmat(kp_int.^2,Nf,1)*dkp_seg;
        Iie_J1_tmp = (1/2*(repmat(sign(zf-zs),1,Nkp_int).*Ae0-Ae1+Ae2+Ae3-Ae4)-Iie_Bsub_tmp_sub) ...
            .*J1_tmp.*repmat(kp_int.^2,Nf,1)*dkp_seg;
        
        %         Iih_J1 = Iih_J1 + sum( Iih_J1_tmp.*repmat(gau_wei,Nf,1) ,2 );
        %         Iie_J1 = Iie_J1 + sum( Iie_J1_tmp.*repmat(gau_wei,Nf,1) ,2 );
        Gzx_num_new = Gzx_num + sum( (Iih_J1_tmp-Iie_J1_tmp).*repmat(gau_wei,Nf,1) ,2 );
        if abs(Gzx_num_new-Gzx_num)<(abs(Gzx_num)*err_tol)
            disp(['Gzx -- computing step:  ',num2str(ig)]);
            break;
        else
            Gzx_num = Gzx_num_new;
        end
    end
    
    
    % Gzz
    for ig = 1:Nkp-1
        kp_int = kp_tot((ig-1)*Nkp_int+(1:Nkp_int));
        kz_int = kz_tot(1:Nlyr,(ig-1)*Nkp_int+(1:Nkp_int));
        dkp_seg = (kp(ig+1)-kp(ig))/2;
        % wave impedance when kp->infinity
        Zh_kp = (w0*mu_lyr(id_ls))./(-1j*kp_int);
        Ze_kp = (-1j*kp_int)./(w0*ep_lyr(id_ls));
        
        [Ah0,Ah1,Ah2,Ah3,Ah4] = dgf_kernal_A(dz0,dz1,dz2,dz3,dz4,...
            Dh(id_ls,(ig-1)*Nkp_int+(1:Nkp_int)), ...
            GRh_ij(id_ls,(ig-1)*Nkp_int+(1:Nkp_int)),...
            GRh_ji(id_ls-1,(ig-1)*Nkp_int+(1:Nkp_int)), ...
            kz_int(id_ls,1:Nkp_int));
        [Ae0,Ae1,Ae2,Ae3,Ae4] = dgf_kernal_A(dz0,dz1,dz2,dz3,dz4,...
            De(id_ls,(ig-1)*Nkp_int+(1:Nkp_int)), ...
            GRe_ij(id_ls,(ig-1)*Nkp_int+(1:Nkp_int)),...
            GRe_ji(id_ls-1,(ig-1)*Nkp_int+(1:Nkp_int)), ...
            kz_int(id_ls,1:Nkp_int));
        
        [Asub_e0,Asub_e1,Asub_e2,Asub_e3,Asub_e4] = dgf_kernal_Asub(dz0,dz1,dz2,dz3,dz4,...
            Re_ij_inf(id_ls),Re_ji_inf(id_ls-1),kz_int(id_ls,1:Nkp_int));
        [Bsub_h0,Bsub_h1,Bsub_h2,Bsub_h3,Bsub_h4] = dgf_kernal_Bsub(dz0,dz1,dz2,dz3,dz4,...
            Rh_ij_inf(id_ls),Rh_ji_inf(id_ls-1),kp_int);
        [Bsub_e0,Bsub_e1,Bsub_e2,Bsub_e3,Bsub_e4] = dgf_kernal_Bsub(dz0,dz1,dz2,dz3,dz4,...
            Re_ij_inf(id_ls),Re_ji_inf(id_ls-1),kp_int);
        
        J0_tmp = besselj(0,repmat(kp_int,Nf,1).*repmat(p,1,Nkp_int));
        
        % substraction term
        Ivh_Bsub_tmp_sub = repmat(1./Zh_kp,Nf,1)/2.*(Bsub_h0-Bsub_h1-Bsub_h2+Bsub_h3+Bsub_h4);
        Ive_Bsub_tmp_sub = repmat(1./Ze_kp,Nf,1)/2.*(Bsub_e0-Bsub_e1-Bsub_e2+Bsub_e3+Bsub_e4);
        Ive_Asub_tmp_sub = repmat(1./Ze(id_ls,(ig-1)*Nkp_int+(1:Nkp_int)),Nf,1)/2.*(Asub_e0-Asub_e1-Asub_e2+Asub_e3+Asub_e4);
        
        Ive_tmp_sub = repmat(1./Ze(id_ls,(ig-1)*Nkp_int+(1:Nkp_int)),Nf,1)/2.*(Ae0-Ae1-Ae2+Ae3+Ae4);
        
        Ivh_J0_tmp = (repmat(1./Zh(id_ls,(ig-1)*Nkp_int+(1:Nkp_int)),Nf,1)/2 ...
            .*(Ah0-Ah1-Ah2+Ah3+Ah4)-Ivh_Bsub_tmp_sub) .*J0_tmp./repmat(kp_int,Nf,1)*dkp_seg;
        Ive_J0_tmp_A = (Ive_tmp_sub - Ive_Asub_tmp_sub) .* J0_tmp.*repmat(kp_int,Nf,1)*dkp_seg;
        Ive_J0_tmp_B = (repmat(kz_int(id_ls,1:Nkp_int).^2,Nf,1) .* Ive_tmp_sub ...
            - (-repmat(kp_int.^2,Nf,1)).*Ive_Bsub_tmp_sub) .* J0_tmp./repmat(kp_int,Nf,1)*dkp_seg;
        
        Gzz_num_new = Gzz_num + sum( (k0^2*mur_lyr(id_ls)*repmat(mur_lyr_f,1,Nint) .* Ivh_J0_tmp ...
            + repmat(mur_lyr_f,1,Nint)/epr_lyr(id_ls).*Ive_J0_tmp_A ...
            - mur_lyr(id_ls)./repmat(epr_lyr_f,1,Nint).*Ive_J0_tmp_B).*repmat(gau_wei,Nf,1), 2);
        
        if abs(Gzz_num_new-Gzz_num)<(abs(Gzz_num)*err_tol)
            disp(['Gzz -- computing step:  ',num2str(ig)]);
            break;
        elseif ig == Nkp-1
            disp(['Gzz -- comupting step:  ',num2str(ig),'  Did not converge!']);
        else
            Gzz_num = Gzz_num_new;
        end
        
    end
    
    
    % Gphi
    for ig = 1:Nkp-1
        kp_int = kp_tot((ig-1)*Nkp_int+(1:Nkp_int));
        kz_int = kz_tot(1:Nlyr,(ig-1)*Nkp_int+(1:Nkp_int));
        dkp_seg = (kp(ig+1)-kp(ig))/2;
        % wave impedance when kp->infinity
        %         Zh_kp = (w0*mu_lyr(id_ls))./(-1j*kp_int);
        Ze_kp = (-1j*kp_int)./(w0*ep_lyr(id_ls));
        
        [Ah0,Ah1,Ah2,Ah3,Ah4] = dgf_kernal_A(dz0,dz1,dz2,dz3,dz4,...
            Dh(id_ls,(ig-1)*Nkp_int+(1:Nkp_int)), ...
            GRh_ij(id_ls,(ig-1)*Nkp_int+(1:Nkp_int)),...
            GRh_ji(id_ls-1,(ig-1)*Nkp_int+(1:Nkp_int)), ...
            kz_int(id_ls,1:Nkp_int));
        [Ae0,Ae1,Ae2,Ae3,Ae4] = dgf_kernal_A(dz0,dz1,dz2,dz3,dz4,...
            De(id_ls,(ig-1)*Nkp_int+(1:Nkp_int)), ...
            GRe_ij(id_ls,(ig-1)*Nkp_int+(1:Nkp_int)),...
            GRe_ji(id_ls-1,(ig-1)*Nkp_int+(1:Nkp_int)), ...
            kz_int(id_ls,1:Nkp_int));
        
        %         [Bsub_h0,Bsub_h1,Bsub_h2,Bsub_h3,Bsub_h4] = dgf_kernal_Bsub(dz0,dz1,dz2,dz3,dz4,...
        %             Rh_ij_inf(id_ls),Rh_ji_inf(id_ls-1),kp_int);
        [Bsub_e0,Bsub_e1,Bsub_e2,Bsub_e3,Bsub_e4] = dgf_kernal_Bsub(dz0,dz1,dz2,dz3,dz4,...
            Re_ij_inf(id_ls),Re_ji_inf(id_ls-1),kp_int);
        
        J0_tmp = besselj(0,repmat(kp_int,Nf,1).*repmat(p,1,Nkp_int));
        %         J1_tmp = besselj(1,repmat(kp_int,Nf,1).*repmat(p,1,Nkp_int));
        
        % substraction term
        Vie_Bsub_tmp_sub = repmat(Ze_kp,Nf,1)/2.*(Bsub_e0+Bsub_e1+Bsub_e2+Bsub_e3+Bsub_e4);
        %         Vih_Bsub_tmp_sub = repmat(Zh_kp,Nf,1)/2.*(Bsub_h0+Bsub_h1+Bsub_h2+Bsub_h3+Bsub_h4);
        
        Vie_J0_kp_tmp = (repmat(Ze(id_ls,(ig-1)*Nkp_int+(1:Nkp_int)),Nf,1)/2.*(Ae0+Ae1+Ae2+Ae3+Ae4) ...
            -Vie_Bsub_tmp_sub) .* J0_tmp./repmat(kp_int,Nf,1)*dkp_seg;
        %         Vih_J0_kp_tmp = (Vih_tmp_sub-Vih_Bsub_tmp_sub) .* J0_tmp./repmat(kp_int,Nf,1)*dkp;
        Vih_J0_kp_tmp = (repmat(Zh(id_ls,(ig-1)*Nkp_int+(1:Nkp_int)),Nf,1)/2.*(Ah0+Ah1+Ah2+Ah3+Ah4) ...
            ).* J0_tmp./repmat(kp_int,Nf,1)*dkp_seg;
        
        %cof_Gphi = cof_Gphi + sum(1./repmat(kp_int.^2,Nf,1).*repmat(gau_wei,Nf,1) ,2);
        Gphi_num_new = Gphi_num + sum((Vie_J0_kp_tmp-Vih_J0_kp_tmp).*repmat(gau_wei,Nf,1) ,2);
        
        if abs(Gphi_num_new-Gphi_num)<(abs(Gphi_num)*err_tol)
            disp(['Gphi -- computing step:  ',num2str(ig)]);
            break;
        elseif ig == Nkp-1
            disp(['Gphi -- comupting step:  ',num2str(ig),'  Did not converge!']);
        else
            Gphi_num = Gphi_num_new;
        end
        
    end
    
    
    [At_h0,At_h1,At_h2,At_h3,At_h4] = dgf_tail_A...
        (dz0,dz1,dz2,dz3,dz4,p, Rh_ij_inf(id_ls),Rh_ji_inf(id_ls-1), kn(id_ls));
    Gxx_tail = mur_lyr(id_ls)/2 * (At_h0+At_h1+At_h2+At_h3+At_h4);
    
    
    [Bt_h0,Bt_h1,Bt_h2,Bt_h3,Bt_h4] = dgf_tail_B...
        (dz0,dz1,dz2,dz3,dz4,p, Rh_ij_inf(id_ls),Rh_ji_inf(id_ls-1));
    [Bt_e0,Bt_e1,Bt_e2,Bt_e3,Bt_e4] = dgf_tail_B...
        (dz0,dz1,dz2,dz3,dz4,p, Re_ij_inf(id_ls),Re_ji_inf(id_ls-1));
    Gzx_tail = sign(zf-zs).*Bt_h0-Bt_h1+Bt_h2+Bt_h3-Bt_h4 ...
        - sign(zf-zs).*Bt_e0+Bt_e1-Bt_e2-Bt_e3+Bt_e4;
    
    
    [Ct_h0,Ct_h1,Ct_h2,Ct_h3,Ct_h4] = dgf_tail_C...
        (dz0,dz1,dz2,dz3,dz4,p, Rh_ij_inf(id_ls),Rh_ji_inf(id_ls-1));
    [Ct_e0,Ct_e1,Ct_e2,Ct_e3,Ct_e4] = dgf_tail_C...
        (dz0,dz1,dz2,dz3,dz4,p, Re_ij_inf(id_ls),Re_ji_inf(id_ls-1));
    [At_e0,At_e1,At_e2,At_e3,At_e4] = dgf_tail_A...
        (dz0,dz1,dz2,dz3,dz4,p, Re_ij_inf(id_ls),Re_ji_inf(id_ls-1), kn(id_ls));
    Gzz_tail = -1j*k0^2*mur_lyr(id_ls)/(2*w0*mu0) * (Ct_h0-Ct_h1-Ct_h2+Ct_h3+Ct_h4) ...
        + 1j*mur_lyr_f*w0*ep0/2 .* ( At_e0-At_e1-At_e2+At_e3+At_e4) ...
        + 1j*w0*ep0*mur_lyr(id_ls)*epr_lyr(id_ls)./(2*epr_lyr_f) .* (Ct_e0-Ct_e1-Ct_e2+Ct_e3+Ct_e4);
    
    
    %     [Dt_e0,Dt_e1,Dt_e2,Dt_e3,Dt_e4] = dgf_tail_D...
    %         (dz0,dz1,dz2,dz3,dz4,p, Re_ij(id_ls,end),Re_ji(id_ls-1,end));
    %   + w0*mu0.*mur_lyr(id_ls)./(2*1j)
    % Fixed: Changed to match other implementations (dgf_main_sub_same_sub_i.m)
    % Original: Gphi_tail = (-1j./(2*w0*ep0*epr_lyr(id_ls))).*(Ct_e0+Ct_e1+Ct_e2+Ct_e3+Ct_e4);
    Gphi_tail = 1./(2*epr_lyr(id_ls)).*(Ct_e0+Ct_e1+Ct_e2+Ct_e3+Ct_e4);
    
    
    
    Gxx(ik,1:Nf) = 1/(2*pi) * (Gxx_tail + 1/(1j*w0*mu0) * Gxx_num);
    Gzx(ik,1:Nf) = 1/(2*pi) * 1/(1j*w0*mu0) * -mur_lyr(id_ls) * ( Gzx_tail + Gzx_num ); % here kx=kn
    Gzz(ik,1:Nf) = 1/(2*pi)*1/(1j*w0*ep0) *  ( Gzz_tail + Gzz_num );
    % Fixed: Changed to match other implementations for consistency
    % Original: Gphi(ik,1:Nf) = 1/(2*pi)*1j*w0*ep0  * ( Gphi_tail + Gphi_num );
    Gphi(ik,1:Nf) = 1/(2*pi) * ( Gphi_tail + 1j*w0*ep0 * Gphi_num );
    
end


end


