
ver = 0;

W = 50e-3; T = 5e-3;

x1 = 0;
y1 = 0;
W1 = 40e-3;
T1 = 5e-3;

x2 = 25e-3;
y2 = 0;
W2 = 10e-3;
T2 = 5e-3;

len = 500;

D0 = gmd_rec_rec(0,0,W,T, 0,0,W,T,ver)

D11 = gmd_rec_rec(x1,y1,W1,T1, x1,y1,W1,T1,ver);
D12 = gmd_rec_rec(x1,y1,W1,T1, x2,y2,W2,T2,ver);
D22 = gmd_rec_rec(x2,y2,W2,T2, x2,y2,W2,T2,ver);


A1 = W1*T1;
A2 = W2*T2;

ln_Rs1 = (A1*(D11)+A2*(D12))./(A1+A2);

ln_Rs2 = (A2*(D22)+A1*(D12))./(A1+A2);

D2 = (A1*ln_Rs1+A2*ln_Rs2)./(A1+A2)


Ls0 = induct_bar_Grover(W, T, len)
 
re = D0;
Ls1 = induct_gmd(re, len)
Ls2 = induct_gmd_long(re, len)


