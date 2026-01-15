function [Gxx,Gzx,Gzz,Gphi] = dgf_main_sub_same_sub_1(...
    Zh,Rh_ij,Rh_ji, Ze,Re_ij,Re_ji, ...
    Rh_ij_inf,Rh_ji_inf,Re_ij_inf,Re_ji_inf, kz_tot,kp_tot, kp,dkp, ...
    zbdy, epr_lyr,mur_lyr, pt_s,dv_s, pt_f,dv_f, ...
    dz0,dz2,gau_nod,gau_wei, w0)
%  Function:       dgf_main_sub_same_sub_1
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


err_tol = 1e-10;

%% basic parameters of layers
Nint = length(gau_nod); % order of the Guassion integration 
Nkp = length(kp);

mu0 = 4e-7*pi;
ep0 = 8.854187817e-12;
k0 = w0*sqrt(ep0.*mu0);

mu_lyr = mu0*mur_lyr;
ep_lyr = ep0*epr_lyr;
kn = w0*sqrt(ep_lyr.*mu_lyr);


Nbdy = size(zbdy,1);
Nlyr = Nbdy+1;

Ns = size(pt_s,1);
Nf = size(pt_f,1);

zs = pt_s(1:Ns,3);
zf = pt_f(1:Nf,3);

%% calculate the integration
Nstep_Gxx = 0;
Nstep_Gzx = 0;
Nstep_Gzz = 0;
Gxx = zeros(Ns,Nf);
Gzx = zeros(Ns,Nf);
Gzz = zeros(Ns,Nf);
Gphi = zeros(Ns,Nf);
for ik = 1:Ns
    
    Gxx_num_new = 0;
    Gxx_num = zeros(Nf,1);
    Gzx_num_new = 0;
    Gzx_num = zeros(Nf,1);
    Gzz_num_new = 0;
    Gzz_num = zeros(Nf,1);
    Gphi_num_new = 0;
    Gphi_num = zeros(Nf,1);
    
    id_s = dgf_locate_lyr(zs(ik), zbdy);
    
    p = sqrt( (pt_s(ik,1)-pt_f(1:Nf,1)).^2 + (pt_s(ik,2)-pt_f(1:Nf,2)).^2 );

    
    indxx = dv_s(ik,:);
    indzx = dv_s(ik,:);
    indzz = dv_s(ik,:);
    indphi = dv_s(ik,:);
    indxx=1:Nf;
    indzx=1:Nf;
    indzz=1:Nf;
    indphi=1:Nf;
    Nxx = length(indxx);
    Nzx = length(indzx);
    Nzz = length(indzz);
    Nphi = length(indphi);
    
    
    
    % Gxx
    for ig = 1:Nkp-1
        ind_int = (ig-1)*Nint+(1:Nint);
        
        kp_int = kp_tot(ind_int);
        kz_int = kz_tot(1:Nlyr,ind_int);
        
        [Ah0,Ah2] = dgf_kernal_A_1(dz0,dz2,Rh_ji(id_s,ind_int), kz_int(id_s,1:Nint));

        [Asub_h0,Asub_h2] = dgf_kernal_Asub_1(dz0,dz2,Rh_ji_inf(id_s), kz_int(id_s,1:Nint));
            
        J0_tmp = besselj(0,repmat(kp_int,Nf,1).*repmat(p,1,Nint));
        
        % substraction term
        Vih_Asub_tmp_sub = repmat(Zh(id_s,ind_int),Nf,1)/2.*(Asub_h0+Asub_h2);
        % original term
        Vih_tmp_sub = repmat(Zh(id_s,ind_int),Nf,1)/2.*(Ah0+Ah2);
        
        Vih_J0_tmp = (Vih_tmp_sub-Vih_Asub_tmp_sub).*J0_tmp.*repmat(kp_int,Nf,1)*dkp;
        
        Gxx_num_new = Gxx_num + sum(Vih_J0_tmp.*repmat(gau_wei,Nf,1) ,2);
        if abs(Gxx_num_new-Gxx_num)<(abs(Gxx_num)*err_tol)
            Nstep_Gxx = ig;
            disp(['Gxx -- computing step:  ',num2str(ig)]);
            break;
        else
            Gxx_num = Gxx_num_new;
        end
    end
    
    
    % Gzx
    for ig = 1:Nkp-1
        ind_int = (ig-1)*Nint+(1:Nint);
        
        kp_int = kp_tot(ind_int);
        kz_int = kz_tot(1:Nlyr,ind_int);
        
        [Ah0,Ah2] = dgf_kernal_A_1(dz0,dz2,Rh_ji(id_s,ind_int),kz_int(id_s,1:Nint));
        [Ae0,Ae2] = dgf_kernal_A_1(dz0,dz2,Re_ji(id_s,ind_int),kz_int(id_s,1:Nint));
        
        [Bsub_h0,Bsub_h2] = dgf_kernal_Bsub_1(dz0,dz2,Rh_ji_inf(id_s),kp_int);
        [Bsub_e0,Bsub_e2] = dgf_kernal_Bsub_1(dz0,dz2,Re_ji_inf(id_s),kp_int);
        
        J1_tmp = besselj(1,repmat(kp_int,Nf,1).*repmat(p,1,Nint));
        
        % substraction term
        Iih_Bsub_tmp_sub = 1/2*(repmat(sign(zf-zs),1,Nint).*Bsub_h0+Bsub_h2);
        Iie_Bsub_tmp_sub = 1/2*(repmat(sign(zf-zs),1,Nint).*Bsub_e0+Bsub_e2);
        dkp_seg = (kp(ig+1)-kp(ig))/2;

        % original term
        Iih_J1_tmp = (1/2*(repmat(sign(zf-zs),1,Nint).*Ah0+Ah2)-Iih_Bsub_tmp_sub) ...
            .*J1_tmp.*repmat(kp_int.^2,Nf,1)*dkp_seg;
        Iie_J1_tmp = (1/2*(repmat(sign(zf-zs),1,Nint).*Ae0+Ae2)-Iie_Bsub_tmp_sub) ...
            .*J1_tmp.*repmat(kp_int.^2,Nf,1)*dkp_seg;

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
        ind_int = (ig-1)*Nint+(1:Nint);
        
        kp_int = kp_tot(ind_int);
        kz_int = kz_tot(1:Nlyr,ind_int);
        % wave impedance when kp->infinity
        Zh_kp = (w0*mu_lyr(id_s))./(-1j*kp_int);
        Ze_kp = (-1j*kp_int)./(w0*ep_lyr(id_s));
        
        [Ah0,Ah2] = dgf_kernal_A_1(dz0,dz2,Rh_ji(id_s,ind_int),kz_int(id_s,1:Nint));
        [Ae0,Ae2] = dgf_kernal_A_1(dz0,dz2,Re_ji(id_s,ind_int),kz_int(id_s,1:Nint));
            
        [Asub_e0,Asub_e2] = dgf_kernal_Asub_1(dz0,dz2,Re_ji_inf(id_s),kz_int(id_s,1:Nint));
        [Bsub_h0,Bsub_h2] = dgf_kernal_Bsub_1(dz0,dz2,Rh_ji_inf(id_s),kp_int); 
        [Bsub_e0,Bsub_e2] = dgf_kernal_Bsub_1(dz0,dz2,Re_ji_inf(id_s),kp_int);

        J0_tmp = besselj(0,repmat(kp_int,Nf,1).*repmat(p,1,Nint));
        
        % substraction term
        Ivh_Bsub_tmp_sub = repmat(1./Zh_kp,Nf,1)/2.*(Bsub_h0-Bsub_h2);
        Ive_Bsub_tmp_sub = repmat(1./Ze_kp,Nf,1)/2.*(Bsub_e0-Bsub_e2);
        Ive_Asub_tmp_sub = repmat(1./Ze(id_s,ind_int),Nf,1)/2.*(Asub_e0-Asub_e2);
        
        Ive_tmp_sub = repmat(1./Ze(id_s,ind_int),Nf,1)/2.*(Ae0-Ae2);
        
        Ivh_J0_tmp = (repmat(1./Zh(id_s,ind_int),Nf,1)/2 ...
            .*(Ah0-Ah2)-Ivh_Bsub_tmp_sub) .*J0_tmp./repmat(kp_int,Nf,1)*dkp;
        Ive_J0_tmp_A = (Ive_tmp_sub - Ive_Asub_tmp_sub) .* J0_tmp.*repmat(kp_int,Nf,1)*dkp;
        Ive_J0_tmp_B = (repmat(kz_int(id_s,1:Nint).^2,Nf,1) .* Ive_tmp_sub ...
            - (-repmat(kp_int.^2,Nf,1)).*Ive_Bsub_tmp_sub) .* J0_tmp./repmat(kp_int,Nf,1)*dkp;
        
        Gzz_num_new = Gzz_num + sum( (k0^2*mur_lyr(id_s)*mur_lyr(id_s) .* Ivh_J0_tmp ...
            + mur_lyr(id_s)/epr_lyr(id_s).*Ive_J0_tmp_A ...
            - mur_lyr(id_s)/epr_lyr(id_s).*Ive_J0_tmp_B).*repmat(gau_wei,Nf,1), 2);
        
        if abs(Gzz_num_new-Gzz_num)<(abs(Gzz_num)*err_tol)
            Nstep_Gzz = ig;
            disp(['Gzz -- computing step:  ',num2str(ig)]);
            break;
        elseif ig == Nkp-1
            disp(['Gzz -- comupting step:  ',num2str(ig),'  Did not converge!']);
        else
            Gzz_num = Gzz_num_new;
        end
        
    end
    
    
    % Gphi
    Nstep_min = min(min(Nstep_Gxx,Nstep_Gzx),Nstep_Gzz);
    for ig = 1:Nstep_min  %Nkp-1
        ind_int = (ig-1)*Nint+(1:Nint);
        
        kp_int = kp_tot(ind_int);
        kz_int = kz_tot(1:Nlyr,ind_int);
        % wave impedance when kp->infinity
        %         Zh_kp = (w0*mu_lyr(id_ls))./(-1j*kp_int);
        Ze_kp = (-1j*kp_int)./(w0*ep_lyr(id_s));
        
        [Ah0,Ah2] = dgf_kernal_A_1(dz0,dz2,Rh_ji(id_s,ind_int),kz_int(id_s,1:Nint));
        [Ae0,Ae2] = dgf_kernal_A(dz0,dz2,Re_ji(id_s,ind_int),kz_int(id_s,1:Nint));            ...
            
        
        %         [Bsub_h0,Bsub_h1,Bsub_h2,Bsub_h3,Bsub_h4] = dgf_kernal_Bsub(dz0,dz1,dz2,dz3,dz4,...
        %             Rh_ij_inf(id_ls),Rh_ji_inf(id_ls),kp_int);
        [Bsub_e0,Bsub_e2] = dgf_kernal_Bsub_1(dz0,dz2,Re_ji_inf(id_s),kp_int);
        
        J0_tmp = besselj(0,repmat(kp_int,Nf,1).*repmat(p,1,Nint));
        %         J1_tmp = besselj(1,repmat(kp_int,Nf,1).*repmat(p,1,Nkp_int));
        
        % substraction term
        Vie_Bsub_tmp_sub = repmat(Ze_kp,Nf,1)/2.*(Bsub_e0+Bsub_e2);
        %         Vih_Bsub_tmp_sub = repmat(Zh_kp,Nf,1)/2.*(Bsub_h0+Bsub_h1+Bsub_h2+Bsub_h3+Bsub_h4);
        
        Vie_J0_kp_tmp = (repmat(Ze(id_s,Nint),Nf,1)/2.*(Ae0+Ae2) ...
            -Vie_Bsub_tmp_sub) .* J0_tmp./repmat(kp_int,Nf,1)*dkp;
        %         Vih_J0_kp_tmp = (Vih_tmp_sub-Vih_Bsub_tmp_sub) .* J0_tmp./repmat(kp_int,Nf,1)*dkp;
        Vih_J0_kp_tmp = (repmat(Zh(id_s,Nint),Nf,1)/2.*(Ah0+Ah2) ...
            ).* J0_tmp./repmat(kp_int,Nf,1)*dkp;
        
        Gphi_num_new = Gphi_num + sum((Vie_J0_kp_tmp-Vih_J0_kp_tmp).*repmat(gau_wei,Nf,1) ,2);
        
        if abs(Gphi_num_new-Gphi_num)<(abs(Gphi_num)*err_tol)
            disp(['Gphi -- computing step:  ',num2str(ig)]);
            break;
        elseif ig == Nstep_min
            disp(['Gphi -- computing step:  ',num2str(ig)]);
            break;
        elseif ig == Nkp-1
            disp(['Gphi -- comupting step:  ',num2str(ig),'  Did not converge!']);
        else
            Gphi_num = Gphi_num_new;
        end
        
    end
    
    
    [At_h0,At_h2] = dgf_tail_A_1(dz0,dz2,p, Rh_ji_inf(id_s), kn(id_s));
        
    Gxx_tail = mur_lyr(id_s)/2 * (At_h0+At_h2);
    
    
    [Bt_h0,Bt_h2] = dgf_tail_B_1(dz0,dz2,p, Rh_ji_inf(id_s));
        
    [Bt_e0,Bt_e2] = dgf_tail_B_1(dz0,dz2,p, Re_ji_inf(id_s));
        
    Gzx_tail = sign(zf-zs).*Bt_h0+Bt_h2 - sign(zf-zs).*Bt_e0-Bt_e2; ...
        
    
    
    [Ct_h0,Ct_h2] = dgf_tail_C_1(dz0,dz2,p, Rh_ji_inf(id_s));
    [Ct_e0,Ct_e2] = dgf_tail_C_1(dz0,dz2,p, Re_ji_inf(id_s));
    [At_e0,At_e2] = dgf_tail_A_1(dz0,dz2,p, Re_ji_inf(id_s), kn(id_s));
        
    Gzz_tail = -mur_lyr(id_s)/2 * (Ct_h0-Ct_h2) ...
        + mur_lyr(id_s)/2 .* ( At_e0-At_e2) ...
        + mur_lyr(id_s)*epr_lyr(id_s)./(2*epr_lyr(id_s)) .* (Ct_e0-Ct_e2);
    
    
    %     [Dt_e0,Dt_e1,Dt_e2,Dt_e3,Dt_e4] = dgf_tail_D...
    %         (dz0,dz1,dz2,dz3,dz4,p, Re_ij(id_ls,end),Re_ji(id_s,end));
    %   + w0*mu0.*mur_lyr(id_ls)./(2*1j)
    Gphi_tail = 1./(2*epr_lyr(id_s)).*(Ct_e0+Ct_e2);
    
    
    
    Gxx(ik,1:Nf) = 1/(2*pi) * (Gxx_tail + 1/(1j*w0*mu0) * Gxx_num);
    Gzx(ik,1:Nf) = 1/(2*pi) * 1/(1j*w0*mu0) * -mur_lyr(id_s) * ( Gzx_tail + Gzx_num ); % here kx=kn
    Gzz(ik,1:Nf) = 1/(2*pi) * ( Gzz_tail + 1/(1j*w0*ep0) * Gzz_num );
    Gphi(ik,1:Nf) = 1/(2*pi) * ( Gphi_tail + 1j*w0*ep0 * Gphi_num );
    
end


end


