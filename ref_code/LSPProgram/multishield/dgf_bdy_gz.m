function gZij_new = dgf_bdy_gz(Zij, Zij1, gZij, kj, dj)


gZij_new = Zij.*( gZij.*cos(kj.*dj) + 1j*Zij.*sin(kj.*dj) )./ ...
    ( Zij1.*cos(kj.*dj) + 1j*gZij.*sin(kj.*dj) );


end

