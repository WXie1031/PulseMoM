function out = gmd_self_ellipse(a, b, ver)


if nargin < 3
    ver = 0;
end


if ver==1
    %out = ln_gmd;
    out = log((a+b)/2) - 1/4;
else
    %out = exp(ln_gmd);
    out = 0.3894*(a+b);
end


end



