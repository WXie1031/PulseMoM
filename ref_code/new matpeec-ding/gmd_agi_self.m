function out = gmd_agi_self(W,T, ver)

if nargin<3
    ver = 0;
end


rec_ver = 1;


x1 = 0;
y1 = 0;
W1 = T;
T1 = T;

x2 = W/2;
y2 = 0;
W2 = W-T;
T2 = T;

x3 = 0;
y3 = W/2;
W3 = T;
T3 = W-T;

ln_D11 = gmd_self_rec(W1,T1, rec_ver);
ln_D12 = gmd_rec_rec(x1,y1,W1,T1, x2,y2,W2,T2,rec_ver);
ln_D22 = gmd_self_rec(W2,T2, rec_ver);
ln_D23 = gmd_rec_rec(x2,y2,W2,T2, x3,y3,W3,T3,rec_ver);

A1 = W1*T1;
A2 = W2*T2;

ln_Rs1 = (A1*(ln_D11)+2*A2*(ln_D12))./(A1+2*A2);
ln_Rs2 = (A2*(ln_D22)+A1*(ln_D12)+A2*(ln_D23))./(A1+2*A2);

ln_Rs = (A1*ln_Rs1+2*A2*ln_Rs2)./(A1+2*A2);

if ver==1
    out = ln_Rs;
else
    out = exp(ln_Rs);
end



end



