
sig = 1e7;
mur = 40;
r_out = 5e-3;
r_in = 0;
len = 1;
fmur = 20e3;
S = pi*(r_out.^2-r_in.^2);
R_pul = 1/(S*sig);
Rsurge = zin_cir_ac(r_out, r_in, sig, mur, len, fmur);

mur_fit = mur_fit_cir(r_out, r_in, R_pul, sig, Rsurge, fmur)


sig = 1e7;
mur = 40;
r_out = 5e-3;
r_in = 4e-3;
len = 1;
fmur = 20e3;
S = pi*(r_out.^2-r_in.^2);
R_pul = 1/(S*sig);

mur_fit = mur_fit_cir(r_out, r_in, R_pul, sig, Rsurge, fmur)
