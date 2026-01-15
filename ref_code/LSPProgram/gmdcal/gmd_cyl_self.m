function out = gmd_cyl_self(rout, ver)


if nargin < 2
    ver = 0;
end


if ver==1
    %out = ln_gmd;
    out = log(rout);
else
    out = rout;
end


end



