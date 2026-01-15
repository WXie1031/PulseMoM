%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% int_tape_p2d_sub Core function of Parallel Tape-Tape integral 
%               (parallel equal length) Tapes lay on x-z plane. 
%               If tape lay in other planes, transfrom the axis to x-z plane.             
% %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% INPUT %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% q             [N*1] x difference of two tapes
% P             [N*1] vertical height between two tapes
% l             [N*1] length of tapes
% %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% OUTPUT %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% int           result of the integral core
% %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% revised by Chen Hongcai 2014.5
function int = int_tape_p_p3d_sub(q, P, l)

err = 1e-12;

[Nrow, Ncol] = size(q);
int = zeros(Nrow,Ncol);

qabs = abs(q);
Pabs = abs(P);
ind0 = (qabs<=err & Pabs<=err);
ind1 = (qabs<=err & Pabs>err);
ind2 = (qabs>err  & Pabs<=err);
ind3 = (qabs>err  & Pabs>err);


q2 = q.*q;
P2 = P.*P;
l2 = l.*l;

A = sqrt( q2 + P2 +l2 );
B = sqrt( q2 + P2 ); 


% % q==0 && P==0
int(ind0) = - 1/3*l2(ind0).*A(ind0);

% % q==0 && P~=0
int(ind1) = -1/2*P2(ind1).*l(ind1).*log((l(ind1)+A(ind1))./(-l(ind1)+A(ind1))) ...
    - 1/3*(-2*P2(ind1)+l2(ind1)).*A(ind1) - 2/3*P2(ind1).*B(ind1) ;
  	
% % q~=0 && P==0
int(ind2) = 1/2*q2(ind2).*l(ind2).*log((l(ind2)+A(ind2))./(-l(ind2)+A(ind2))) ...
    + l2(ind2).*q(ind2).*log(q(ind2)+A(ind2)) ...
    - 1/3*(q2(ind2)+l2(ind2)).*A(ind2) + 1/3*q2(ind2).*B(ind2) ;
 	
% % Others
int(ind3) = 1/2*(q2(ind3)-P2(ind3)).*l(ind3).*log((l(ind3)+A(ind3))./(-l(ind3)+A(ind3))) ...
    + (l2(ind3)-P2(ind3)).*q(ind3).*log(q(ind3)+A(ind3)) ...
 	- 1/3*(q2(ind3)-2*P2(ind3)+l2(ind3)).*A(ind3) ...
    - 2*q(ind3).*P(ind3).*l(ind3).*atan(q(ind3).*l(ind3)./P(ind3)./A(ind3)) ...
  	+ P2(ind3).*q(ind3).*log(q(ind3)+B(ind3)) + 1/3*(q2(ind3)-2*P2(ind3)).*B(ind3) ;


end



