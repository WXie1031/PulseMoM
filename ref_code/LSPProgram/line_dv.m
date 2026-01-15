function [dv, len] = line_dv(pt_start, pt_end)
%  Function:       angle_line
%  Description:    Calculate angle between two lines
%  Calls:          
%  Input:          po1  --  coordinate of the strat point of line
%                  po2  --  coordinate of the end point of line
%  Output:         dv   --  direction vector of line
%                  lc   --  length of line
%  Others:         
%  Author:         Chen Hongcai    
%  Email :         hc.chen@live.com      
%  Date:           2014-12-13
%  History:         
%      <author>      <time>       <desc>
%      David         96/10/12     build this moudle  

if nargin <2
    dx = pt_start(:,4)-pt_start(:,1);
    dy = pt_start(:,5)-pt_start(:,2);
    dz = pt_start(:,6)-pt_start(:,3);
else
    dx = pt_end(:,1)-pt_start(:,1);
    dy = pt_end(:,2)-pt_start(:,2);
    dz = pt_end(:,3)-pt_start(:,3);
end

DX = abs(dx);
DY = abs(dy);
DZ = abs(dz);


IX = sign(DX);
IY = sign(DY);
IZ = sign(DZ);


NDIR = IX*1 + IY*2 + IZ*3;

Nc = length(NDIR);
cosa = zeros(Nc,3); 
len = zeros(Nc,1);

for k = 1:Nc
    switch NDIR(k)
        case 1
            %NDIR(k) = NDIR(k);
            len(k) = DX(k);
            cosa(k,1) = dx(k)/len(k);
            cosa(k,2) = 0;
            cosa(k,3) = 0;
        case 2
            %NDIR(k) = NDIR(k);
            len(k) = DY(k);
            cosa(k,1) = 0;
            cosa(k,2) = dy(k)/len(k);
            cosa(k,3) = 0;
        case 3
            if IZ(k) ~= 0
                
                %NDIR(k) = NDIR(k);
                len(k) = DZ(k);
                cosa(k,1) = 0;
                cosa(k,2) = 0;
                cosa(k,3) = dz(k)/len(k);
            else
                % on x-y plane
                len(k) = sqrt(DX(k)^2 + DY(k)^2);
                cosa(k,1) = dx(k)/len(k);
                cosa(k,2) = dy(k)/len(k);
                cosa(k,3) = 0;
            end
            
        otherwise

            len(k) = sqrt(DX(k)^2 + DY(k)^2 + DZ(k)^2);
            cosa(k,1) = dx(k)/len(k);
            cosa(k,2) = dy(k)/len(k);
            cosa(k,3) = dz(k)/len(k);
    end
end

dv = cosa;


end


