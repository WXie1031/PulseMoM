function out = gmd_self_ellip_shell(a, b, ver)


if nargin < 3
    ver = 0;
end


if ver==1
    %out = ln_gmd;
    out = log((a+b)/4);
else
    %out = exp(ln_gmd);
    out = (a+b)/4;
end


end



