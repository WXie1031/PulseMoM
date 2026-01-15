function out = gmd_rec_self(w,t, ver)

if nargin < 3
    ver = 0;
end


if ver==1
    %out = ln_gmd;
    out = log(w+t)-3/2;
else
    %out = exp(ln_gmd);
    out = 0.2235*(w+t);
end


end


