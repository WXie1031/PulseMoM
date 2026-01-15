%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% int_tape_p_2d Calculate the Parallel Tape-Tape integral.Tapes lay 
%               on x-y plane. If tape lay in other planes,
%               transfrom the axis to x-y plane. Tapes should be with same
%               Z axis, which means belong to the same plane.
% %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% INPUT %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Xox           [N*1] start and end x-axis of the tape1(source)
% Zox           [N*1] start and end z-axis of the tape1(source)
% Xfz           [N*1] start and end x-axis of the tape2(field)
% Zfz           [N*1] start and end z-axis of the tape2(field)
% %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% PARAMETERS %%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% qx            [N*1] x difference of two tapes
% sx            [N*1] z difference of two tapes
% %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% OUTPUT %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% int           result of the integral
% %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% revised by Chen Hongcai 2014.5
function int = int_tape_p_h2d(pt_mid_s, d1_s,d2_s, pt_mid_f, d1_f,d2_f)
% (Xo1, Xo2, Yo1, Yo2, Xf1, Xf2, Yf1, Yf2)

No = size(pt_mid_s,1);
Nf = size(pt_mid_f,1);
    
if No>0 && Nf>0

    Xo1 = pt_mid_s(:,1) - d1_s/2;
    Xo2 = pt_mid_s(:,1) + d1_s/2;
    Yo1 = pt_mid_s(:,2) - d2_s/2;
    Yo2 = pt_mid_s(:,2) + d2_s/2;
    
    Xf1 = pt_mid_f(:,1) - d1_f/2;
    Xf2 = pt_mid_f(:,1) + d1_f/2;
    Yf1 = pt_mid_f(:,2) - d2_f/2;
    Yf2 = pt_mid_f(:,2) + d2_f/2;

%     q1 = repmat(Xf1',No,1) - repmat(Xo2,1,Nf);
%     q2 = repmat(Xf2',No,1) - repmat(Xo2,1,Nf);
%     q3 = repmat(Xf2',No,1) - repmat(Xo1,1,Nf);
%     q4 = repmat(Xf1',No,1) - repmat(Xo1,1,Nf);
% 
%     s1 = repmat(Yf1',No,1) - repmat(Yo2,1,Nf);
%     s2 = repmat(Yf2',No,1) - repmat(Yo2,1,Nf);
%     s3 = repmat(Yf2',No,1) - repmat(Yo1,1,Nf);
%     s4 = repmat(Yf1',No,1) - repmat(Yo1,1,Nf);

    q1 = Xf1 - Xo2;
    q2 = Xf2 - Xo2;
    q3 = Xf2 - Xo1;
    q4 = Xf1 - Xo1;
    s1 = Yf1 - Yo2;
    s2 = Yf2 - Yo2;
    s3 = Yf2 - Yo1;
    s4 = Yf1 - Yo1;

    I1  = int_tape_p_h2d_sub(q1, s1);
    I2  = int_tape_p_h2d_sub(q1, s2);
    I3  = int_tape_p_h2d_sub(q1, s3);
    I4  = int_tape_p_h2d_sub(q1, s4);

    I5  = int_tape_p_h2d_sub(q2, s1);
    I6  = int_tape_p_h2d_sub(q2, s2);
    I7  = int_tape_p_h2d_sub(q2, s3);
    I8  = int_tape_p_h2d_sub(q2, s4);

    I9  = int_tape_p_h2d_sub(q3, s1);
    I10 = int_tape_p_h2d_sub(q3, s2);
    I11 = int_tape_p_h2d_sub(q3, s3);
    I12 = int_tape_p_h2d_sub(q3, s4);

    I13 = int_tape_p_h2d_sub(q4, s1);
    I14 = int_tape_p_h2d_sub(q4, s2);
    I15 = int_tape_p_h2d_sub(q4, s3);
    I16 = int_tape_p_h2d_sub(q4, s4);

    int = (I1-I2+I3-I4) - (I5-I6+I7-I8) + (I9-I10+I11-I12) - (I13-I14+I15-I16);
else
    
    int = zeros(No,1)*zeros(1,Nf);
end


end


