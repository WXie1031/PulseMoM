function out = gmd_cir_self(rout, ver)


if nargin < 2
    ver = 0;
end




if ver==1
    %out = ln_gmd;
    out = log(rout)-1/4;
else
    %out = exp(ln_gmd);
    out = 0.7788*rout;
end


end



