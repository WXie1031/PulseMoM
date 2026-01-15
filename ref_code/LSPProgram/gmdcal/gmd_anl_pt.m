function out = gmd_anl_pt(x0,y0,rout,rin, xpt,ypt, ver)


R = sqrt((xpt-x0).^2+(ypt-y0).^2);

if nargin < 7
    ver = 0;
end



if R<rout
    ln_gmd = (rout.^2.*log(rout)-rin.^2.*log(rin))./(rout.^2-rin.^2)-0.5;
else
    ln_gmd = log(R);
end


if ver==1
    out = ln_gmd;
else
    if R<rout
        out = exp(ln_gmd);
    else
        out = R;
    end
end


end



