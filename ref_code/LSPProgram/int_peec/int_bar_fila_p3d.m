%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% INT_BAR_LINE_SEG2D Calculate the Bar-Filament integral.(parallel
%               equal length) Both Bar(source) and Filament(field)lay 
%               along z axis. If those lay in other directions,
%               transfrom the axis to specific directions.                  
% %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% INPUT %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Xox           [N*1] start and end x-axis of the bar(source)
% Yox           [N*1] start and end y-axis of the bar(source)
% Xf            [N*1] x-axis of the tape2(field)
% Yf            [N*1] y-axis of the tape2(field)
% len           [N*1] length of the tape
% %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% PARAMETERS %%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% qx            [N*1] x difference of bar and filament
% rx            [N*1] y difference of bar and filament
% %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% OUTPUT %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% int           result of the integral
% %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% revised by Chen Hongcai 2014.5
function int = int_bar_fila_p3d(Xo1, Xo2, Yo1, Yo2, Xf, Yf, l)

No = length(Xo1);
Nf = length(Xf);
    
q1 = repmat(Xf',No,1) - repmat(Xo1,1,Nf);
q2 = repmat(Xf',No,1) - repmat(Xo2,1,Nf);

r1 = repmat(Yf',No,1) - repmat(Yo1,1,Nf);
r2 = repmat(Yf',No,1) - repmat(Yo2,1,Nf);

if length(l) == 1
    l = l*ones(No,Nf);
elseif length(l) == No
    l = l*ones(1,Nf);
elseif length(l) == Nf
    l = ones(No,1)*l';
end

% q1 = Xf - Xo1;
% q2 = Xf - Xo2;
% r1 = Yf - Yo1;
% r2 = Yf - Yo2;

I1 = int_tape_v_p3d_sub(q1, r1, l);
I2 = int_tape_v_p3d_sub(q2, r1, l);
I3 = int_tape_v_p3d_sub(q1, r2, l);
I4 = int_tape_v_p3d_sub(q2, r2, l);



int = ( I1 - I2 - I3 + I4 );

end


