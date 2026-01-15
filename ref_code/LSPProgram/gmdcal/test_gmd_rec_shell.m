% from paper with GMD, AMSD, AMD
addpath('..\');
addpath('..\induct');
addpath('..\intpeec');
R = 0.2235*44e-3;
len = 1;
Wo=10e-3;
To=10e-3;
% include self internal inductance
Lcir = 2e-7*( len.*log( sqrt(len.^2+R.^2)+1 ) ...
    - len.*(log(R)-1/4) - sqrt(len.^2+R.^2) + 0.905415*R );

Ls = induct_bar_Grover(Wo, To, len)/len;
Lin0 = (induct_gmd(gmd_rec_self(Wo,To),len))/len;

Lin1 = (induct_gmd(gmd_rec_shell_self(Wo,To),len))/len;
% Lin2 = induct_rec_shell( Wo, To, len )/len;
Lin3 = induct_bar_ext(Wo,To,len)/len;

Lin = [];
Lin = [Lin, Lin0-Lin1];
% Lin = [Lin, Lin0-Lin2];
Lin = [Lin, Lin0-Lin3];
% Lin = [Lin, Lin0-Lin4];

Lin = [Lin, 2.35*induct_bar_in_dc_appr(Wo, To, len)/len]

