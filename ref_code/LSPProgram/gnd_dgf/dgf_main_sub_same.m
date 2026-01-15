function [Gxx,Gzx,Gzz,Gphi] = dgf_main_sub_same(...
    Zh,Dh,GRh_ij,GRh_ji, Ze,De,GRe_ij,GRe_ji, ...
    Rh_ij_inf,Rh_ji_inf,Re_ij_inf,Re_ji_inf, kz_tot,kp_tot, kp,dkp, ...
    zbdy, d_lyr, epr_lyr,mur_lyr, pt_s,dv_s, pt_f,dv_f, gau_nod,gau_wei, w0)
    
%  Function:       dgf_main_sub_same
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



zs = pt_s(:,3);
zf = pt_f(:,3);

Ns = size(pt_s,1);
Nf = size(pt_f,1);
Nlyr = size(epr_lyr,1);

%% calculate the integration

Gxx = zeros(Ns,Nf);
Gzx = zeros(Ns,Nf);
Gzz = zeros(Ns,Nf);
Gphi = zeros(Ns,Nf);
for ik = 1:Ns

    dz0 = abs(zs(ik)-zf(1:Nf));
    
    id_s = dgf_locate_lyr(zs(ik), zbdy);

    if id_s==1  % in the first layer
        
        %dz1 = 2*zbdy(id_s-1) - (zf+zs(ik));
        dz2 = (zf+zs(ik)) - 2*zbdy(id_s);
        %dz3 = 2*d_lyr(id_s) + (zf-zs(ik));
        %dz4 = 2*d_lyr(id_s) - (zf-zs(ik));
    
        [Gxx(ik,1:Nf),Gzx(ik,1:Nf),Gzz(ik,1:Nf),Gphi(ik,1:Nf)] = ...
            dgf_main_sub_same_sub_1(Zh,GRh_ij,GRh_ji, Ze,GRe_ij,GRe_ji, ...
            Rh_ij_inf,Rh_ji_inf,Re_ij_inf,Re_ji_inf, kz_tot,kp_tot, kp,dkp, ...
            zbdy, epr_lyr,mur_lyr, ...
            pt_s,dv_s, pt_f,dv_f,dz0,dz2,gau_nod,gau_wei, w0); 

    elseif id_s==Nlyr  % in the last layer
        dz1 = 2*zbdy(id_s-1) - (zf+zs(ik));
        dz2 = (zf+zs(ik)) - 2*zbdy(id_s);
        dz3 = 2*d_lyr(id_s) + (zf-zs(ik));
        dz4 = 2*d_lyr(id_s) - (zf-zs(ik));
        
        [Gxx(ik,1:Nf),Gzx(ik,1:Nf),Gzz(ik,1:Nf),Gphi(ik,1:Nf)] = ...
            dgf_main_sub_same_sub_n(Zh,Dh,GRh_ij,GRh_ji, Ze,De,GRe_ij,GRe_ji, ...
            Rh_ij_inf,Rh_ji_inf,Re_ij_inf,Re_ji_inf, kz_tot,kp_tot, kp,dkp, ...
            zbdy, epr_lyr,mur_lyr,sig_lyr, ...
            pt_s,dv_s, pt_f,dv_f,dz0,dz1,dz2,dz3,dz4,gau_nod,gau_wei, w0); 
            
    else  % in middle layers
        dz1 = 2*zbdy(id_s-1) - (zf+zs(ik));
        dz2 = (zf+zs(ik)) - 2*zbdy(id_s);
        dz3 = 2*d_lyr(id_s) + (zf-zs(ik));
        dz4 = 2*d_lyr(id_s) - (zf-zs(ik));
        
%         dz2 = 2*zbdy(id_s-1) - (zf+zs(ik));
%         dz1 = (zf+zs(ik)) - 2*zbdy(id_s);
%         dz4 = 2*d_lyr(id_s) + (zf-zs(ik));
%         dz3 = 2*d_lyr(id_s) - (zf-zs(ik));
        
        [Gxx(ik,1:Nf),Gzx(ik,1:Nf),Gzz(ik,1:Nf),Gphi(ik,1:Nf)] = ...
            dgf_main_sub_same_sub_i(Zh,Dh,GRh_ij,GRh_ji, Ze,De,GRe_ij,GRe_ji, ...
            Rh_ij_inf,Rh_ji_inf,Re_ij_inf,Re_ji_inf, kz_tot,kp_tot, kp,dkp, ...
            zbdy, epr_lyr,mur_lyr, pt_s,dv_s, pt_f,dv_f,...
            dz0,dz1,dz2,dz3,dz4,gau_nod,gau_wei, w0); 

    end

    
end



end


