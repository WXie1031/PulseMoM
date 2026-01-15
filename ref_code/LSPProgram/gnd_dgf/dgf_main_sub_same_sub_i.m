function [Gxx,Gzx,Gzz,Gphi] = dgf_main_sub_same_sub_i(...
    Zh,Dh,GRh_ij,GRh_ji, Ze,De,GRe_ij,GRe_ji, ...
    Rh_ij_inf,Rh_ji_inf,Re_ij_inf,Re_ji_inf, kz_tot,kp_tot, kp,dkp, ...
    zbdy, epr_lyr,mur_lyr, pt_s,dv_s, pt_f,dv_f, ...
    dz0,dz1,dz2,dz3,dz4, gau_nod,gau_wei, w0)
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


err_tol = 1e-5;

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
    Gxx_tmp1=[];Gxx_tmpN=[];
    % Gxx
    for ig = 1:Nkp-1
        ind_int = (ig-1)*Nint+(1:Nint);
        
        kp_int = kp_tot(ind_int);
        kz_int = kz_tot(1:Nlyr,ind_int);
        
        [Ah0,Ah1,Ah2,Ah3,Ah4] = dgf_kernal_A(dz0(indxx),dz1(indxx),dz2(indxx),dz3(indxx),dz4(indxx),...
            Dh(id_s,ind_int), GRh_ij(id_s,ind_int),GRh_ji(id_s-1,ind_int), ...
            kz_int(id_s,1:Nint));
        
        [Asub_h0,Asub_h1,Asub_h2,Asub_h3,Asub_h4] = dgf_kernal_Asub...
            (dz0(indxx),dz1(indxx),dz2(indxx),dz3(indxx),dz4(indxx),...
            Rh_ij_inf(id_s), Rh_ji_inf(id_s-1), kz_int(id_s,1:Nint));
        
        J0_tmp = besselj(0,repmat(kp_int,Nxx,1).*repmat(p,1,Nint));
        
        % original term
        Vih_tmp_sub = repmat(Zh(id_s,ind_int),Nxx,1)/2.*(Ah0+Ah1+Ah2+Ah3+Ah4);
        % substraction term
        Vih_Asub_tmp_sub = repmat(Zh(id_s,ind_int),Nxx,1)/2.*(Asub_h0+Asub_h1+Asub_h2+Asub_h3+Asub_h4);
 
        Vih_J0_tmp = (Vih_tmp_sub-Vih_Asub_tmp_sub).*J0_tmp.*repmat(kp_int,Nxx,1)*dkp;
        Gxx_tmp1(ig) = sum(Vih_J0_tmp(1,:).*gau_wei ,2);
        Gxx_tmpN(ig) = sum(Vih_J0_tmp(Nxx,:).*gau_wei ,2);
        Gxx_num_new = Gxx_num + sum(Vih_J0_tmp.*repmat(gau_wei,Nxx,1) ,2);
        if abs(Gxx_num_new-Gxx_num)<(abs(Gxx_num)*err_tol)
            Nstep_Gxx = ig;
            disp(['Gxx -- computing step:  ',num2str(ig)]);
            break;
        elseif ig == Nkp-1
            disp(['Gxx -- comupting step:  ',num2str(ig),'  Did not converge!']);
        else
            Gxx_num = Gxx_num_new;
        end
    end
    
   Gxx_tmp1=[];Gxx_tmpN=[];
    % Gzx
    for ig = 1:Nkp-1
        ind_int = (ig-1)*Nint+(1:Nint);
        
        kp_int = kp_tot(ind_int);
        kz_int = kz_tot(1:Nlyr,ind_int);
        
        [Ah0,Ah1,Ah2,Ah3,Ah4] = dgf_kernal_A(dz0(indzx),dz1(indzx),dz2(indzx),dz3(indzx),dz4(indzx),...
            Dh(id_s,ind_int),GRh_ij(id_s,ind_int),GRh_ji(id_s-1,ind_int), ...
            kz_int(id_s,1:Nint));
        
        [Ae0,Ae1,Ae2,Ae3,Ae4] = dgf_kernal_A(dz0(indzx),dz1(indzx),dz2(indzx),dz3(indzx),dz4(indzx),...
            De(id_s,ind_int),GRe_ij(id_s,ind_int),GRe_ji(id_s-1,ind_int), ...
            kz_int(id_s,1:Nint));
        
        [Bsub_h0,Bsub_h1,Bsub_h2,Bsub_h3,Bsub_h4] = dgf_kernal_Bsub...
            (dz0(indzx),dz1(indzx),dz2(indzx),dz3(indzx),dz4(indzx),...
            Rh_ij_inf(id_s),Rh_ji_inf(id_s-1), kp_int);
        
        [Bsub_e0,Bsub_e1,Bsub_e2,Bsub_e3,Bsub_e4] = dgf_kernal_Bsub(dz0(indzx),dz1(indzx),dz2(indzx),dz3(indzx),dz4(indzx),...
            Re_ij_inf(id_s),Re_ji_inf(id_s-1), kp_int);
        
        J1_tmp = besselj(1,repmat(kp_int,Nzx,1).*repmat(p,1,Nint));

        dkp_seg = (kp(ig+1)-kp(ig))/2;

        sign_A0 = sign(pt_f(indzx,3)-zs(ik));
        sign_A0(sign_A0==0) = 1;
        sign_A0 = repmat(sign_A0,1,Nint);

        % substraction term
        Iih_Bsub_tmp_sub = 1/2*(sign_A0.*Bsub_h0-Bsub_h1+Bsub_h2+Bsub_h3-Bsub_h4);
        Iie_Bsub_tmp_sub = 1/2*(sign_A0.*Bsub_e0-Bsub_e1+Bsub_e2+Bsub_e3-Bsub_e4);
        % original term
        Iih_J1_tmp = (1/2*(sign_A0.*Ah0-Ah1+Ah2+Ah3-Ah4)-Iih_Bsub_tmp_sub).*J1_tmp.*repmat(kp_int.^2,Nzx,1)*dkp_seg;
        Iie_J1_tmp = (1/2*(sign_A0.*Ae0-Ae1+Ae2+Ae3-Ae4)-Iie_Bsub_tmp_sub).*J1_tmp.*repmat(kp_int.^2,Nzx,1)*dkp_seg;

        Gxx_tmp1(ig) = sum((Iih_J1_tmp(1,:)-Iie_J1_tmp(1,:)).*gau_wei ,2);
        Gxx_tmpN(ig) = sum((Iih_J1_tmp(Nxx,:)-Iie_J1_tmp(Nxx,:)).*gau_wei ,2);
        
        Gzx_num_new = Gzx_num + sum( (Iih_J1_tmp-Iie_J1_tmp).*repmat(gau_wei,Nzx,1) ,2 );
        if abs(Gzx_num_new-Gzx_num)<(abs(Gzx_num)*err_tol)
            Nstep_Gzx = ig;
            disp(['Gzx -- computing step:  ',num2str(ig)]);
            break;
        elseif ig == Nkp-1
            disp(['Gzx -- comupting step:  ',num2str(ig),'  Did not converge!']);
        else
            Gzx_num = Gzx_num_new;
        end
    end
    
    Gxx_tmp1=[];Gxx_tmpN=[];
    % Gzz
    for ig = 1:Nkp-1
        ind_int = (ig-1)*Nint+(1:Nint);
        
        kp_int = kp_tot(ind_int);
        kz_int = kz_tot(1:Nlyr,ind_int);
        % wave impedance when kp->infinity
        Zh_kp = (w0*mu_lyr(id_s))./(-1j*kp_int);
        Ze_kp = (-1j*kp_int)./(w0*ep_lyr(id_s));
        
        [Ah0,Ah1,Ah2,Ah3,Ah4] = dgf_kernal_A(dz0(indzz),dz1(indzz),dz2(indzz),dz3(indzz),dz4(indzz),...
            Dh(id_s,ind_int),GRh_ij(id_s,ind_int),GRh_ji(id_s-1,ind_int), ...
            kz_int(id_s,1:Nint));
        [Ae0,Ae1,Ae2,Ae3,Ae4] = dgf_kernal_A(dz0(indzz),dz1(indzz),dz2(indzz),dz3(indzz),dz4(indzz),...
            De(id_s,ind_int),GRe_ij(id_s,ind_int),GRe_ji(id_s-1,ind_int), ...
            kz_int(id_s,1:Nint));
        
        [Asub_e0,Asub_e1,Asub_e2,Asub_e3,Asub_e4] = dgf_kernal_Asub...
            (dz0(indzz),dz1(indzz),dz2(indzz),dz3(indzz),dz4(indzz),...
            Re_ij_inf(id_s),Re_ji_inf(id_s-1),kz_int(id_s,1:Nint));
        [Bsub_h0,Bsub_h1,Bsub_h2,Bsub_h3,Bsub_h4] = dgf_kernal_Bsub...
            (dz0(indzz),dz1(indzz),dz2(indzz),dz3(indzz),dz4(indzz),...
            Rh_ij_inf(id_s),Rh_ji_inf(id_s-1),kp_int);
        [Bsub_e0,Bsub_e1,Bsub_e2,Bsub_e3,Bsub_e4] = dgf_kernal_Bsub...
            (dz0(indzz),dz1(indzz),dz2(indzz),dz3(indzz),dz4(indzz),...
            Re_ij_inf(id_s),Re_ji_inf(id_s-1),kp_int);
        
        J0_tmp = besselj(0,repmat(kp_int,Nzz,1).*repmat(p,1,Nint));
        
        % substraction term
        Ivh_Bsub_tmp_sub = repmat(1./Zh_kp,Nzz,1)/2.*(Bsub_h0-Bsub_h1-Bsub_h2+Bsub_h3+Bsub_h4);
        Ive_Bsub_tmp_sub = repmat(1./Ze_kp,Nzz,1)/2.*(Bsub_e0-Bsub_e1-Bsub_e2+Bsub_e3+Bsub_e4);
        Ive_Asub_tmp_sub = repmat(1./Ze(id_s,ind_int),Nzz,1)/2.*(Asub_e0-Asub_e1-Asub_e2+Asub_e3+Asub_e4);
        
        Ive_tmp_sub = repmat(1./Ze(id_s,ind_int),Nzz,1)/2.*(Ae0-Ae1-Ae2+Ae3+Ae4);
        
        Ivh_J0_tmp = (repmat(1./Zh(id_s,ind_int),Nzz,1)/2.*(Ah0-Ah1-Ah2+Ah3+Ah4) ...
            -Ivh_Bsub_tmp_sub) .*J0_tmp./repmat(kp_int,Nzz,1)*dkp;
        Ive_J0_tmp_A = (Ive_tmp_sub - Ive_Asub_tmp_sub) .* J0_tmp.*repmat(kp_int,Nzz,1)*dkp;
        Ive_J0_tmp_B = (repmat(kz_int(id_s,1:Nint).^2,Nzz,1) .* Ive_tmp_sub ...
            - (-repmat(kp_int.^2,Nzz,1)).*Ive_Bsub_tmp_sub) .* J0_tmp./repmat(kp_int,Nzz,1)*dkp;
        
        Gxx_tmp1(ig) = sum((k0^2*mur_lyr(id_s)*mur_lyr(id_s) .* Ivh_J0_tmp(1,:) ...
            + mur_lyr(id_s)/epr_lyr(id_s).*Ive_J0_tmp_A(1,:) ...
            -mur_lyr(id_s)/epr_lyr(id_s).*Ive_J0_tmp_B(1,:)).*gau_wei ,2);
        Gxx_tmpN(ig) = sum((k0^2*mur_lyr(id_s)*mur_lyr(id_s) .* Ivh_J0_tmp(Nxx,:) ...
            + mur_lyr(id_s)/epr_lyr(id_s).*Ive_J0_tmp_A(Nxx,:) ...
            -mur_lyr(id_s)/epr_lyr(id_s).*Ive_J0_tmp_B(Nxx,:)).*gau_wei ,2);
        
        % as field and source points are in the same layer, medium
        % parameters are the same
        Gzz_num_new = Gzz_num + sum( (k0^2*mur_lyr(id_s)*mur_lyr(id_s) .* Ivh_J0_tmp ...
            + mur_lyr(id_s)/epr_lyr(id_s).*Ive_J0_tmp_A ...
            - mur_lyr(id_s)/epr_lyr(id_s).*Ive_J0_tmp_B).*repmat(gau_wei,Nzz,1), 2);
        
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
    
    Gxx_tmp1=[];Gxx_tmpN=[];
    % Gphi
    Nstep_min = min(min(Nstep_Gxx,Nstep_Gzx),Nstep_Gzz);
    for ig = 1:Nstep_min  % Nkp-1
        ind_int = (ig-1)*Nint+(1:Nint);
        
        kp_int = kp_tot(ind_int);
        kz_int = kz_tot(1:Nlyr,ind_int);
        % wave impedance when kp->infinity
        %         Zh_kp = (w0*mu_lyr(id_ls))./(-1j*kp_int);
        Ze_kp = (-1j*kp_int)./(w0*ep_lyr(id_s));
        
        [Ah0,Ah1,Ah2,Ah3,Ah4] = dgf_kernal_A(dz0(indphi),dz1(indphi),dz2(indphi),dz3(indphi),dz4(indphi),...
            Dh(id_s,ind_int),GRh_ij(id_s,ind_int),GRh_ji(id_s-1,ind_int), ...
            kz_int(id_s,1:Nint));
        [Ae0,Ae1,Ae2,Ae3,Ae4] = dgf_kernal_A(dz0(indphi),dz1(indphi),dz2(indphi),dz3(indphi),dz4(indphi),...
            De(id_s,ind_int),GRe_ij(id_s,ind_int),GRe_ji(id_s-1,ind_int), ...
            kz_int(id_s,1:Nint));
        
        %         [Bsub_h0,Bsub_h1,Bsub_h2,Bsub_h3,Bsub_h4] = dgf_kernal_Bsub(dz0,dz1,dz2,dz3,dz4,...
        %             Rh_ij_inf(id_ls),Rh_ji_inf(id_ls-1),kp_int);
        [Bsub_e0,Bsub_e1,Bsub_e2,Bsub_e3,Bsub_e4] = dgf_kernal_Bsub...
            (dz0(indphi),dz1(indphi),dz2(indphi),dz3(indphi),dz4(indphi),...
            Re_ij_inf(id_s),Re_ji_inf(id_s-1),kp_int);
        
        J0_tmp = besselj(0,repmat(kp_int,Nphi,1).*repmat(p,1,Nint));
        %         J1_tmp = besselj(1,repmat(kp_int,Nf,1).*repmat(p,1,Nint));
        
        % substraction term
        Vie_Bsub_tmp_sub = repmat(Ze_kp,Nphi,1)/2.*(Bsub_e0+Bsub_e1+Bsub_e2+Bsub_e3+Bsub_e4);
        %         Vih_Bsub_tmp_sub = repmat(Zh_kp,Nf,1)/2.*(Bsub_h0+Bsub_h1+Bsub_h2+Bsub_h3+Bsub_h4);
        
        Vie_J0_kp_tmp = (repmat(Ze(id_s,ind_int),Nphi,1)/2.*(Ae0+Ae1+Ae2+Ae3+Ae4) ...
            -Vie_Bsub_tmp_sub) .* J0_tmp./repmat(kp_int,Nphi,1)*dkp;
        %         Vih_J0_kp_tmp = (Vih_tmp_sub-Vih_Bsub_tmp_sub) .* J0_tmp./repmat(kp_int,Nf,1)*dkp;
        Vih_J0_kp_tmp = (repmat(Zh(id_s,ind_int),Nphi,1)/2.*(Ah0+Ah1+Ah2+Ah3+Ah4)) ...
            .* J0_tmp./repmat(kp_int,Nphi,1)*dkp;
        
        Gxx_tmp1(ig) = sum((Vie_J0_kp_tmp(1,:)-Vih_J0_kp_tmp(1,:)).*gau_wei ,2);
        Gxx_tmpN(ig) = sum((Vie_J0_kp_tmp(Nxx,:)-Vih_J0_kp_tmp(Nxx,:)).*gau_wei ,2);
        
        %cof_Gphi = cof_Gphi + sum(1./repmat(kp_int.^2,Nf,1).*repmat(gau_wei,Nf,1) ,2);
        Gphi_num_new = Gphi_num + sum((Vie_J0_kp_tmp-Vih_J0_kp_tmp).*repmat(gau_wei,Nphi,1) ,2);
        
        if abs(Gphi_num_new-Gphi_num)<(abs(Gphi_num)*err_tol)
            disp(['Gphi -- computing step:  ',num2str(ig)]);
            break;
        elseif ig == Nstep_min && ig ~= Nkp-1
            disp(['Gphi -- computing step:  ',num2str(ig)]);
            break;
        elseif ig == Nkp-1
            disp(['Gphi -- comupting step:  ',num2str(ig),'  Did not converge!']);
        else
            Gphi_num = Gphi_num_new;
        end
        
    end
    
    
    [At_h0,At_h1,At_h2,At_h3,At_h4] = dgf_tail_A...
        (dz0(indxx),dz1(indxx),dz2(indxx),dz3(indxx),dz4(indxx),p(indxx), ...
        Rh_ij_inf(id_s),Rh_ji_inf(id_s-1), kn(id_s));
    Gxx_tail = mur_lyr(id_s)/2 * (At_h0+At_h1+At_h2+At_h3+At_h4);
    
    
    [Bt_h0,Bt_h1,Bt_h2,Bt_h3,Bt_h4] = dgf_tail_B...
        (dz0(indzx),dz1(indzx),dz2(indzx),dz3(indzx),dz4(indzx),p(indzx), ...
        Rh_ij_inf(id_s),Rh_ji_inf(id_s-1));
    [Bt_e0,Bt_e1,Bt_e2,Bt_e3,Bt_e4] = dgf_tail_B...
        (dz0(indzx),dz1(indzx),dz2(indzx),dz3(indzx),dz4(indzx),p(indzx), ...
        Re_ij_inf(id_s),Re_ji_inf(id_s-1));
    sign_A0 = sign(pt_f(indzx,3)-zs(ik));
    sign_A0(sign_A0==0) = 1;
    Gzx_tail = sign_A0.*Bt_h0-Bt_h1+Bt_h2+Bt_h3-Bt_h4 ...
        - sign_A0.*Bt_e0+Bt_e1-Bt_e2-Bt_e3+Bt_e4;
        
    
    
    [Ct_h0,Ct_h1,Ct_h2,Ct_h3,Ct_h4] = dgf_tail_C...
        (dz0(indzz),dz1(indzz),dz2(indzz),dz3(indzz),dz4(indzz),p(indzz), ...
        Rh_ij_inf(id_s),Rh_ji_inf(id_s-1));
    [Ct_e0,Ct_e1,Ct_e2,Ct_e3,Ct_e4] = dgf_tail_C...
        (dz0(indzz),dz1(indzz),dz2(indzz),dz3(indzz),dz4(indzz),p(indzz), ...
        Re_ij_inf(id_s),Re_ji_inf(id_s-1));
    [At_e0,At_e1,At_e2,At_e3,At_e4] = dgf_tail_A...
        (dz0(indzz),dz1(indzz),dz2(indzz),dz3(indzz),dz4(indzz),p(indzz), ...
        Re_ij_inf(id_s),Re_ji_inf(id_s-1), kn(id_s));

    Gzz_tail = -mur_lyr(id_s)/2 * (Ct_h0-Ct_h1-Ct_h2+Ct_h3+Ct_h4) ...
        + mur_lyr(id_s)/2 .* ( At_e0-At_e1-At_e2+At_e3+At_e4) ...
        + mur_lyr(id_s)/2 .* (Ct_e0-Ct_e1-Ct_e2+Ct_e3+Ct_e4);  % Fixed: epr_lyr(id_s)./(2*epr_lyr(id_s)) = 1/2
    
    %     [Dt_e0,Dt_e1,Dt_e2,Dt_e3,Dt_e4] = dgf_tail_D...
    %         (dz0,dz1,dz2,dz3,dz4,p, Re_ij(id_ls,end),Re_ji(id_ls-1,end));
    %   + w0*mu0.*mur_lyr(id_ls)./(2*1j)
%     Gphi_tail = (-1j./(2*w0*ep0*epr_lyr(id_s))).*(Ct_e0+Ct_e1+Ct_e2+Ct_e3+Ct_e4);
    [Ct_e0,Ct_e1,Ct_e2,Ct_e3,Ct_e4] = dgf_tail_C...
        (dz0(indphi),dz1(indphi),dz2(indphi),dz3(indphi),dz4(indphi),p(indphi), ...
        Re_ij_inf(id_s),Re_ji_inf(id_s-1));
    Gphi_tail = 1./(2*epr_lyr(id_s)).*(Ct_e0+Ct_e1+Ct_e2+Ct_e3+Ct_e4);

    
    Gxx(ik,indxx) = 1/(2*pi) * (Gxx_tail + 1/(1j*w0*mu0) .* Gxx_num);
    Gzx(ik,indzx) = 1/(2*pi) * 1/(1j*w0*mu0) * -mur_lyr(id_s) * ( Gzx_tail + Gzx_num ); % here kx=kn
    Gzz(ik,indzz) = 1/(2*pi) * ( Gzz_tail + 1/(1j*w0*ep0) * Gzz_num );
    Gphi(ik,indphi) = 1/(2*pi) * ( Gphi_tail + 1j*w0*ep0 * Gphi_num );
    
end


end


