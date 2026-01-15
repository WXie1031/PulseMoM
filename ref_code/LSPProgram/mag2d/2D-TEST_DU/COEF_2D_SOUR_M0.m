%COEF_2D_SOUR_M0	 NUMERICAL INTEGRATION of 2D cell coefficients (2 fold)
%            cell-from-source lines
%            subscript "1" = observation, "2" = source (curr. dir. = U/u)
%            
%            METHOD 0 (point-match)
%
%            constant Is
%
% As:         T1=sum[-0.5*log(dx^2+dy^2)*Is] (As)
% Bsx;        T2=sum[-dy/(dx^2+dy^2)*Is]     (Bsx)
% Bsy:        T3=sum[ dx/(dx^2+dy^2)*Is]     (Bsy)
%
% Lxy=[x1 y1; x2 y2; ...]    coorindates of source lines 
% Oxy=[x1 y1; x2 y2; ...]    coorindates of observation points 

% edited on 8 July 2009

function [P1 P2 P3]=COEF_2D_SOUR_M0(Lxy,Is,Oxy)
[ns mm]=size(Lxy);
[no mm]=size(Oxy);
P1=0; P2=0; P3=0; 

for i=1:ns
    dx=Oxy(:,1)-Lxy(i,1);
    dy=Oxy(:,2)-Lxy(i,2);
    x2=dx.*dx;
    y2=dy.*dy;
    r2=x2+y2;
    P1=P1-0.5*log(r2)*Is(i);
    P2=P2-dy./r2*Is(i);
    P3=P3+dx./r2*Is(i);
end

end