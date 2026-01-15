function [Td, TdT] = sol_td(pt_start, pt_end, dt)
%  Function:       dg_retard_time
%  Description:    Calculate retarded time of among conductors
%  Calls:          
%
%  Input:          pt_start  --  start point of conductors (N*3) (m)
%                  pt_end    --  end point of conductors (N*3) (m)
%                  dt        --  time interval of time analysis
%  Output:         Td   --  retarded time
%                  TdT  --  retarded time in time analysis (N*dt)
%                  Ntd  --  max num of retarded time 
%  Others:
%  Author:         Chen Hongcai
%  Email :         hc.chen@live.com
%  Date:           2016-03-8


C = 299792458; % the speed of the light in vacuum m/s


% ##################### caculate the relay of all conductors ########################
% td = distance/velocity
if ~isempty(pt_start)
    
    if nargin == 3
        pt_mid = (pt_start+pt_end)/2; % middle of the conductor
    elseif nargin < 3
        pt_mid = pt_start;
        dt = pt_end;
    end
    
    Nps = size(pt_start,1);
    
    Npf = size(pt_start,1);
    Td = zeros(Nps,Npf);
    TdT = zeros(Nps,Npf);
    
    for ik = 1:Nps
        dis = sqrt( (pt_mid(ik,1)-pt_mid(:,1)).^2 + ...
            (pt_mid(ik,2)-pt_mid(:,2)).^2 + (pt_mid(ik,3)-pt_mid(:,3)).^2 );
        tdtmp = dis./C;
        Nd = round(tdtmp./dt)';
        %Nd = floor(tdtmp./dt)';
        TdT(ik,:) = Nd;
        Td(ik,:) = tdtmp;
        %Td(ik,:) = Nd*dt;
    end
else
    Td = [];
    TdT = [];
end


end



