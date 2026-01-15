function d0 = distance_pt2line3d(Q1, Q2, P)
%  Function:       distance_pt2line3d
%  Description:    calculate the distance between a point and a line in 
%                  the same plane (3D). 1 line and multi points.
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


d0 = norm(cross(Q2-Q1,P-Q1))/norm(Q2-Q1);


end


   