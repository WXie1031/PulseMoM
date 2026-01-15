function [Kabc,Kab,Kc,K0] = dgf_euler_angle(pt_start, pt_end, dv)
%  Function:       dgf_euler_angle
%  Description:    Calculate eular angles of among conductors
%  Calls:          
%
%  Input:          pt_start  --  start point of conductors (N*3) (m)
%                  pt_end    --  end point of conductors (N*3) (m)
%                  dv        --  direction vector (N*3)
%  Output:         Kabc --  
%                  Kab  -- 
%                  Kc   -- 
%                  K0   --
%  Others:
%  Author:         Chen Hongcai
%  Email :         hc.chen@live.com
%  Date:           2016-03-08

Nc = size(pt_start,1);
Kabc = zeros(Nc,Nc);
Kab  = zeros(Nc,Nc);
Kc   = zeros(Nc,Nc);
K0   = zeros(Nc,Nc);

pt_mid = (pt_start+pt_end)/2; % middle of the conductor

for ik = 1:Nc
    
    % Kabc(ik,:) = dv(ik,1).*dv(:,1) + dv(ik,2).*dv(:,2) + dv(ik,3).*dv(:,3);
    Kab(ik,:)  = dv(ik,1).*dv(:,1) + dv(ik,2).*dv(:,2);
    Kc(ik,:)   = dv(ik,3).*dv(:,3);
    Kabc(ik,:) = Kab(ik,:)+Kc(ik,:);
    
    dx = pt_mid(ik,1)-pt_mid(:,1);
    dy = pt_mid(ik,2)-pt_mid(:,2);
    dij = sqrt( dx.^2 + dy.^2 );
    cosij  = ( dx.*dv(:,1) + dy.*dv(:,2) ) ./ dij;
    cosji  = ( -dx.*dv(ik,1) - dy.*dv(ik,2) ) ./ dij;
    
    K0(ik,:) = cosij.*dv(ik,3).*sqrt(1-dv(:,3).^2) ...
        + cosji.*dv(:,3).*sqrt(1-dv(ik,3).^2);

end


