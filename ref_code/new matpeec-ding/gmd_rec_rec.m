function out = gmd_rec_rec(x1,y1,W1,T1, x2,y2,W2,T2, ver)

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
% Wtmp = max(W1,W2(id0));
% Ttmp = max(T1,T2(id0));
% ln_gmd(id0) = log(Wtmp+Ttmp)-1.5;
% gmd = 0.2235*(W1+T1);



%% 2. mutual gmd
% ln_gmd(id1) = log(d0(id1));

if sum(id2)>0
    q1 = s(id2)-W1/2-W2(id2)/2;
    q2 = s(id2)+W1/2-W2(id2)/2;
    q3 = s(id2)+W1/2+W2(id2)/2;
    q4 = s(id2)-W1/2+W2(id2)/2;
    
    r1 = h(id2)-T1/2-T2(id2)/2;
    r2 = h(id2)+T1/2-T2(id2)/2;
    r3 = h(id2)+T1/2+T2(id2)/2;
    r4 = h(id2)-T1/2+T2(id2)/2;
    
%     q1 = s-W1/2-W2/2;
%     q2 = s+W1/2-W2/2;
%     q3 = s+W1/2+W2/2;
%     q4 = s-W1/2+W2/2;
%     
%     r1 = h-T1/2-T2/2;
%     r2 = h+T1/2-T2/2;
%     r3 = h+T1/2+T2/2;
%     r4 = h-T1/2+T2/2;
    
    f11 = gmd_rec_sub(q1, r1);
    f12 = gmd_rec_sub(q1, r2);
    f13 = gmd_rec_sub(q1, r3);
    f14 = gmd_rec_sub(q1, r4);
    
    f21 = gmd_rec_sub(q2, r1);
    f22 = gmd_rec_sub(q2, r2);
    f23 = gmd_rec_sub(q2, r3);
    f24 = gmd_rec_sub(q2, r4);
    
    f31 = gmd_rec_sub(q3, r1);
    f32 = gmd_rec_sub(q3, r2);
    f33 = gmd_rec_sub(q3, r3);
    f34 = gmd_rec_sub(q3, r4);
    
    f41 = gmd_rec_sub(q4, r1);
    f42 = gmd_rec_sub(q4, r2);
    f43 = gmd_rec_sub(q4, r3);
    f44 = gmd_rec_sub(q4, r4);
    
    ln_gmd(id2) = -25/12+1./(2*W1.*T1.*W2(id2).*T2(id2)).*( f11-f12+f13-f14 -f21+f22-f23+f24 ...
        +f31-f32+f33-f34 -f41+f42-f43+f44 );
%      ln_gmd = -25/12+1./(2*W1.*T1.*W2.*T2).*( f11-f12+f13-f14 -f21+f22-f23+f24 ...
%         +f31-f32+f33-f34 -f41+f42-f43+f44 );
end

%% 3. output version

if ver==1
    out = ln_gmd;
else
    out = exp(ln_gmd);
end



end



