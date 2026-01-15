%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% INT_TAPE_P_2D_SUB Core function of Parallel Tape-Tape integral 
%               Tapes lay on x-z plane. If tape lay in other planes,
%               transfrom the axis to x-z plane.             
% %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% INPUT %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% q             [N*1] x difference of two tapes
% P             [N*1] vertical height between two tapes(y difference)
% s             [N*1] z difference of two tapes
% %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% OUTPUT %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% int           result of the integral core
% %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% revised by Chen Hongcai 2014.5
function int = int_tape_p_3d_sub(q, P, s)

err = 1e-12;

[Nrow, Ncol] = size(q);
int = zeros(Nrow,Ncol);

qabs = abs(q);
Pabs = abs(P);
sabs = abs(s);

ind0 = (qabs<=err & Pabs<=err & sabs<=err);
ind1 = (qabs<=err & Pabs<=err & sabs>err);
ind2 = (qabs<=err & Pabs>err  & sabs>err);
ind3 = (qabs>err  & Pabs<=err & sabs>err);
ind4 = (qabs>err  & Pabs<=err & sabs<=err);
ind5 = ~(ind0+ind1+ind2+ind3+ind4);

q2 = q.*q;
P2 = P.*P;
s2 = s.*s;

A = sqrt( q2 + P2 +s2 );


% % q==0 && P==0 && s==0
int(ind0) = zeros(size(find(ind0>0)));

% % q==0 && P==0 && s~=0
int(ind1) =  - 1/6*s2(ind1).*A(ind1) ;

% % q==0 && P~=0 && s~=0
int(ind2) = -P2(ind2)/2.*s(ind2).*log(s(ind2)+A(ind2)) ...
    - 1/6*(-2*P2(ind2)+s2(ind2)).*A(ind2) ;
    
% % q~=0 && P==0 && s~=0
int(ind3) = q2(ind3)/2.*s(ind3).*log(s(ind3)+A(ind3)) ...
    + s2(ind3)/2.*q(ind3).*log(q(ind3)+A(ind3)) ...
    - 1/6*(q2(ind3)+s2(ind3)).*A(ind3) ;
           
% % q~=0 && P==0 && s==0
int(ind4) =  - 1/6*q2(ind4).*A(ind4) ;

% % Others
int(ind5) = (q2(ind5)-P2(ind5))/2.*s(ind5).*log(s(ind5)+A(ind5)) ...
    + (s2(ind5)-P2(ind5))/2.*q(ind5).*log(q(ind5)+A(ind5)) ...
	- 1/6*(q2(ind5)-2*P2(ind5)+s2(ind5)).*A(ind5)  ...
    - q(ind5).*P(ind5).*s(ind5).*atan(q(ind5).*s(ind5)./P(ind5)./A(ind5)) ;



end




