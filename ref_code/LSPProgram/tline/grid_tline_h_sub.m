function err = grid_tline_h_sub(Zin, D, kg, rg, wm, K)

    mu0 = 4*pi*1e-7;

    Ytmp = pi*kg ./ log(1.12./(K*D));
    
    Ztmp = Zin + 1j*wm*mu0/(2*pi) .* log( 1.85./(sqrt(K.^2+rg.^2).*D) );
        
    err = abs(K - sqrt(Ztmp.*Ytmp));
    

end