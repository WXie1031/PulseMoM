function [Xs,Ys, rs, as1,as2, NVst] = mesh2d_P_spt(pt_2d, rout, p2Doth)
%  Function:       mesh2d_P_spt
%  Description:    Mesh circular cross sections (2D) on the surface for 
%                  Potential cells.         
%  Calls:          
%  Input:          pt_2d --  coordinate of source line (2D cross section)
%                  rout  --  outer radius of the conductor
%  Output:         Xs    --  [Ns*1] X axis of the center of segments (anulus section)
%                  Ys    --  [Ns*1] Y axis of the center of segments (anulus section)
%                  rs    --  [Nc*1] outer radius of the segments (anulus section)
%                  as1   --  [Ns*1] start angle of the segments (anulus section)
%                  as2   --  [Ns*1] end angle of the segments (anulus section)
%                  NVsa  --  [Nc*1] number of segment in each conductor
%  Others:         
%  Author:         Chen Hongcai    
%  Email :         hc.chen@live.com      
%  Date:           2018-06-13
%  History:         


mu0 = 4*pi*1e-7;
Nc = size(pt_2d,1);  % the number of cable
f0 = 200e3;
sig = 5.8e7;  % conductivity


%% 1. Determine the no. of segments in the tangential direction.
s_dep = 1./sqrt(pi*f0*sig.*mu0) * ones(Nc,1);     % skin depth

X = s_dep./rout;
a = 3.519;
b = -0.7034;
c = 4.345;
NVst = min(max(ceil( a*X.^b+c ), 40), 4000);


%% 2. genenrate the axis for the segments 
Ns = sum(NVst);
rs = zeros(Ns,1);
as1 = zeros(Ns,1);
as2 = zeros(Ns,1);
Xs = zeros(Ns,1);
Ys = zeros(Ns,1);


cnt = 0;
for ik = 1:Nc
    
    angv = 2*pi/NVst(ik)*(1:NVst(ik));
    % start and end index
    ind = cnt+(1:NVst(ik));
    
    rs(ind,1) = rout(ik)*ones(1,NVst(ik));
    as1(ind,1) = [0 (angv(1:NVst(ik)-1))];
    as2(ind,1) = (angv);
    
    cnt = cnt + NVst(ik);
end

cnt = 0;
for ik = 1 : Nc
    % start and end index
    ind = cnt+(1:NVst(ik));
    
    X_a = (as2(ind,1)+as1(ind,1))/2; 
    X_r = rs(ind,1);

    Xs(ind,1) = X_r.*cos(X_a) + pt_2d(ik,1);
    Ys(ind,1) = X_r.*sin(X_a) + pt_2d(ik,2);

    cnt = cnt + NVst(ik);
end

% figure(21)
% plot(Xs, Ys, '*');

end

