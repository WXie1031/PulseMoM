function [Xs,Ys] = mesh2d_nn_cir(pt_2d, rout, dl)
%  Function:       mesh2d_P_cir
%  Description:    Mesh circular cross sections (2D) on the surface for
%                  using in neural network
%  Calls:
%  Input:          pt_2d --  coordinate of source line (2D cross section)
%                  rout  --  outer radius of the conductor
%                  f0    --  frequency
%  Output:         Xs    --  [Ns*1] X axis of the center of segments (anulus section)
%                  Ys    --  [Ns*1] Y axis of the center of segments (anulus section)
%                  NVsa  --  [Nc*1] number of segment in each conductor
%  Others:
%  Author:         Chen Hongcai
%  Email :         hc.chen@live.com
%  Date:           2017-03-13
%  History:


% X1 and X2 is the center of the cable
% Nlmax is the max number of segments
% dNl is decrese number of each layer
% cal_t is the calculation thick of the cable

% for nonmagnetic body (mur = 1)
% mu0 = 4*pi*1e-7;
%

Nc = length(pt_2d(:,1));  % the number of cable
asta = zeros(Nc,1); % Angle should be in Radian Form
dAal = 2*pi*ones(Nc,1);


%% %%%%%%%%%%%%%%%%%%%%%%%%%% Segment Scheme %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%NVsa = max(1, floor(2*pi*rout./dl));
NVsa = round(2*pi*rout./dl);


%% %%%%%%%%%%%%%%%% genenrate the axis for the segments %%%%%%%%%%%%%%%%%%%
Ns = sum(NVsa);
% rs = zeros( Ns,1);
a1 = zeros( Ns,1);
a2 = zeros( Ns,1);
Xs = zeros( Ns,1);
Ys = zeros( Ns,1);

cnt = 0;
for ik = 1:Nc
    if NVsa(ik)>0
        % start and end index
        ind = cnt+(1:NVsa(ik));
        
        %    	rs(ind,1) = rout(ik)*ones(1,NVsa(ik));
        a1(ind,1) = dAal(ik)/NVsa(ik)*(0:NVsa(ik)-1)' + asta(ik);
        a2(ind,1) = dAal(ik)/NVsa(ik)*(1:NVsa(ik))' + asta(ik);
        
        cnt = cnt + NVsa(ik);
    end
end

cnt = 0;
for ik = 1 : Nc
    if NVsa(ik)>0
        % start and end index
        ind = cnt+1:cnt+NVsa(ik);
        
        ang0 = (a2(ind,1) + a1(ind,1))/2;
        
        Xs(ind) = rout(ik).*cos(ang0) + pt_2d(ik,1);
        Ys(ind) = rout(ik).*sin(ang0) + pt_2d(ik,2);
        %Xs(ind,3) = ones(NVsa(ik),1) * PoL(ik,3);
        
        cnt = cnt + NVsa(ik);
    end
end


end

