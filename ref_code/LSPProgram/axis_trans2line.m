%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% AXIS_T_LINEX  axis transformation to let point Xo and line X1-X2 at X
%               direction
% %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% INPUT %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Xo            origin of new coordinates
% X1            start point of the line
% X2            end point of the line
% XV            (vector) field point
% %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% OUTPUT %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% int           result of the integral
% %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% revised by Chen Hongcai 2014.2
function [P1new,P2new, Qptnew] = axis_trans2line(P1,P2, Qpt)

% transform the origin of coordinates to Xo
x0 = (P1(:,1)+P2(:,1))/2;
y0 = (P1(:,2)+P2(:,2))/2;

x1_tmp = P1(:,1)-x0;
y1_tmp = P1(:,2)-y0;

ang = pi+atan2(y1_tmp,x1_tmp);

x2_tmp = P2(:,1)-x0;
y2_tmp = P2(:,2)-y0;

xpt_tmp = Qpt(:,1)-x0;
ypt_tmp = Qpt(:,2)-y0;


P1new(:,2) = x1_tmp.*sin(ang)-y1_tmp.*cos(ang);
P1new(:,1) = x1_tmp.*cos(ang)+y1_tmp.*sin(ang);

P2new(:,2) = x2_tmp.*sin(ang)-y2_tmp.*cos(ang);
P2new(:,1) = x2_tmp.*cos(ang)+y2_tmp.*sin(ang);

Qptnew(:,2) = xpt_tmp.*sin(ang)-ypt_tmp.*cos(ang);
Qptnew(:,1) = xpt_tmp.*cos(ang)+ypt_tmp.*sin(ang);


end


