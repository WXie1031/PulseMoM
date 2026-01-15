% from paper with GMD, AMSD, AMD
R = 0.2235*44e-3/0.7788;
len = 6.6;

% include self internal inductance
Lcir = 2e-7*( len.*log( sqrt(len.^2+R.^2)+1 ) ...
    - len.*(log(R)-1/4) - sqrt(len.^2+R.^2) + 0.905415*R );

Lcyl = 2e-7*( len.*log( sqrt(len.^2+2*R.^2)+1 ) ...
    - len.*(log(R)) - sqrt(len.^2+2*R.^2) + pi/4*R );

(Lcir-Lcyl)/len


Lin1 = (induct_gmd(gmd_cir_self(R),len)-induct_gmd(gmd_cyl_self(R),len))/len
Lin2 = (induct_gmd(gmd_cir_self(R),len)-induct_cyl_num(R, len))/len
Lin3 = (induct_gmd(gmd_cir_self(R),len)-induct_cyl_Aebischer(R, len))/len






