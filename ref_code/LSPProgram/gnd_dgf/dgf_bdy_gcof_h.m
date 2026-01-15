function gRij_new = dgf_bdy_gcof_h(Rij,gRij, kzj, dj)


gRij_new = ( -Rij + gRij.*exp(-2*1j.*kzj.*dj) )./( 1 - Rij.*gRij.*exp(-2*1j.*kzj.*dj) );

id_inf = (gRij_new==Inf) | (gRij_new==-Inf) | (isnan(gRij_new));

gRij_new(id_inf) = 1./Rij(id_inf);

end

