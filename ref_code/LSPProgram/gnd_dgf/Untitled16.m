% the location of the points 
idAe = ones(Ns,1);
Ae0 = exp(-1j*repmat(kz(idAe,1:Np),Ns,1).*(dz0*ones(1,Np)));
Ae1 = GRe_ij(idAe,1:Np)./De(idAe,1:Np).*exp(-1j*repmat(kz(idAe,1:Np),Ns,1).*(dz1*ones(1,Np)));
Ae2 = GRe_ji(idAe,1:Np)./De(idAe,1:Np).*exp(-1j*repmat(kz(idAe,1:Np),Ns,1).*(dz2*ones(1,Np)));
Ae3 = GRe_ij(idAe,1:Np).*GRe_ji(idAe,1:Np)./De(idAe,1:Np).*exp(-1j*repmat(kz(idAe,1:Np),Ns,1).*(dz3*ones(1,Np)));
Ae4 = GRe_ij(idAe,1:Np).*GRe_ji(idAe,1:Np)./De(idAe,1:Np).*exp(-1j*repmat(kz(idAe,1:Np),Ns,1).*(dz4*ones(1,Np)));


idAh = ones(Ns,1);
Ah0 = exp(-1j*ones(Ns,1)*kz(idAh,1:Np).*(dz0*ones(1,Np)));
Ah1 = GRh_ij(idAh,1:Np)./Dh(idAh,1:Np).*exp(-1j*ones(Ns,1)*kz(idAh,1:Np).*(dz1*ones(1,Np)));
Ah2 = GRh_ji(idAh,1:Np)./Dh(idAh,1:Np).*exp(-1j*ones(Ns,1)*kz(idAh,1:Np).*(dz2*ones(1,Np)));
Ah3 = GRh_ij(idAh,1:Np).*GRh_ji(idAh,1:Np)./Dh(idAh,1:Np).*exp(-1j*ones(Ns,1)*kz(idAh,1:Np).*(dz3*ones(1,Np)));
Ah4 = GRh_ij(idAh,1:Np).*GRh_ji(idAh,1:Np)./Dh(idAh,1:Np).*exp(-1j*ones(Ns,1)*kz(idAh,1:Np).*(dz4*ones(1,Np)));


idBe = ones(Ns,1);
Be0 = exp(-1j*ones(Ns,1)*kz(idBe,1:Np).*(dz0*ones(1,Np)));
Be1 = GRe_ij(idBe,1:Np)./De(idBe,1:Np).*exp(-1j*ones(Ns,1)*kp(idBe,1:Np).*(dz1*ones(1,Np)));
Be2 = GRe_ji(idBe,1:Np)./De(idBe,1:Np).*exp(-1j*ones(Ns,1)*kp(idBe,1:Np).*(dz2*ones(1,Np)));
Be3 = GRe_ij(idBe,1:Np).*GRe_ji(idBe,1:Np)./De(idBe,1:Np).*exp(-1j*ones(Ns,1)*kp(idBe,1:Np).*(dz3*ones(1,Np)));
Be4 = GRe_ij(idBe,1:Np).*GRe_ji(idBe,1:Np)./De(idBe,1:Np).*exp(-1j*ones(Ns,1)*kp(idBe,1:Np).*(dz4*ones(1,Np)));

idBh = ones(Ns,1);

Bh0 = exp(-1j*ones(Ns,1)*kz(idBh,1:Np).*(dz0*ones(1,Np)));
Bh1 = GRh_ij(idBh,1:Np)./Dh(idBh,1:Np).*exp(-1j*ones(Ns,1)*kp(idBh,1:Np).*(dz1*ones(1,Np)));
Bh2 = GRh_ji(idBh,1:Np)./Dh(idBh,1:Np).*exp(-1j*ones(Ns,1)*kp(idBh,1:Np).*(dz2*ones(1,Np)));
Bh3 = GRh_ij(idBh,1:Np).*GRh_ji(idBh,1:Np)./Dh(idBh,1:Np).*exp(-1j*ones(Ns,1)*kp(idBh,1:Np).*(dz3*ones(1,Np)));
Bh4 = GRh_ij(idBh,1:Np).*GRh_ji(idBh,1:Np)./Dh(idBh,1:Np).*exp(-1j*ones(Ns,1)*kp(idBh,1:Np).*(dz4*ones(1,Np)));




Ae_sub0 = exp(-1j*ones(Ns,1)*kz(idAe,1:Np).*(dz0*ones(1,Np)));
Ae_sub1 = Re_ij(idAe,1:Np).*exp(-1j*ones(Ns,1)*kz(idAe,1:Np).*(dz1*ones(1,Np)));
Ae_sub2 = Re_ji(idAe,1:Np).*exp(-1j*ones(Ns,1)*kz(idAe,1:Np).*(dz2*ones(1,Np)));
Ae_sub3 = Re_ij(idAe,1:Np).*Re_ji(idAe,1:Np).*exp(-1j*ones(Ns,1)*kz(idAe,1:Np).*(dz3*ones(1,Np)));
Ae_sub4 = Re_ij(idAe,1:Np).*Re_ji(idAe,1:Np).*exp(-1j*ones(Ns,1)*kz(idAe,1:Np).*(dz4*ones(1,Np)));



idBe = ones(Ns,1);
Be_sub0 = exp(-1j*ones(Ns,1)*kz(idBe,1:Np).*(dz0*ones(1,Np)));
Be_sub1 = Re_ij(idBe,1:Np).*exp(-1j*ones(Ns,1)*kp(idBe,1:Np).*(dz1*ones(1,Np)));
Be_sub2 = Re_ji(idBe,1:Np).*exp(-1j*ones(Ns,1)*kp(idBe,1:Np).*(dz2*ones(1,Np)));
Be_sub3 = Re_ij(idBe,1:Np).*Re_ji(idBe,1:Np).*exp(-1j*ones(Ns,1)*kp(idBe,1:Np).*(dz3*ones(1,Np)));
Be_sub4 = Re_ij(idBe,1:Np).*Re_ji(idBe,1:Np).*exp(-1j*ones(Ns,1)*kp(idBe,1:Np).*(dz4*ones(1,Np)));

idBh = ones(Ns,1);

Bh_sub0 = exp(-1j*ones(Ns,1)*kz(idBh,1:Np).*(dz0*ones(1,Np)));
Bh_sub1 = Rh_ij(idBh,1:Np).*exp(-1j*ones(Ns,1)*kp(idBh,1:Np).*(dz1*ones(1,Np)));
Bh_sub2 = Rh_ji(idBh,1:Np).*exp(-1j*ones(Ns,1)*kp(idBh,1:Np).*(dz2*ones(1,Np)));
Bh_sub3 = Rh_ij(idBh,1:Np).*Rh_ji(idBh,1:Np).*exp(-1j*ones(Ns,1)*kp(idBh,1:Np).*(dz3*ones(1,Np)));
Bh_sub4 = Rh_ij(idBh,1:Np).*Rh_ji(idBh,1:Np).*exp(-1j*ones(Ns,1)*kp(idBh,1:Np).*(dz4*ones(1,Np)));


D = sqrt(dz0.^2+p.^2);
Ae_tail0 = dgf_tail_A_sub(kn, dz0, D);
Ae_tail1 = Re_ij(idAe,1:Np).*dgf_tail_A_sub(kn, dz1, D);
Ae_tail2 = Re_ji(idAe,1:Np).*dgf_tail_A_sub(kn, dz2, D);
Ae_tail3 = Re_ij(idAe,1:Np).*Re_ji(idAe,1:Np).*dgf_tail_A_sub(kn, dz3, D);
Ae_tail4 = Re_ij(idAe,1:Np).*Re_ji(idAe,1:Np).*dgf_tail_A_sub(kn, dz4, D);


