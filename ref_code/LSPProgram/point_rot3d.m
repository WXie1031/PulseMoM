function Puvw1 = point_rot3d(Pxyz1, ax, ay, az)
%  Function:       rot3D
%  Description:    rotate 
%  Calls:          angle_line
%  Input:          ps1  --  coordinate of start point of source line
%                  ps2  --  coordinate of end point of source line
%                  dv1  --  direction vector of source line
%                  l1   --  length of source line
%                  pf1  --  coordinate of start point of field line
%                  pf2  --  coordinate of end point of field line
%                  dv2  --  direction vector of field line
%                  l2   --  length of field line
%  Output:         int  --  integral result
%  Others:         
%  Author:         Chen Hongcai    
%  Email :         hc.chen@live.com      
%  Date:           2014-12-13
%  History:         
%      <author>      <time>       <desc>
%      David         96/10/12     build this moudle  


%Pxyz = [x y z 1]
T_xa = [1  0  0;  0  cos(ax)  sin(ax);  0  -sin(ax)  cos(ax);];
T_yb = [cos(ay)  0  -sin(ay);  0  1  0; sin(ay)  0  cos(ay);];
T_zc = [cos(az) sin(az)  0;  -sin(az)  cos(az)  0;  0  0  1;];

Tr = T_xa * T_yb * T_zc;


% Ax = Axyz(1);
% Ay = Axyz(2);
% Az = Axyz(3);
% 
% Tr =  ...
% [                    cos(Ay)*cos(Az)                     -cos(Ay)*sin(Az)         sin(Ay)   ;  ...
% cos(Ax)*sin(Az)+cos(Az)*sin(Ax)*sin(Ay) cos(Ax)*cos(Az)-sin(Ax)*sin(Ay)*sin(Az) -cos(Ay)*sin(Ax); ...
% sin(Ax)*sin(Az)-cos(Ax)*cos(Az)*sin(Ay) cos(Az)*sin(Ax)+cos(Ax)*sin(Ay)*sin(Az)  cos(Ax)*cos(Ay);];      


Puvw1 = Pxyz1*Tr;

end    
    
    