function out = gmd_rec_rec_adj(W1,T1, W2,T2, ver)

% GMD for two adjacent symmetrically placed rectangles

s = abs(x2-x1);
h = abs(y2-y1);

d0 = sqrt(s.^2+h.^2);

if nargin < 9
    ver = 0;
end


Ns = size(W2,1);
if Ns==0
    out=zeros(0,1);
    return;
end

ln_gmd = zeros(Ns,1);


id0 = (s==0 & h==0 & W1==W2 & T1==T2);
id1 = (d0>(max(W1,T1)+max(W2,T2)));
id2 =~(id0+id1);


%% 1. self gmd
Wtmp = max(W1,W2(id0));
Ttmp = max(T1,T2(id0));
ln_gmd(id0) = log(Wtmp+Ttmp)-1.5;
% gmd = 0.2235*(W1+T1);



%% 2. mutual gmd
ln_gmd(id1) = log(d0(id1));

if sum(id2)>0
    ln_gmd = 1/4.*( (b+c).^2.*() )
    
end

%% 3. output version

if ver==1
    out = ln_gmd;
else
    out = exp(ln_gmd);
end



end



