function out = gmd_line_line_v(P1,P2, Q1,Q2, ver)

if nargin < 5
    ver = 0;
end

Nq = size(Q1,1);
ln_gmd = zeros(Nq,1);


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

u = (2*m2.*(R22-R32-l2)+d2.*(R42-R32-m2)).*l ./ (4*l2.*m2-d2.*d2);
v = (2*l2.*(R42-R32-m2)+d2.*(R22-R32-l2)).*m ./ (4*l2.*m2-d2.*d2);


angQ1 = acos( (R22+R32-l2)./(2*R2.*R3) );
angQ2 = acos( (R12+R42-l2)./(2*R1.*R4) );
angP1 = acos( (R32+R42-m2)./(2*R3.*R4) );
angP2 = acos( (R12+R22-m2)./(2*R1.*R2) );

ind0 = abs(sinO2)>1e-3;

ln_gmd(ind0) = sinO2(ind0).*( (...
    log(R3(ind0)).*(2*u(ind0).*v(ind0)) ...
    + log(R1(ind0)).*(2*(u(ind0)+l(ind0)).*(m(ind0)+v(ind0))) ...
    -log(R2(ind0)).*(2*(u(ind0)+l(ind0)).*v(ind0)) ...
    - log(R4(ind0)).*(2*u(ind0).*(m(ind0)+v(ind0))) ...
    -( v(ind0).^2.*angQ1(ind0) - (v(ind0)+m(ind0)).^2.*angQ2(ind0) ...
    + u(ind0).^2.*angP1(ind0) -(u(ind0)+l(ind0)).^2.*angP2(ind0) ) ...
    )./(l(ind0).*m(ind0))-3 ) /2;


% 3. output version
if ver==1
    out = ln_gmd;
else
    out = exp(ln_gmd);
    
    id0 = abs(ln_gmd)<1e-12;
    out(id0) = 1e10;
end



end



