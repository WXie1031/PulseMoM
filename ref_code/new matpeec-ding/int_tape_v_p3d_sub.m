%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% INT_TAPE_V_SEG2D_SUB Core function of Perpendicular Tape-Tape or 
%               Bar-Filament integral. (parallel equal length) Tapes lay on 
%               x-z plane. If tape lay in other planes, transfrom the axis
%               to x-z plane.             
% %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% INPUT %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% q             [N*1] x difference of two tapes
% r             [N*1] y difference of two tapes
% l             [N*1] length of tapes
% %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% OUTPUT %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% int           result of the integral core
% %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% revised by Chen Hongcai 2014.5
function int = int_tape_v_p3d_sub(q, r, l)

err = 1e-12;

[Nrow, Ncol] = size(q);
int = zeros(Nrow,Ncol);

qabs = abs(q);
rabs = abs(r);

ind0 = (qabs<=err & rabs<=err);
ind1 = (qabs<=err & rabs>err );
ind2 = (qabs>err  & rabs<=err);
ind3 = (qabs>err  & rabs>err);


q2 = q.*q;
r2 = r.*r;
l2 = l.*l;

q3 = q2.*q;
r3 = r2.*r;
l3 = l2.*l;

qr = q.*r;
A = sqrt(q2 + r2 + l2);
B = sqrt(q2 + r2);


% % q==0 && r==0 
int(ind0) = zeros(size(find(ind0>0)));

% % q==0 && r~=0 
int(ind1) = (l2(ind1)-1/3*r2(ind1)).*r(ind1).*log(A(ind1)) + 1/3*r3(ind1).*log(B(ind1)) ;
    
% % q~=0 && r==0
int(ind2) = (l2(ind2)-1/3*q2(ind2)).*q(ind2).*log(A(ind2)) + 1/3*q3(ind2).*log(B(ind2)) ;

% % q~=0 && r~= 0
int(ind3) = (l2(ind3)-1/3*r2(ind3)).*r(ind3).*log(q(ind3)+A(ind3)) ...
    + (l2(ind3)-1/3*q2(ind3)).*q(ind3).*log(r(ind3)+A(ind3)) ...
  	+ qr(ind3).*l(ind3).*log((l(ind3)+A(ind3))./(-l(ind3)+A(ind3))) ...
    - 2/3*qr(ind3).*A(ind3) - 1/3*l3(ind3).*atan(qr(ind3)./l(ind3)./A(ind3)) ...
  	- q2(ind3).*l(ind3).*atan(r(ind3).*l(ind3)./q(ind3)./A(ind3)) ...
    - r2(ind3).*l(ind3).*atan(q(ind3).*l(ind3)./r(ind3)./A(ind3)) ...
 	+ 1/3*r3(ind3).*log(q(ind3)+B(ind3)) + 1/3*q3(ind3).*log(r(ind3)+B(ind3)) ...
    + 2/3*qr(ind3).*B(ind3) ;


end



