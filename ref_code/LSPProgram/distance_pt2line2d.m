function d0 = distance_pt2line2d(Q1, Q2, P, ver)
%  Function:       distance_pt2line2d
%  Description:    calculate the distance between a point and a line in 
%                  the same plane (2D). 1 line and multi points.
%  Calls:          
%  Input:          Q1   --  start point of the line (x,y,z)
%                  Q2   --  end point of the line (x,y,z)
%                  P    --  coordinate of point (x,y,z)
%  Output:         d0   --  distance between two points
%  Others:         
%  Author:         Chen Hongcai    
%  Email :         hc.chen@live.com      
%  Date:           2015-12-13

Npt = size(P,1);


Q2 = ones(Npt,1)*Q2;
Q1 = ones(Npt,1)*Q1;

% ver - 1: return the distance with the sign
% ver - others: return the absolute distance
if ver == 1
    d0 = (dot(Q2-Q1,P-Q1))/norm(Q2-Q1); 
else
    d0 = abs(dot(Q2-Q1,P-Q1))/norm(Q2-Q1); 
end


end


