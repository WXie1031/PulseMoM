function err = grid_tline_v_sub(Zin, re, kg, rg, wm, K)

    mu0 = 4*pi*1e-7;

    Ytmp = 2*pi*kg ./ log((sqrt(K.^2+rg.^2).*re)/3.56);
    
    Ztmp = Zin + 1j*wm*mu0/(2*pi) .* log( 1.12./(sqrt(K.^2+rg.^2).*re) );
        
    err = abs(K - sqrt(Ztmp.*Ytmp));
    

end