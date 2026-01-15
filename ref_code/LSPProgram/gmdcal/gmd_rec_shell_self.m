function out = gmd_rec_shell_self(W,T, ver)

if nargin < 3
    ver = 0;
end


W2 = W.*W;
T2 = T.*T;

WT2 = (W+T).^2;
W2_T2 = W2+T2;


ln_gmd = ( (T2.*log(T)+W2.*log(W)) + (W.*T).*log(W2_T2) ...
    + (T2.*atan(W./T)+W2.*atan(T./W)) + pi/2*W.*T )./WT2 ...
    -3/2 ;

% ln_gmd = 0.5*log(W2_T2) + (T2.*atan(W./T)+W2.*atan(T./W))./WT2 - 3/2;


%% output version
if ver==1
    out = ln_gmd;
else
    out = exp(ln_gmd);
end



end


