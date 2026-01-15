function [Gxx,Gzx,Gzz,Gphi] = dgf_main_num_same(Zh,Dh,GRh_ij,GRh_ji, Ze,De,GRe_ij,GRe_ji, ...
    kz_tot,kp_tot, kp,dkp, zbdy, epr_lyr,mur_lyr, ...
    pt_s,dv_s, pt_f,dv_f, dz0,dz1,dz2,dz3,dz4,...
    gau_nod,gau_wei, w0)
%  Function:       dgf_main_num_same
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



%% basic parameters of layers

Nint = length(gau_nod); % order of the Guassion integration 
Nkp = length(kp);

vc = 3e8;
mu0 = 4e-7*pi;
ep0 = 8.854187817e-12;

k0 = w0*sqrt(ep0.*mu0);

Nbdy = size(zbdy,1);
Nlyr = Nbdy+1;

Ns = size(pt_s,1);
Nf = size(pt_f,1);

zs = pt_s(1:Ns,3);
zf = pt_f(1:Nf,3);

%% calculate the integration
Gxx = zeros(Ns,Nf);
Gzx = zeros(Ns,Nf);
Gzz = zeros(Ns,Nf);
Gphi = zeros(Ns,Nf);
for ik = 1:Ns
    
    Gxx_num = 0;
    Gzx_num = 0;
    Gzz_num = 0;
    Gphi_num = 0;

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
    
    for ig = 1:Nkp-1
        ind_int = (ig-1)*Nint+(1:Nint);
        
        kp_int = kp_tot(ind_int);
        kz_int = kz_tot(1:Nlyr,ind_int);

        
        [Ah0,Ah1,Ah2,Ah3,Ah4] = dgf_kernal_A(dz0,dz1,dz2,dz3,dz4,...
            Dh(id_s,ind_int),GRh_ij(id_s,ind_int),GRh_ji(id_s-1,ind_int), ...
            kz_int(id_s,1:Nint));
        
        [Ae0,Ae1,Ae2,Ae3,Ae4] = dgf_kernal_A(dz0,dz1,dz2,dz3,dz4,...
            De(id_s,ind_int),GRe_ij(id_s,ind_int),GRe_ji(id_s-1,ind_int), ...
            kz_int(id_s,1:Nint));

        J0_tmp = besselj(0,repmat(kp_int,Nf,1).*repmat(p,1,Nint));
        J1_tmp = besselj(1,repmat(kp_int,Nf,1).*repmat(p,1,Nint));

        dkp_seg = (kp(ig+1)-kp(ig))/2;

        % Gxx
        Vih_J0_tmp = repmat(Zh(id_s,ind_int),Nf,1)/2.*(Ah0+Ah1+Ah2+Ah3+Ah4) ...
            .*J0_tmp.*repmat(kp_int,Nf,1)*dkp_seg;

        Gxx_num = Gxx_num + sum(Vih_J0_tmp.*repmat(gau_wei,Nf,1) ,2);

        % Gzx
        sign_A0 = sign(zf-zs(ik));
        sign_A0(sign_A0==0) = 1;
        sign_A0 = repmat(sign_A0,1,Nint);

        Gzx_tmp = 1/2*((sign_A0.*Ah0-Ah1+Ah2+Ah3-Ah4)-(sign_A0.*Ae0-Ae1+Ae2+Ae3-Ae4)) ...
            .*J1_tmp.*repmat(kp_int.^2,Nf,1)*dkp_seg;

        Gzx_num = Gzx_num + sum( Gzx_tmp.*repmat(gau_wei,Nf,1) ,2 );

        % Gzz
        Ivh_J0_tmp = (repmat(1./Zh(id_s,ind_int),Nf,1)/2.*(Ah0-Ah1-Ah2+Ah3+Ah4)) ...
             .* J0_tmp./repmat(kp_int,Nf,1)*dkp_seg;
        Ive_tmp_sub = repmat(1./Ze(id_s,ind_int),Nf,1)/2.*(Ae0-Ae1-Ae2+Ae3+Ae4);
        Ive_J0_tmp_A = Ive_tmp_sub .* J0_tmp.*repmat(kp_int,Nf,1)*dkp_seg;
        Ive_J0_tmp_B = repmat(kz_int(id_s,1:Nint).^2,Nf,1) .* Ive_tmp_sub ...
            .* J0_tmp./repmat(kp_int,Nf,1)*dkp_seg;

        % as field and source points are in the same layer, medium
        % parameters are the same
        Gzz_num = Gzz_num + sum( (k0^2*mur_lyr(id_s)*mur_lyr(id_s) .* Ivh_J0_tmp ...
            + mur_lyr(id_s)/epr_lyr(id_s).*Ive_J0_tmp_A ...
            - mur_lyr(id_s)./epr_lyr(id_s).*Ive_J0_tmp_B).*repmat(gau_wei,Nf,1), 2);

        % Gphi
        Gphi_tmp = ( repmat(Ze(id_s,ind_int),Nf,1)/2.*(Ae0+Ae1+Ae2+Ae3+Ae4) ...
            - repmat(Zh(id_s,ind_int),Nf,1)/2.*(Ah0+Ah1+Ah2+Ah3+Ah4) )...
            .* J0_tmp./repmat(kp_int,Nf,1)*dkp_seg;

        Gphi_num = Gphi_num + sum(Gphi_tmp.*repmat(gau_wei,Nf,1) ,2);
        
    end


    Gxx(ik,indxx) = 1/(2*pi) * 1/(1j*w0*mu0) * Gxx_num;
    Gzx(ik,indzx) = 1/(2*pi) * 1/(1j*w0*mu0) * -mur_lyr(id_s) .* Gzx_num; % here kx=kn
    Gzz(ik,indzz) = 1/(2*pi) * 1/(1j*w0*ep0) * Gzz_num;
    Gphi(ik,indphi) = 1/(2*pi) * 1j*w0*ep0 * Gphi_num;
    
end



end


