function out = gmd_line_line_p(P1,P2, Q1,Q2, ver)

if nargin < 5
    ver = 0;
end

Nq = size(Q1,1);

P1V = ones(Nq,1)*P1;
P2V = ones(Nq,1)*P2;

dv_sign = sign(sum((P1V-P2V).*(Q1-Q2)));

R12 = sum((Q2-P2V).^2,2);
R22 = sum((Q1-P2V).^2,2);
R32 = sum((Q1-P1V).^2,2);
R42 = sum((Q2-P1V).^2,2);

R1 = sqrt(R12);
R2 = sqrt(R22);
R3 = sqrt(R32);
R4 = sqrt(R42);

l = sqrt(sum((P2-P1).^2,2));
m = sqrt(sum((Q2-Q1).^2,2));


ln_gmd = dv_sign.*( ( -log(R3).*R32-log(R1).*R12+log(R2).*R22+log(R4).*R42) ...
    ./(l.*m)-3 ) /2;


% 3. output version
if ver==1
    out = ln_gmd;
else
    out = exp(ln_gmd);
    
    id0 = abs(ln_gmd)<1e-12;
    out(id0) = 1e10;
end



end



