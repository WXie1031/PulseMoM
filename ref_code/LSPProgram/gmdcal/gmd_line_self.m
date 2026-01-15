function out = gmd_line_self(w, ver)


if nargin < 3
    ver = 0;
end


if ver==1
    %out = ln_gmd;
    out = log(w)-3/2;
else
    %out = exp(ln_gmd);
    out = 0.22313*w;
end


end



