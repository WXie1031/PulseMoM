%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% INT_TAPE_V_SEG2D Calculate the Perpendicular Tape-Tape integral.(parallel
%               equal length) Tape1(source) lay on x-z plane and 
%               Tape2(field) lay on y-z plane. If tape lay in other planes,
%               transfrom the axis to specific planes.                  
% %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% INPUT %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Xox           [N*1] start and end x-axis of the tape1(source)
% Yo            [N*1] y-axis of the tape1(source)
% Xf            [N*1] x-axis of the tape2(field)
% Yfx           [N*1] start and end y-axis of the tape2(field)
% len           [N*1] length of the tape
% %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% PARAMETERS %%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% qx            [N*1] x difference of bar and filament
% rx            [N*1] y difference of bar and filament
% %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% OUTPUT %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% int           result of the integral
% %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% revised by Chen Hongcai 2014.5
function int = int_tape_v_p3d(Xo1, Xo2, Yo, Xf, Yf1, Yf2, len)

No = length(Xo1);
Nf = length(Xf);
   
if No>0 && Nf>0
%     q1 = repmat(Xf',No,1) - repmat(Xo1,1,Nf);
%     q2 = repmat(Xf',No,1) - repmat(Xo2,1,Nf);
%     r1 = repmat(Yf2',No,1) - repmat(Yo,1,Nf);
%     r2 = repmat(Yf1',No,1) - repmat(Yo,1,Nf);

    if length(len) == 1
        len = len*ones(No,Nf);
    elseif length(len) == No
        len = len*ones(1,Nf);
    elseif length(len) == Nf
        len = ones(No,1)*len';
    end

    q1 = Xf - Xo1;
    q2 = Xf - Xo2;
    r1 = Yf2 - Yo;
    r2 = Yf1 - Yo;

    I1 = int_tape_v_p3d_sub(q1, r1, len);
    I2 = int_tape_v_p3d_sub(q2, r1, len);
    I3 = int_tape_v_p3d_sub(q1, r2, len);
    I4 = int_tape_v_p3d_sub(q2, r2, len);

    int = ( I1 - I2 - I3 + I4 );
else
    int = zeros(No,1)*zeros(1,Nf);
end


end


