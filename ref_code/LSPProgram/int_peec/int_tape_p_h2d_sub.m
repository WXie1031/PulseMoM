%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% INT_TAPE_P_2D_SUB Core function of Parallel Tape-Tape integral 
%               Tapes lay on x-z plane. If tape lay in other planes,
%               transfrom the axis to x-z plane.             
% %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% INPUT %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% q             [N*1] x difference of two tapes
% s             [N*1] z difference of two tapes
% %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% OUTPUT %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% int           result of the integral core
% %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% revised by Chen Hongcai 2014.5
function int = int_tape_p_h2d_sub(q, s)

err = 1e-12;

[Nrow, Ncol] = size(q);
int = zeros(Nrow,Ncol);

qabs = abs(q);
sabs = abs(s);

ind0 = (qabs<=err & sabs<=err);
ind1 = (qabs<=err & sabs>err);
ind3 = (qabs>err  & sabs>err);
ind4 = (qabs>err  & sabs<=err);

q2 = q.*q;
s2 = s.*s;

A = sqrt( q2 + s2 );


% % q==0 && P==0 && s==0
int(ind0) = zeros(size(find(ind0>0)));

% % q==0 && P==0 && s~=0
int(ind1) =  - 1/6*s2(ind1).*A(ind1) ;
    
% % q~=0 && P==0 && s~=0
int(ind3) = q2(ind3)/2.*s(ind3).*log(s(ind3)+A(ind3)) ...
    + s2(ind3)/2.*q(ind3).*log(q(ind3)+A(ind3)) ...
    - 1/6*(q2(ind3)+s2(ind3)).*A(ind3) ;
           
% % q~=0 && P==0 && s==0
int(ind4) =  - 1/6*q2(ind4).*A(ind4) ;



end




