function [ Mtx, exitflag ] = update_mtx_data( Mtx, ik, ig, data )

[Nrow, Ncol] = size(Mtx);

if (ik > Nrow) ||(ig > Ncol)
    exitflag = 0;
else
    Mtx(ik,ig) = data;
    exitflag = 1;
end

end

