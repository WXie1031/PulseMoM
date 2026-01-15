%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% int_tape_p2d Calculate the Parallel Tape-Tape integral.(parallel equal 
%               length) Tapes lay on x-z plane. If tape lay in other planes,
%               transfrom the axis to x-z plane.
% %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% INPUT %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Xox           [N*1] start and end x-axis of the tape1(source)
% Yo            [N*1] start and end y-axis of the tape1(source)
% Xfz           [N*1] start and end x-axis of the tape2(field)
% Yf            [N*1] start and end y-axis of the tape2(field)
% l             [N*1] length of the tape
% %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% PARAMETERS %%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% qx            [N*1] x difference of two tapes
% P             [N*1] vertical height between two tapes
% %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% OUTPUT %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% int           result of the integral
% %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% revised by Chen Hongcai 2014.5
function int = int_tape_p_p3d(Xo1, Xo2, Yo, Xf1, Xf2, Yf, len)

No = length(Xo1);
Nf = length(Xf1);
    
if No>0 && Nf>0
    
%     P = repmat(Yf',No,1) - repmat(Yo,1,Nf);

%     q1 = repmat(Xf1',No,1) - repmat(Xo2,1,Nf);
%     q2 = repmat(Xf2',No,1) - repmat(Xo2,1,Nf);
%     q3 = repmat(Xf2',No,1) - repmat(Xo1,1,Nf);
%     q4 = repmat(Xf1',No,1) - repmat(Xo1,1,Nf);

    if length(len) == 1
        len = len*ones(No,Nf);
    elseif length(len) == No
        len = len*ones(1,Nf);
    elseif length(len) == Nf
        len = ones(No,1)*len';
    end

    P = Yf - Yo;
    q1 = Xf1 - Xo2;
    q2 = Xf2 - Xo2;
    q3 = Xf2 - Xo1;
    q4 = Xf1 - Xo1;

    I1 = int_tape_p_p3d_sub(q1, P, len);
    I2 = int_tape_p_p3d_sub(q2, P, len);
    I3 = int_tape_p_p3d_sub(q3, P, len);
    I4 = int_tape_p_p3d_sub(q4, P, len);


    int = (I1 - I2 + I3 - I4);
else
    int = zeros(No,1)*zeros(1,Nf);
end


end

