function out = gmd_line_line_a(P1,P2, Q1,Q2, ver)

if nargin < 5
    ver = 0;
end

Nq = size(Q1,1);

P1V = ones(Nq,1)*P1;
P2V = ones(Nq,1)*P2;

R12 = sum((Q2-P2V).^2,2);
R22 = sum((Q1-P2V).^2,2);
R32 = sum((Q1-P1V).^2,2);
R42 = sum((Q2-P1V).^2,2);

R1 = sqrt(R12);
R2 = sqrt(R22);
R3 = sqrt(R32);
R4 = sqrt(R42);

l2 = sum((P2-P1).^2,2);
m2 = sum((Q2-Q1).^2,2);
m = sqrt(m2);
l = sqrt(l2);

d2 = R42-R32+R22-R12;
cosO = d2./(2*l.*m);
sinO2 = abs(1-cosO.^2);
sinO = sqrt(sinO2);


u = (2*m2.*(R22-R32-l2)+d2.*(R42-R32-m2)).*l ./ (4*l2.*m2-d2.*d2);
v = (2*l2.*(R42-R32-m2)+d2.*(R22-R32-l2)).*m ./ (4*l2.*m2-d2.*d2);


angQ1 = acos( (R22+R32-l2)./(2*R2.*R3) );
angQ2 = acos( (R12+R42-l2)./(2*R1.*R4) );
angP1 = acos( (R32+R42-m2)./(2*R3.*R4) );
angP2 = acos( (R12+R22-m2)./(2*R1.*R2) );

% Ns = size(P2,1);
% ln_gmd = zeros(Ns,1);


ln_gmd = ( (...
    log(R3).*(2*u.*v.*sinO2-R32.*cosO) + log(R1).*(2*(u+l).*(m+v).*sinO2-R12.*cosO) ...
    -log(R2).*(2*(u+l).*v.*sinO2-R22.*cosO) - log(R4).*(2*u.*(m+v).*sinO2-R42.*cosO) ...
    -sinO.*( v.^2.*angQ1 - (v+m).^2.*angQ2 + u.^2.*angP1 -(u+l).^2.*angP2 ) ...
    )./(l.*m)-3 ) /2;


% 3. output version

if ver==1
    out = ln_gmd;
else
    out = exp(ln_gmd);
    
    id0 = abs(ln_gmd)<1e-12;
    out(id0) = 1e10;
end



end



