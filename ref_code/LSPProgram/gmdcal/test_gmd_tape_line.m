% from paper with GMD, AMSD, AMD
W = 4e-3;
len = 100;

P1=[0 0];
P2=[W 0e-3];

Qpt=[-10e-3,-0e-3];

gmdtmp = gmd_line_pt(P1,P2, Qpt);


L1 = (induct_gmd(gmdtmp,len))/len
L2 = 1e-7*int_tape_line_2d(P1,P2, Qpt, len)/len
%Ls = 1e-7*int_line_p2d(P2(:,1)/2, P2(:,2)/2, Qpt(:,1), Qpt(:,2), len, 2e-3)/len


