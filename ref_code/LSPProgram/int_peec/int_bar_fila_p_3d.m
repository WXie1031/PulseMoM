%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% INT_BAR_LINE_2D Calculate the Bar-Filament integral.
%               Both Bar(source) and Filament(field)lay
%               along z axis. If those lay in other directions,
%               transfrom the axis to specific directions.
% %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% INPUT %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Xox           [N*1] start and end x-axis of the bar(source)
% Yox           [N*1] start and end y-axis of the bar(source)
% Zox           [N*1] start and end z-axis of the bar(source)
% Xf            [N*1] x-axis of the filament(field)
% Yf            [N*1] y-axis of the filament(field)
% Zfx           [N*1] z-axis of the filament(filed)
% %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% PARAMETERS %%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% qx            [N*1] x difference of bar and filament
% rx            [N*1] y difference of bar and filament
% sx            [N*1] z difference of bar and filament
% %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% OUTPUT %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% int           result of the integral
% %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% revised by Chen Hongcai 2014.5
function int = int_bar_fila_p_3d(pt_mid_s, dv_s, d1_s,d2_s,d3_s, pt_mid_f, dv_f, d1_f,d2_f,d3_f)
% (Xo1, Xo2, Yo1, Yo2, Zo1, Zo2, Xf, Yf, Zf1, Zf2)

No = size(pt_mid_s,1);
Nf = size(pt_mid_f,1);

if No>0 && Nf>0
    
    Xo1 = zeros(Nf,1);
    Xo2 = zeros(Nf,1);
    Yo1 = zeros(Nf,1);
    Yo2 = zeros(Nf,1);
    Zo1 = zeros(Nf,1);
    Zo2 = zeros(Nf,1);
    
    Xf  = zeros(Nf,1);
    Yf = zeros(Nf,1);
    Zf1 = zeros(Nf,1);
    Zf2 = zeros(Nf,1);
    
    err = 1e-3;
    % corresponding to the mesh_plate_3d
     if sum(abs(dv_s-[1 0 0]))<err  % x cell (y, z)
        
        Xo1 = pt_mid_s(:,2) - d2_s/2;
        Xo2 = pt_mid_s(:,2) + d2_s/2;
        Yo1 = pt_mid_s(:,3) - d3_s/2;
        Yo2 = pt_mid_s(:,3) + d3_s/2;
        Zo1 = pt_mid_s(:,1) - d1_s/2;
        Zo2 = pt_mid_s(:,1) + d1_s/2;

        Xf  = pt_mid_f(:,2);
        Yf  = pt_mid_f(:,3);
        Zf1 = pt_mid_f(:,1) - d1_f/2;
        Zf2 = pt_mid_f(:,1) + d1_f/2;
        
    elseif sum(abs(dv_s-[0 1 0]))<err  % y cell (x, z)
    
        Xo1 = pt_mid_s(:,1) - d1_s/2;
        Xo2 = pt_mid_s(:,1) + d1_s/2;
        Yo1 = pt_mid_s(:,3) - d3_s/2;
        Yo2 = pt_mid_s(:,3) + d3_s/2;
        Zo1 = pt_mid_s(:,2) - d2_s/2;
        Zo2 = pt_mid_s(:,2) + d2_s/2;

        Xf  = pt_mid_f(:,1);
        Yf  = pt_mid_f(:,3);
        Zf1 = pt_mid_f(:,2) - d2_f/2;
        Zf2 = pt_mid_f(:,2) + d2_f/2;
        
    elseif sum(abs(dv_s-[0 0 1]))<err % z cell (x, z)
        
        Xo1 = pt_mid_s(:,1) - d1_s/2;
        Xo2 = pt_mid_s(:,1) + d1_s/2;
        Yo1 = pt_mid_s(:,2) - d2_s/2;
        Yo2 = pt_mid_s(:,2) + d2_s/2;
        Zo1 = pt_mid_s(:,3) - d3_s/2;
        Zo2 = pt_mid_s(:,3) + d3_s/2;

        Xf  = pt_mid_f(:,1);
        Yf  = pt_mid_f(:,2);
        Zf1 = pt_mid_f(:,3) - d3_f/2;
        Zf2 = pt_mid_f(:,3) + d3_f/2;
    end
    
    
    % q1 = repmat(Xf',No,1) - repmat(Xo1,1,Nf);
    % q2 = repmat(Xf',No,1) - repmat(Xo2,1,Nf);
    %
    % r1 = repmat(Yf',No,1) - repmat(Yo1,1,Nf);
    % r2 = repmat(Yf',No,1) - repmat(Yo2,1,Nf);
    %
    % s1 = repmat(Zf1',No,1) - repmat(Zo2,1,Nf);
    % s2 = repmat(Zf2',No,1) - repmat(Zo2,1,Nf);
    % s3 = repmat(Zf2',No,1) - repmat(Zo1,1,Nf);
    % s4 = repmat(Zf1',No,1) - repmat(Zo1,1,Nf);
    
    q1 = Xf - Xo1;
    q2 = Xf - Xo2;
    r1 = Yf - Yo1;
    r2 = Yf - Yo2;
    s1 = Zf1 - Zo2;
    s2 = Zf2 - Zo2;
    s3 = Zf2 - Zo1;
    s4 = Zf1 - Zo1;
    
    I1  = int_tape_v_3d_sub(q1, r1, s1);
    I2  = int_tape_v_3d_sub(q1, r1, s2);
    I3  = int_tape_v_3d_sub(q1, r1, s3);
    I4  = int_tape_v_3d_sub(q1, r1, s4);
    
    I5  = int_tape_v_3d_sub(q1, r2, s1);
    I6  = int_tape_v_3d_sub(q1, r2, s2);
    I7  = int_tape_v_3d_sub(q1, r2, s3);
    I8  = int_tape_v_3d_sub(q1, r2, s4);
    
    I9  = int_tape_v_3d_sub(q2, r1, s1);
    I10 = int_tape_v_3d_sub(q2, r1, s2);
    I11 = int_tape_v_3d_sub(q2, r1, s3);
    I12 = int_tape_v_3d_sub(q2, r1, s4);
    
    I13 = int_tape_v_3d_sub(q2, r2, s1);
    I14 = int_tape_v_3d_sub(q2, r2, s2);
    I15 = int_tape_v_3d_sub(q2, r2, s3);
    I16 = int_tape_v_3d_sub(q2, r2, s4);
    
    int = (I1-I2+I3-I4) - (I5-I6+I7-I8) - (I9-I10+I11-I12) + (I13-I14+I15-I16);
    
else
    int = zeros(No,1)*zeros(1,Nf);
end


