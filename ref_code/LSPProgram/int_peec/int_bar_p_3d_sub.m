%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% INT_BAR_2D_SUB Core function of Perpendicular Bar-Bar integral 
%               Bars lay on x-z plane. If tape lay in other planes, 
%               transfrom the axis to x-z plane.             
% %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% INPUT %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% q             [N*1] x difference of two tapes
% r             [N*1] y difference of two tapes
% s             [N*1] length of tapes
% %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% OUTPUT %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% int           result of the integral core
% %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% revised by Chen Hongcai 2014.5
function int = int_box_p_3d_sub(q, r, s)

err = 1e-12;

[Nrow, Ncol] = size(q);
int = zeros(Nrow,Ncol);

qabs = abs(q);
rabs = abs(r);
sabs = abs(s);

ind0 = (qabs<=err & rabs<=err & sabs<=err);
ind1 = (qabs<=err & rabs<=err & sabs>err);
ind2 = (qabs<=err & rabs>err  & sabs<=err);
ind3 = (qabs<=err & rabs>err  & sabs>err);
ind4 = (qabs>err  & rabs<=err & sabs<=err);
ind5 = (qabs>err  & rabs<=err & sabs>err);
ind6 = (qabs>err  & rabs>err  & sabs<=err);
ind7 = (qabs>err  & rabs>err  & sabs>err);


q2 = q.*q;
r2 = r.*r;
s2 = s.*s;
q4 = q2.*q2;
r4 = r2.*r2;
s4 = s2.*s2;

r2s2 = r2.*s2;
q2s2 = q2.*s2;
q2r2 = q2.*r2;

qrs = q.*r.*s;

A = sqrt(q2 + r2 + s2);
B1 = sqrt(q2+r2);
B2 = sqrt(r2+s2);
B3 = sqrt(q2+s2);

% % q==0 && r==0 && s==0
int(ind0) = zeros(size(find(ind0>0)));

% % q==0 && r==0 && s~=0
int(ind1) = 1/60*s4(ind1).*A(ind1);

% % q==0 && r~=0 && s==0
int(ind2) = 1/60*r4(ind2).*A(ind2);

% % q==0 && r~=0 && s~=0
int(ind3) = -s4(ind3)/24.*r(ind3).*log((r(ind3)+A(ind3))./B3(ind3)) ...
    - r4(ind3)/24.*s(ind3).*log((s(ind3)+A(ind3))./B1(ind3)) ...
  	+ 1/60*(r4(ind3)+s4(ind3)-3*r2s2(ind3)).*A(ind3);
  	
% % q~=0 && r==0 && s==0
int(ind4) = 1/60*q4(ind4).*A(ind4);

% % q~=0 && r==0 && s~=0
int(ind5) = -s4(ind5)/24.*q(ind5).*log((q(ind5)+A(ind5))./B2(ind5)) ...
    - q4(ind5)/24.*s(ind5).*log((s(ind5)+A(ind5))./B1(ind5)) ...
  	+ 1/60*(q4(ind5)+s4(ind5)-3*q2s2(ind5)).*A(ind5) ;
 
% % q~=0 && r~=0 && s==0
int(ind6) = (-r4(ind6)/24).*q(ind6).*log((q(ind6)+A(ind6))./B2(ind6)) ...
    + (-q4(ind6)/24).*r(ind6).*log((r(ind6)+A(ind6))./B3(ind6)) ...
  	+ 1/60*(q4(ind6)+r4(ind6)-3*q2r2(ind6)).*A(ind6) ;
  
% % q~=0 && r~=0 && s~=0
int(ind7) = (r2s2(ind7)/4-r4(ind7)/24-s4(ind7)/24).*q(ind7).*log((q(ind7)+A(ind7))./B2(ind7)) ...
	+ (q2s2(ind7)/4-q4(ind7)/24-s4(ind7)/24).*r(ind7).*log((r(ind7)+A(ind7))./B3(ind7)) ...
	+ (q2r2(ind7)/4-q4(ind7)/24-r4(ind7)/24).*s(ind7).*log((s(ind7)+A(ind7))./B1(ind7)) ...
  	+ 1/60*(q4(ind7)+r4(ind7)+s4(ind7)-3*q2r2(ind7)-3*r2s2(ind7)-3*q2s2(ind7)).*A(ind7) ...
  	- qrs(ind7).*s2(ind7)/6.*atan(q(ind7).*r(ind7)./s(ind7)./A(ind7)) ...
  	- qrs(ind7).*r2(ind7)/6.*atan(q(ind7).*s(ind7)./r(ind7)./A(ind7)) ...
  	- qrs(ind7).*q2(ind7)/6.*atan(r(ind7).*s(ind7)./q(ind7)./A(ind7));


end


