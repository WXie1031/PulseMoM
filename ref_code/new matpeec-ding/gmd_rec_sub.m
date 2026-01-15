function f = gmd_rec_sub(q, r)

id0 = (q==0 & r==0);
id1 = ~id0;

Nc = size(q,1);
f = zeros(Nc,1);


f(id0) = 0;

f(id1) = (q(id1).^2.*r(id1).^2/4 - q(id1).^4/24 -r(id1).^4/24).*log(q(id1).^2+r(id1).^2) ...
    + q(id1).^3.*r(id1)/3.*atan(r(id1)./q(id1)) + q(id1).*r(id1).^3/3.*atan(q(id1)./r(id1));

% f = (q.^2.*r.^2/4 - q.^4/24 -r.^4/24).*log(q.^2+r.^2) ...
%     + q.^3.*r/3.*atan(r./q) + q.*r.^3/3.*atan(q./r);


end


