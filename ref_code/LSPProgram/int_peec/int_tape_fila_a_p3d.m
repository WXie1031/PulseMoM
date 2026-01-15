%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% INT_LINE_PT_2D calculate the line-point integral (parallel equal length)
% %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% INPUT %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% X1            [1*3] start point of the line
% X2            [1*3] end point of the line
% Xm            [1*3] middle point of the line
% XVs           [N*3] source points
% l             [N*1] length of the conductor
% %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% PARAMETERS %%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% X1n           [1*3] new axis of start point (after transformation)
% X2n           [1*3] new axis of end point (after transformation)
% Xsn           [N*3] new axis of source points (after transformation)
% %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% OUTPUT %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% int           result of the integral
% %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% revised by Chen Hongcai 2014.2
function int = int_tape_fila_a_p3d(P1,P2, Qpt, l)


[P1new,P2new, Qptnew] = axis_trans2line(P1,P2, Qpt);

x1 = P1new(:,1);
x2 = P2new(:,1);

xs = Qptnew(:,1);
ys = Qptnew(:,2);

W = x2-x1;
d02 = xs.^2+ys.^2;

ys2 = ys.*ys;
x1_xs = x1-xs;
x2_xs = x2-xs;
d12 = (x1_xs).^2+ys.^2;
d22 = (x2_xs).^2+ys.^2;

I1 = ( 2*l.*log(l+sqrt(l.*l+d02)) - 2*sqrt(l.*l+d02) ).*(x2-x1);

I21 = x1 - x1_xs/2.*log(d12) - ys.*atan(x1_xs./ys);
I22 = x2 - x2_xs/2.*log(d22) - ys.*atan(x2_xs./ys);

I31 = x1_xs.*sqrt(d12) + ys2.*log( max(x1_xs+sqrt(d12),1e-8) ); % avoid 0
I32 = x2_xs.*sqrt(d22) + ys2.*log( max(x2_xs+sqrt(d22),1e-8) );

int = ( I1 + 2*l.*(I22-I21) + (I32-I31) )./W;


end

