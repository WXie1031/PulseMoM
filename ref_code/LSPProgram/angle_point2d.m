function [d0, a0] = angle_point2d( p02D, pf2D )
%  Function:       angle_line2D
%  Description:    calculate the distance and angle between two points in 
%                  the same plane (2D for the angle)
%  Calls:          
%  Input:          p02D --  original point
%                  pf2D --  field point
%  Output:         d0   --  distance between two points
%                  a0   --  angle between two points
%  Others:         
%  Author:         Chen Hongcai    
%  Email :         hc.chen@live.com      
%  Date:           2013-11-13
%  Update:         


dX = pf2D(:,1)-p02D(1);
dY = pf2D(:,2)-p02D(2);
    
d0 = sqrt( dX.^2+dY.^2 );
a0 = atan2(dY,dX);  % [0,2*pi]

ind = a0<0;
a0(ind) = a0(ind)+2*pi;


end

   
