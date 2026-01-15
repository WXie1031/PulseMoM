%function int = dgf_kernal_1(pt_s,dv_s, pt_f,dv_f, zbdy, Zx, Dx, Rx_ij,Rx_ji,GRx_ij, GRx_ji, kz, kp)
%  Function:       dgf_kernal_1
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

zs = pt_s(:,3);
zf = pt_f(:,3);

Ns = size(pt_s,1);
Nf = size(pt_f,1);

%idx_s
%idy_s
%idz_s


for ik = 1:Ns
    
    id_lyr_s = dgf_locate_lyr(zs(ik), zbdy);
    id_lyr_f = dgf_locate_lyr(zf(1:Nf), zbdy);
    
    p = sqrt( (pt_s(ik,1)-pt_f(1:Nf,1)).^2 + (pt_s(ik,2)-pt_f(1:Nf,2)).^2 );
    
    dz0 = abs(zs(ik)-zf(1:Nf));
    
    dz1 = zeros(Nf,1);
    dz2 = zeros(Nf,1);
    dz3 = zeros(Nf,1);
    dz4 = zeros(Nf,1);
    
    % x indicates kp
    kz = @(x) sqrt(kn(id_lyr_s).^2 - (x).^2);
    
    Ze = @(x) dgf_Ze(x, ep_lyr, w0);
    Zh = @(x) dgf_Zh(x, mu_lyr, w0);
    
    dgf_Ze(kz, ep_lyr);
    
    Re_ij = (Ze(1:Nlyr-1,1:Np)-Ze(2:Nlyr,1:Np))./(Ze(2:Nlyr,1:Np)+Ze(1:Nlyr-1,1:Np));
    Re_ji = -Re_ij;
    
    Rh_ij = (Zh(1:Nlyr-1,1:Np)-Zh(2:Nlyr,1:Np))./(Zh(2:Nlyr,1:Np)+Zh(1:Nlyr-1,1:Np));
    Rh_ji = -Rh_ij;
    
    
    % forward layer recursion
    GRe_ij(Nbdy,1:Np) = Re_ij(Nbdy,1:Np);
    GRh_ij(Nbdy,1:Np) = Rh_ij(Nbdy,1:Np);
    for ik = Nbdy-1:-1:1
        GRe_ij(ik,1:Np) = dgf_bdy_gcof_sub(Re_ij(ik,1:Np),GRe_ij(ik+1,1:Np), kz(ik+1,1:Np), d_lyr(ik+1) );
        GRh_ij(ik,1:Np) = dgf_bdy_gcof_sub(Rh_ij(ik,1:Np),GRh_ij(ik+1,1:Np), kz(ik+1,1:Np), d_lyr(ik+1) );
    end
    
    % backward layer recursion
    GRe_ji(1,1:Np) = Re_ji(1,1:Np);
    GRh_ji(1,1:Np) = Rh_ji(1,1:Np);
    for ik = 2:Nbdy
        GRe_ji(ik,1:Np) = dgf_bdy_gcof_sub(Re_ji(ik,1:Np),GRe_ji(ik-1,1:Np), kz(ik,1:Np), d_lyr(ik) );
        GRh_ji(ik,1:Np) = dgf_bdy_gcof_sub(Rh_ji(ik,1:Np),GRh_ji(ik-1,1:Np), kz(ik,1:Np), d_lyr(ik) );
    end
    
    
    
    if id_lyr_s==1
        idz_lyr1 = (id_lyr_f==1);
        
        dz1 = 2*zbdy(1)-(zs(ik)+zf);
        
        
    elseif id_lyr_s==Nlyr
        
        
    else
        % source and field in the same layer
        idf_same = (id_lyr_f==id_lyr_s);
        
        dz1(idf_same) = 2*zbdy(id_lyr_s-1) - (zf(idf_same)+zs(ik));
        dz2(idf_same) = (zf(idf_same)+zs(ik)) - 2*zbdy(id_lyr_s);
        dz3(idf_same) = 2*d_lyr(id_lyr_s) + (zf(idf_same)-zs(ik));
        dz4(idf_same) = 2*d_lyr(id_lyr_s) - (zf(idf_same)-zs(ik));
        
        % field in the front layer of source
        idf_adj = (id_lyr_f>id_lyr_s);
        
        zf_tmp = zbdy(id_lyr_s);
        
        dz1(idf_adj) = 2*zbdy(ik) - (zf_tmp+zs(ik));
        dz2(idf_adj) = (zf_tmp+zs(ik)) - 2*zbdy(id_lyr_s-1);
        dz3(idf_adj) = 2*d_lyr(id_lyr_s) + (zf_tmp-zs(ik));
        dz4(idf_adj) = 2*d_lyr(id_lyr_s) - (zf_tmp-zs(ik));
        
        % source and field in the back layer of source
        idf_oth = (id_lyr_f<id_lyr_s);
        
        zf_tmp = zbdy(id_lyr_s-1);
        
        dz1(idf_oth) = 2*zbdy(id_lyr_s) - (zf_tmp+zs(ik));
        dz2(idf_oth) = (zf_tmp+zs(ik)) - 2*zbdy(id_lyr_s-1);
        dz3(idf_oth) = 2*d_lyr(id_lyr_s) + (zf_tmp-zs(ik));
        dz4(idf_oth) = 2*d_lyr(id_lyr_s) - (zf_tmp-zs(ik));
        
        
        
        % forward layer recursion   (recursion from the last layer)

    
    
    idAe = id_lyr_s*ones(Ns,1);
        Ae0 = exp(-1j*repmat(kz(idAe,1:Np),Ns,1).*(dz0*ones(1,Np)));
        Ae1 = GRe_ij(idAe,1:Np)./De(idAe,1:Np).*exp(-1j*repmat(kz(idAe),Ns,1).*(dz1*ones(1,Np)));
        Ae2 = GRe_ji(idAe,1:Np)./De(idAe,1:Np).*exp(-1j*repmat(kz(idAe),Ns,1).*(dz2*ones(1,Np)));
        Ae3 = GRe_ij(idAe,1:Np).*GRe_ji(idAe-1,1:Np)./De(idAe,1:Np).*exp(-1j*repmat(kz(idAe,1:Np),Ns,1).*(dz3));
        Ae4 = GRe_ij(idAe,1:Np).*GRe_ji(idAe-1,1:Np)./De(idAe,1:Np).*exp(-1j*repmat(kz(idAe,1:Np),Ns,1).*(dz4));
        
    
    Vei = (Ze(id_lyr_s,:)/2.*(Ae0+Ae1+Ae2+0+0).*besselj(0,kp.*p).*kp)*dkp;
    
    %Kxx =
    figure(10);
    subplot(2,2,1)
    hold on
    plot(real(kp),sign(real(Ae0)).*log10(abs(real(Ae0))))
    plot(real(kp),sign(real(Ae1)).*log10(abs(real(Ae1))))
    plot(real(kp),sign(real(Ae2)).*log10(abs(real(Ae2))))
    plot(real(kp),sign(real(Ae3)).*log10(abs(real(Ae3))))
    plot(real(kp),sign(real(Ae4)).*log10(abs(real(Ae4))))
    
    subplot(2,2,2)
    hold on
    plot(real(kp),sign(imag(Ae0)).*log10(abs(imag(Ae0))))
    plot(real(kp),sign(imag(Ae1)).*log10(abs(imag(Ae1))))
    plot(real(kp),sign(imag(Ae2)).*log10(abs(imag(Ae2))))
    plot(real(kp),sign(imag(Ae3)).*log10(abs(imag(Ae3))))
    plot(real(kp),sign(imag(Ae4)).*log10(abs(imag(Ae4))))
    
    subplot(2,2,3)
    plot(real(kp),sign(real(Vei)).*log10(abs(real(Vei))))
    subplot(2,2,4)
    plot(real(kp),sign(imag(Vei)).*log10(abs(imag(Vei))))
    
    
    
    
    
    
    
end

