%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% int_bar_p_3d   Calculate the Bar-Bar integral. Both Bars lay along z axis.
%               If those lay in other directions, transfrom the axis
%               to specific directions.                              
% %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% INPUT %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Xox           [N*1] start and end x-axis of the bar(source)
% Yox           [N*1] start and end y-axis of the bar(source)
% Zox           [N*1] start and end z-axis of the tape(source)
% Xfx           [N*1] start and end x-axis of the filament(field)
% Yfx           [N*1] start and end y-axis of the filament(field)
% Zfx           [N*1] start and end z-axis of the filament(filed)
% %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% PARAMETERS %%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% qx            [N*1] x difference of bars 
% rx            [N*1] y difference of bars
% sx            [N*1] z difference of bars
% %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% OUTPUT %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% int           result of the integral
% %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% revised by Chen Hongcai 2014.5
function int = int_bar_p_3d(pt_mid_s, d1_s,d2_s,d3_s, pt_mid_f, d1_f,d2_f,d3_f)
%(Xo1, Xo2, Yo1, Yo2, Zo1, Zo2, Xf1, Xf2, Yf1, Yf2, Zf1, Zf2)

No = size(pt_mid_s,1);
Nf = size(pt_mid_f,1);
    
if No>0 && Nf>0
    Xo1 = pt_mid_s(:,1) - d1_s/2;
    Xo2 = pt_mid_s(:,1) + d1_s/2;
    Yo1 = pt_mid_s(:,2) - d2_s/2;
    Yo2 = pt_mid_s(:,2) + d2_s/2;
    Zo1 = pt_mid_s(:,3) - d3_s/2;
    Zo2 = pt_mid_s(:,3) + d3_s/2;
    
    Xf1 = pt_mid_f(:,1) - d1_f/2;
    Xf2 = pt_mid_f(:,1) + d1_f/2;
    Yf1 = pt_mid_f(:,2) - d2_f/2;
    Yf2 = pt_mid_f(:,2) + d2_f/2;
    Zf1 = pt_mid_f(:,3) - d3_f/2;
    Zf2 = pt_mid_f(:,3) + d3_f/2;

%     q1 = repmat(Xf1',No,1) - repmat(Xo2,1,Nf);
%     q2 = repmat(Xf2',No,1) - repmat(Xo2,1,Nf);
%     q3 = repmat(Xf2',No,1) - repmat(Xo1,1,Nf);
%     q4 = repmat(Xf1',No,1) - repmat(Xo1,1,Nf);
% 
%     r1 = repmat(Yf1',No,1) - repmat(Yo1,1,Nf);
%     r2 = repmat(Yf2',No,1) - repmat(Yo1,1,Nf);
%     r3 = repmat(Yf2',No,1) - repmat(Yo2,1,Nf);
%     r4 = repmat(Yf1',No,1) - repmat(Yo2,1,Nf);
% 
%     s1 = repmat(Zf1',No,1) - repmat(Zo2,1,Nf);
%     s2 = repmat(Zf2',No,1) - repmat(Zo2,1,Nf);
%     s3 = repmat(Zf2',No,1) - repmat(Zo1,1,Nf);
%     s4 = repmat(Zf1',No,1) - repmat(Zo1,1,Nf);

    q1 = Xf1 - Xo2;
    q2 = Xf2 - Xo2;
    q3 = Xf2 - Xo1;
    q4 = Xf1 - Xo1;
    r1 = Yf1 - Yo1;
    r2 = Yf2 - Yo1;
    r3 = Yf2 - Yo2;
    r4 = Yf1 - Yo2;
    s1 = Zf1 - Zo2;
    s2 = Zf2 - Zo2;
    s3 = Zf2 - Zo1;
    s4 = Zf1 - Zo1;

    I1  = int_bar_p_3d_sub(q1, r1, s1);
    I2  = int_bar_p_3d_sub(q1, r1, s2);
    I3  = int_bar_p_3d_sub(q1, r1, s3);
    I4  = int_bar_p_3d_sub(q1, r1, s4);

    I5  = int_bar_p_3d_sub(q1, r2, s1);
    I6  = int_bar_p_3d_sub(q1, r2, s2);
    I7  = int_bar_p_3d_sub(q1, r2, s3);
    I8  = int_bar_p_3d_sub(q1, r2, s4);

    I9  = int_bar_p_3d_sub(q1, r3, s1);
    I10 = int_bar_p_3d_sub(q1, r3, s2);
    I11 = int_bar_p_3d_sub(q1, r3, s3);
    I12 = int_bar_p_3d_sub(q1, r3, s4);

    I13 = int_bar_p_3d_sub(q1, r4, s1);
    I14 = int_bar_p_3d_sub(q1, r4, s2);
    I15 = int_bar_p_3d_sub(q1, r4, s3);
    I16 = int_bar_p_3d_sub(q1, r4, s4);

    % % % % % % % %

    I17 = int_bar_p_3d_sub(q2, r1, s1);
    I18 = int_bar_p_3d_sub(q2, r1, s2);
    I19 = int_bar_p_3d_sub(q2, r1, s3);
    I20 = int_bar_p_3d_sub(q2, r1, s4);

    I21 = int_bar_p_3d_sub(q2, r2, s1);
    I22 = int_bar_p_3d_sub(q2, r2, s2);
    I23 = int_bar_p_3d_sub(q2, r2, s3);
    I24 = int_bar_p_3d_sub(q2, r2, s4);

    I25 = int_bar_p_3d_sub(q2, r3, s1);
    I26 = int_bar_p_3d_sub(q2, r3, s2);
    I27 = int_bar_p_3d_sub(q2, r3, s3);
    I28 = int_bar_p_3d_sub(q2, r3, s4);

    I29 = int_bar_p_3d_sub(q2, r4, s1);
    I30 = int_bar_p_3d_sub(q2, r4, s2);
    I31 = int_bar_p_3d_sub(q2, r4, s3);
    I32 = int_bar_p_3d_sub(q2, r4, s4);

    % % % % % % % % % % % %

    I33 = int_bar_p_3d_sub(q3, r1, s1);
    I34 = int_bar_p_3d_sub(q3, r1, s2);
    I35 = int_bar_p_3d_sub(q3, r1, s3);
    I36 = int_bar_p_3d_sub(q3, r1, s4);

    I37 = int_bar_p_3d_sub(q3, r2, s1);
    I38 = int_bar_p_3d_sub(q3, r2, s2);
    I39 = int_bar_p_3d_sub(q3, r2, s3);
    I40 = int_bar_p_3d_sub(q3, r2, s4);

    I41 = int_bar_p_3d_sub(q3, r3, s1);
    I42 = int_bar_p_3d_sub(q3, r3, s2);
    I43 = int_bar_p_3d_sub(q3, r3, s3);
    I44 = int_bar_p_3d_sub(q3, r3, s4);

    I45 = int_bar_p_3d_sub(q3, r4, s1);
    I46 = int_bar_p_3d_sub(q3, r4, s2);
    I47 = int_bar_p_3d_sub(q3, r4, s3);
    I48 = int_bar_p_3d_sub(q3, r4, s4);

    % % % % % % % % % % % %

    I49 = int_bar_p_3d_sub(q4, r1, s1);
    I50 = int_bar_p_3d_sub(q4, r1, s2);
    I51 = int_bar_p_3d_sub(q4, r1, s3);
    I52 = int_bar_p_3d_sub(q4, r1, s4);

    I53 = int_bar_p_3d_sub(q4, r2, s1);
    I54 = int_bar_p_3d_sub(q4, r2, s2);
    I55 = int_bar_p_3d_sub(q4, r2, s3);
    I56 = int_bar_p_3d_sub(q4, r2, s4);

    I57 = int_bar_p_3d_sub(q4, r3, s1);
    I58 = int_bar_p_3d_sub(q4, r3, s2);
    I59 = int_bar_p_3d_sub(q4, r3, s3);
    I60 = int_bar_p_3d_sub(q4, r3, s4);

    I61 = int_bar_p_3d_sub(q4, r4, s1);
    I62 = int_bar_p_3d_sub(q4, r4, s2);
    I63 = int_bar_p_3d_sub(q4, r4, s3);
    I64 = int_bar_p_3d_sub(q4, r4, s4);

    % % % % % % % % % %

    int = -(I1-I2+I3-I4) + (I5-I6+I7-I8) - (I9-I10+I11-I12) + (I13-I14+I15-I16) ...
        + (I17-I18+I19-I20) - (I21-I22+I23-I24) + (I25-I26+I27-I28) - (I29-I30+I31-I32) ...
        - (I33-I34+I35-I36) + (I37-I38+I39-I40) - (I41-I42+I43-I44) + (I45-I46+I47-I48) ...
        + (I49-I50+I51-I52) - (I53-I54+I55-I56) + (I57-I58+I59-I60) - (I61-I62+I63-I64) ;
else
    int = [];
end


end


