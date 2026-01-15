%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% int_tape_v_2d_sub Core function of Perpendicular Tape-Tape/Bar-Filament 
%               integral. Tapes lay on x-z plane. If tape lay in other planes, 
%               transfrom the axis to x-z plane.             
% %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% INPUT %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% q             [N*1] x difference of two tapes
% r             [N*1] y difference of two tapes
% s             [N*1] z difference of two tapes
% %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% OUTPUT %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% int           result of the integral core
% %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% revised by Chen Hongcai 2014.5
function int = int_tape_v_3d_sub(q, r, s)

err = 1e-12;

[Nrow, Ncol] = size(q);
int = zeros(Nrow,Ncol);

qabs = abs(q);
rabs = abs(r);
sabs = abs(s);

ind0 = (qabs<=err & rabs<=err);
ind2 = (qabs<=err & rabs>err  & sabs<=err);
ind3 = (qabs<=err & rabs>err  & sabs>err);
ind5 = (qabs>err  & rabs<=err & sabs>err);
ind4 = (qabs>err  & rabs<=err & sabs<=err);
ind6 = ~(ind0+ind3+ind5+ind4+ind2);


q2 = q.*q;
r2 = r.*r;
s2 = s.*s;
s3 = s2.*s;
qr = q.*r;
    
A = sqrt(q2 + r2 + s2);


% % q==0 && r==0 
int(ind0) = zeros(size(find(ind0>0)));

% % q==0 && r~=0 && s==0
int(ind2) = -r2(ind2)/6.*r(ind2).*log(A(ind2)) ;

% % q==0 && r~=0 && s~=0
int(ind3) = (s2(ind3)/2-r2(ind3)/6).*r(ind3).*log(A(ind3)) ;

% % q~=0 && r==0 && s==0
int(ind4) = -q2(ind4)/6.*q(ind4).*log(A(ind4)) ;

% % q~=0 && r==0 && s~=0
int(ind5) = (s2(ind5)/2-q2(ind5)/6).*q(ind5).*log(A(ind5)) ;
           
% % Others
int(ind6) = (s2(ind6)/2-r2(ind6)/6).*r(ind6).*log(q(ind6)+A(ind6)) ...
    + (s2(ind6)/2-q2(ind6)/6).*q(ind6).*log(r(ind6)+A(ind6)) ...
  	+ qr(ind6).*s(ind6).*log(s(ind6)+A(ind6)) - 1/3*qr(ind6).*A(ind6)...
    - 1/6*s3(ind6).*atan(qr(ind6)./s(ind6)./A(ind6)) ...
 	- 1/2*q2(ind6).*s(ind6).*atan(r(ind6).*s(ind6)./q(ind6)./A(ind6)) ...
    - 1/2*r2(ind6).*s(ind6).*atan(q(ind6).*s(ind6)./r(ind6)./A(ind6));


end


