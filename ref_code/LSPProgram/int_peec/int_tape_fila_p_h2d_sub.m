%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% int_tape_line_p_3d_sub Core function of Parallel Tape-Filament integral 
%               Tapes lay on x-z plane. If tape lay in other planes, 
%               transfrom the axis to x-z plane.             
% %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% INPUT %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% q             [N*1] x difference of tape and filament
% P             [N*1] vertical height between tape and line(y difference)
% s             [N*1] z difference of tape and filament
% %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% OUTPUT %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% int           result of the integral core
% %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% revised by Chen Hongcai 2014.5
function int = int_tape_fila_p_h2d_sub(q, s)

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
qs = q.*s;

A = sqrt( q2 +s2 );

% % q==0 && P==0 && s==0
int(ind0) = zeros(size(find(ind0>0)));

% % q==0 && P==0 && s~=0
int(ind1) = s2(ind1)/2.*log(A(ind1));
 
% % q~=0 && P==0 && s~=0
int(ind3) = s2(ind3)/2.*log(q(ind3)+A(ind3)) + ...
    qs(ind3).*log(s(ind3)+A(ind3)) - q(ind3)/2.*A(ind3) ;

% % q~=0 && P==0 && s==0
int(ind4) = -q(ind4)/2.*A(ind4) ;
          

end

