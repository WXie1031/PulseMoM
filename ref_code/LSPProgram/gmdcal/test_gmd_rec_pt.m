% from paper with GMD, AMSD, AMD
W = 4e-3;
T = 40e-3;
len = 100;

P0 = [0,0];

P1=P0;
P1(1)=P1(1)-W/2;P1(2)=P1(2)-T/2;
P2=P0;
P2(1)=P2(1)-W/2;P2(2)=P2(2)+T/2;
P3=P0;
P3(1)=P3(1)+W/2;P3(2)=P3(2)+T/2;
P4=P0;
P4(1)=P4(1)+W/2;P4(2)=P4(2)-T/2;

Q=[40e-3,100e-3];


gmd1 = gmd_line_line_a(P1,P2, Q(1),Q(2));
gmd2 = gmd_rec_pt_Maxwell(P1,P2,P3,P4, Q)

L1 = (induct_gmd(gmd1,len))/len
L2 = (induct_gmd(gmd2,len))/len
%L3 = 1e-7/W/( Q1(:,2)-Q2(:,2))*int_tape_p2d(0, W, 0, Q1(:,2), Q2(:,2), -10e-3, len)/len
Xo1 = P1(1);
Xo2 = P3(1);
Yo1 = P1(2);
Yo2 = P3(2);
Xf=Q(1);Yf=Q(2);
1e-7/W/T*int_bar_line_2d(Xo1, Xo2, Yo1, Yo2, Xf, Yf, len)

