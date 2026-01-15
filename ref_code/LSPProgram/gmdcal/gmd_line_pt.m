function out = gmd_line_pt(P1,P2, Qpt, ver)


if nargin < 4
    ver = 0;
end

[P1new,P2new, Qptnew] = axis_trans2line(P1,P2, Qpt);

W = abs(P2new(:,1)-P1new(:,2));

R12 = (Qptnew(:,1)+W/2).^2+Qptnew(:,2).^2;
R22 = (Qptnew(:,1)-W/2).^2+Qptnew(:,2).^2;
R1 = sqrt(R12);
R2 = sqrt(R22);

% ang = acos( dot([(W/2+Qptnew(:,1)), Qptnew(:,2)], [(Qptnew(:,1))-W/2, Qptnew(:,2)])./(R1.*R2) );

ang = acos((R12+R22-W.^2)./(2*R1.*R2));


Ns = size(Qpt,1);
ln_gmd = zeros(Ns,1);


id0 = (abs(Qptnew(:,2))<1e-6);
id1 =~id0;


% 1. self gmd
ln_gmd(id0) = ((Qptnew(:,1)+W/2).*log(R1)-(Qptnew(:,1)-W/2).*log(R2))./W - 1;

ln_gmd(id1) = ((Qptnew(:,1)+W/2).*log(R1)-(Qptnew(:,1)-W/2).*log(R2)+abs(Qptnew(:,2)).*ang)./W - 1;


% 3. output version

if ver==1
    out = ln_gmd;
else
    out = exp(ln_gmd);
end



end



