function [GRe_ij,GRe_ji,De,Dh] = dgf_gref_cof_backup(kz, ep_lyr, mu_lyr, id_lyr_s, Nbdy,w0)


Ze1 = dgf_Ze(kz(x), ep_lyr(Nbdy), w0);
Ze2 = dgf_Ze(kz(x), ep_lyr(Nbdy-1), w0);
Re_12 = dgf_Rij(Ze1(x), Ze2(x));

Zh1 = dgf_Zh(kz(x), ep_lyr(1), w0);
Zh2 = dgf_Zh(kz(x), ep_lyr(2), w0);
Rh_12 = dgf_Rij(Zh1(x), Zh2(x));

GRe_ij = Re_12(x);
GRh_ij = Rh_12(x);

for ig = Nbdy-1:-1:id_lyr_s
    Zei = dgf_Ze(kz(x), ep_lyr(ig), w0);
    Zej = dgf_Ze(kz(x), ep_lyr(ig), w0);
    Re_ij = dgf_Rij(Zei(x), Zej(x));
    
    Zhi = dgf_Zh(kz(x), mu_lyr(ig), w0);
    Zhj = dgf_Zh(kz(x), mu_lyr(ig), w0);
    Rh_ij = dgf_Rij(Zhi(x), Zhj(x));
    
    kzz = @(x) sqrt(kn(ig+1).^2 - (x).^2);
    
    GRe_ij = dgf_bdy_gcof_sub(Re_ij(x),GRe_ij(x), kzz(x), d_lyr(ig+1) );
    GRh_ij = dgf_bdy_gcof_sub(Rh_ij(x),GRh_ij(x), kzz(x), d_lyr(ig+1) );
end


% backward layer recursion (recursion from the 1st layer)
Ze1 = dgf_Ze(kz(x), mu_lyr(1), w0);
Ze2 = dgf_Ze(kz(x), mu_lyr(2), w0);
Re_21 = dgf_Re(Ze2(x), Ze1(x));

Zh1 = dgf_Ze(kz(x), mu_lyr(1), w0);
Zh2 = dgf_Ze(kz(x), mu_lyr(2), w0);
Rh_21 = dgf_Rh(Zh2(x), Zh1(x));

GRe_ji = Re_21(x);
GRh_ji = Rh_21(x);


for ig = 2:Nbdy
    Zei = dgf_Ze(kz(x), ep_lyr(ig), w0);
    Zej = dgf_Ze(kz(x), ep_lyr(ig), w0);
    Re_ji = dgf_Rij(Zej(x), Zei(x));
    
    Zhi = dgf_Zh(kz(x), mu_lyr(ig), w0);
    Zhj = dgf_Zh(kz(x), mu_lyr(ig), w0);
    Rh_ji = dgf_Rij(Zhj(x), Zhi(x));
    
    kzz = @(x) sqrt(kn(ig).^2 - (x).^2);
    
    GRe_ji = dgf_bdy_gcof_sub(Re_ji(x),GRe_ji(x), kz(x), d_lyr(ig) );
    GRh_ji = dgf_bdy_gcof_sub(Rh_ji(x),GRh_ji(x), kz(x), d_lyr(ig) );
end

De = 1 - GRe_ij(x).*GRe_ji(x)...
    .*exp(-2*1j*kz(x).*d_lyr(id_lyr_s));

Dh = 1 - GRh_ij(x).*GRh_ji(x)...
    .*exp(-2*1j*kz(2:Nlyr-1,1:Np).*d_lyr(id_lyr_s));

end