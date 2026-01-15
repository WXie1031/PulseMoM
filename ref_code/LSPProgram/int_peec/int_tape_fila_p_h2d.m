%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% int_tape_line_p_3d Calculate the Parallel Tape-Filament integral.
%               Tapes lay on x-z plane. If tape lay in other planes,
%               transfrom the axis to x-z plane.        
% %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% INPUT %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Xox           [N*1] start and end x-axis of the tape1(source)
% Yo            [N*1] start and end y-axis of the tape1(source)
% Zox           [N*1] start and end z-axis of the tape1(source)
% Xfz           [N*1] start and end x-axis of the tape2(field)
% Yf            [N*1] start and end y-axis of the tape2(field)
% Zfz           [N*1] start and end z-axis of the tape2(field)
% %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% PARAMETERS %%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% qx            [N*1] x difference of two tapes
% P             [N*1] vertical height between two tapes (y difference)
% sx            [N*1] z difference of two tapes
% %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% OUTPUT %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% int           result of the integral
% %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% revised by Chen Hongcai 2014.5
function int = int_tape_fila_p_h2d(pt_mid_s, dv_s, d1_s,d2_s, pt_mid_f, dv_f, d1_f,d2_f)
%(Xo1, Xo2, Yo, Zo1, Zo2, Xf, Yf, Zf1, Zf2)

No = size(pt_mid_s,1);
Nf = size(pt_mid_f,1);
    
if No>0 && Nf>0
    
    err = 1e-3;
    % corresponding to the mesh_plate_3d
    if sum(abs(dv_s-[1 0 0]))<=err  % x cell  (x y)
        Xo1 = pt_mid_s(:,2) - d2_s/2;
        Xo2 = pt_mid_s(:,2) + d2_s/2;
        Zo1 = pt_mid_s(:,1) - d1_s/2;
        Zo2 = pt_mid_s(:,1) + d1_s/2;
        
        Xf = pt_mid_f(:,2);
        Zf1 = pt_mid_f(:,1) - d1_f/2;
        Zf2 = pt_mid_f(:,1) + d1_f/2;
        
    elseif sum(abs(dv_s-[0 1 0]))<=err  % y cell  (x y)
        Xo1 = pt_mid_s(:,1) - d1_s/2;
        Xo2 = pt_mid_s(:,1) + d1_s/2;
        Zo1 = pt_mid_s(:,2) - d2_s/2;
        Zo2 = pt_mid_s(:,2) + d2_s/2;
        
        Xf = pt_mid_f(:,1);
        Zf1 = pt_mid_f(:,2) - d2_f/2;
        Zf2 = pt_mid_f(:,2) + d2_f/2;
    else
        error('Incline tape is not surpported in "int_tape_p_3d."');
    end

%     q1 = repmat(Xf',No,1) - repmat(Xo1,1,Nf);
%     q2 = repmat(Xf',No,1) - repmat(Xo2,1,Nf);
%     s1 = repmat(Zf1',No,1) - repmat(Zo2,1,Nf);
%     s2 = repmat(Zf2',No,1) - repmat(Zo2,1,Nf);
%     s3 = repmat(Zf2',No,1) - repmat(Zo1,1,Nf);
%     s4 = repmat(Zf1',No,1) - repmat(Zo1,1,Nf);

    q1 = Xf - Xo1;
    q2 = Xf - Xo2;
    s1 = Zf1 - Zo2;
    s2 = Zf2 - Zo2;
    s3 = Zf2 - Zo1;
    s4 = Zf1 - Zo1;

    I1 = int_tape_fila_p_h2d_sub(q1, s1);
    I2 = int_tape_fila_p_h2d_sub(q1, s2);
    I3 = int_tape_fila_p_h2d_sub(q1, s3);
    I4 = int_tape_fila_p_h2d_sub(q1, s4);
    I5 = int_tape_fila_p_h2d_sub(q2, s1);
    I6 = int_tape_fila_p_h2d_sub(q2, s2);
    I7 = int_tape_fila_p_h2d_sub(q2, s3);
    I8 = int_tape_fila_p_h2d_sub(q2, s4);

    int = (I1-I2+I3-I4) - (I5-I6 +I7-I8);
else
    int = zeros(No,1)*zeros(1,Nf);
end


end


