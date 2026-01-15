% from paper with GMD, AMSD, AMD
W = 4e-3;
len = 100;

P1=[0 0];
P2=[W 3 ];

Q1=[0e-3,20e-3];
Q2=[50e-3,40e-3];

gmd1 = gmd_line_line_a(P1,P2, Q1,Q2);
gmd2 = gmd_line_line_p(P1,P2, Q1,Q2);
gmd3 = gmd_line_line_v(P1,P2, Q1,Q2);

L0 = (induct_gmd(gmd1,len))/len
L1 = (induct_gmd(gmd2,len))/len
L2 = (induct_gmd(gmd3,len))/len
% L3 = 1e-7*int_tape_p2d(0, W, 0, Q1(:,2), Q2(:,2), -10e-3, len)/len
% L4 = 1e-7*int_line_p2d(P2(:,1)/2, P2(:,2)/2, (Q1(:,1)+Q2(:,1))/2, (Q1(:,2)+Q2(:,2))/2, len, 2e-3)/len


