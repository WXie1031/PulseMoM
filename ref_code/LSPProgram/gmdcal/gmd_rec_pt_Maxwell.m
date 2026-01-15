function out = gmd_rec_pt_Maxwell(P1,P2,P3,P4, Q, ver)


if nargin < 6
    ver = 0;
end

Nq = size(Q,1);

R1 = distance_pt2line2d(P1, P2, Q, 1); % the order should not be changed !
R2 = distance_pt2line2d(P2, P3, Q, 1); % the order should not be changed !
R3 = -distance_pt2line2d(P3, P4, Q, 1); % the order should not be changed !
R4 = -distance_pt2line2d(P4, P1, Q, 1); % the order should not be changed !

P1V = ones(Nq,1)*P1;
P2V = ones(Nq,1)*P2;
P3V = ones(Nq,1)*P3;
P4V = ones(Nq,1)*P4;

RP12 = sum((Q-P1V).^2,2);
RP22 = sum((Q-P2V).^2,2);
RP32 = sum((Q-P3V).^2,2);
RP42 = sum((Q-P4V).^2,2);

RP1 = sqrt(RP12);
RP2 = sqrt(RP22);
RP3 = sqrt(RP32);
RP4 = sqrt(RP42);

W2 = sum((P1-P2).^2,2);
T2 = sum((P1-P4).^2,2);
W = sqrt(W2);
T = sqrt(T2);

angP12 = acos( (RP12+RP22-W2)./(2*RP1.*RP2) );
angP14 = acos( (RP12+RP42-T2)./(2*RP1.*RP4) );
angP23 = acos( (RP22+RP32-T2)./(2*RP2.*RP3) );
angP34 = acos( (RP32+RP42-W2)./(2*RP3.*RP4) );

R4xR1 = R4.*R1;
R1xR2 = R1.*R2;
R2xR3 = R2.*R3;
R3xR4 = R3.*R4;

ln_gmd = ( (2*R4xR1.*log(RP1) + sign(R4xR1).*R4.^2.*angP14 ...
    + 2*R1xR2.*log(RP2) + sign(R1xR2).*R1.^2.*angP12 ...
    + 2*R2xR3.*log(RP3) + sign(R2xR3).*R2.^2.*angP23 ...
    + 2*R3xR4.*log(RP4) + sign(R3xR4).*R3.^2.*angP34)./(W.*T)-3 )/2;


% 3. output version
if ver==1
    out = ln_gmd;
else
    out = exp(ln_gmd);
end



end



