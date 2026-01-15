
ver = 1;

W = 40e-3; T = 4e-3;

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

D11 = gmd_rec(x1,y1,W1,T1, x1,y1,W1,T1,ver)
D12 = gmd_rec(x1,y1,W1,T1, x2,y2,W2,T2,ver)
D22 = gmd_rec(x2,y2,W2,T2, x2,y2,W2,T2,ver)
D23 = gmd_rec(x2,y2,W2,T2, x3,y3,W3,T3,ver)

A1 = W1*T1;
A2 = W2*T2;

ln_Rs1 = (A1*(D11)+2*A2*(D12))./(A1+2*A2);

ln_Rs2 = (A2*(D22)+A1*(D12)+A2*(D23))./(A1+2*A2);

D2 = (A1*ln_Rs1+A2*ln_Rs2)./(A1+A2)

len = 1;
Ls = induct_agi_gmd(W, T, len)

Ls = induct_bar_Grover(W, T, len)
 
re = 
Ls = 2e-7*len.*( log(len./re+sqrt((len./re).^2+1)) ...
    - sqrt(1+(re./len).^2) + re./len );


