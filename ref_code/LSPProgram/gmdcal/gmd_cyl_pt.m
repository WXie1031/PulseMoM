function out = gmd_cyl_pt(x0,y0,rout, xpt,ypt, ver)


if nargin < 6
    ver = 0;
end

R = sqrt( (x0-xpt).^2+(y0-ypt).^2 );


if ver==1
    %out = ln_gmd;
    out = log(max(R,rout));
else
    %out = exp(ln_gmd);
    out = max(R,rout);
end


end



