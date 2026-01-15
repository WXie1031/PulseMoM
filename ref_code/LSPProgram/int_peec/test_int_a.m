
ps1 = [0 0 0;];
ps2 = [1 0 0;];
[dv1, l1] = line_dv(ps1,ps2);
r1 = 5e-3;

Nc = 10;
theta = 2*pi/Nc*(0:Nc)';
cos(theta)
sin(theta)
pf1 = zeros(Nc+1,3);
pf2 = [cos(theta)  sin(theta)  zeros(Nc+1,1);];
[dv2, l2] = line_dv(pf1,pf2);

r2 = 5e-3*ones(Nc+1,1);

int1 = int_fila_a_anal(ps1, ps2, l1, pf1, pf2, l2)
int2 = int_fila_a_num(ps1,ps2,dv1,l1,r1, pf1,pf2,dv2,l2,r2)
int3 = int_fila_v_num(ps1,ps2,dv1,l1,r1, pf1,pf2,dv2,l2,r2)

%out=INT_LINE_D2T(U1a,U1b,V1,W1,U2,V2a,V2b,W2)
    
