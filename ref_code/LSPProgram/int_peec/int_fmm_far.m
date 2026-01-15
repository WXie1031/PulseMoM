function int = int_fmm_far(dv1, len1, dv2, len2, d)

% Nf = size(dv2,1);
% dv_sign = ones(Nf,1);
% for ik = 1:Nf
%     if sum( abs(dv2(ik,1:3) + dv1) )<1e-3
%         dv_sign(ik) = -1;
%     end
% end


% ind = mu0/(2*pi).*len1.*(log(2*len1./d)-1) ;

cosb = dot(repmat(dv1,size(dv2,1),1),dv2,2);

int =  cosb .* len1.*len2./d ;
% ind = fre*len1/2

end

