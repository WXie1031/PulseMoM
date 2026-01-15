function [Xs,Ys, rs, a1,a2, NVsa] = mesh2d_P_cir(pt_2d, rout)
%  Function:       mesh2d_P_cir
%  Description:    Mesh circular cross sections (2D) on the surface for
%                  Potential cells.         
%  Calls:          
%  Input:          pt_2d  --  coordinate of source line (2D cross section)
%                  rout   --  outer radius of the conductor
%                  f0     --  frequency
%  Output:         Xs   --  [Ns*1] X axis of the center of segments (anulus section)
%                  Ys   --  [Ns*1] Y axis of the center of segments (anulus section)
%                  rs   --  [Nc*1] outer radius of the segments (anulus section)
%                  as1  --  [Ns*1] start angle of the segments (anulus section)
%                  as2  --  [Ns*1] end angle of the segments (anulus section)
%                  NVsa --  [Nc*1] number of segment in each conductor
%  Others:         
%  Author:         Chen Hongcai    
%  Email :         hc.chen@live.com      
%  Date:           2018-06-.3
%  History: 


% X1 and X2 is the center of the cable
% Nlmax is the max number of segments 
% dNl is decrese number of each layer
% cal_t is the calculation thick of the cable

% for nonmagnetic body (mur = 1)
mu0 = 4*pi*1e-7;
Nc = size(pt_2d,1);  % the number of cable
f0 = 1e6;
sig = 5.8e7;  % conductivity


%% 1. Determine the no. of segments in the tangential direction.
asta = zeros(Nc,1); % Angle should be in Radian Form
dAal = 2*pi*ones(Nc,1);

s_dep = 1./sqrt(pi*f0.*sig.*mu0);

X = s_dep./rout;
a =       3.519;
b =     -0.7034;
c =       4.345;
NVsa = max(ceil( (a*X.^b+c) .* dAal./(2*pi) ), 24);


%% 2. genenrate the axis for the segments 
Ns = sum(NVsa);
rs = zeros( Ns,1);
a1 = zeros( Ns,1);
a2 = zeros( Ns,1);
Xs = zeros( Ns,1);
Ys = zeros( Ns,1);

cnt = 0;
for ik = 1:Nc
    % start and end index
  	ind = cnt+(1:NVsa(ik));
        
   	rs(ind,1) = rout(ik)*ones(1,NVsa(ik));
    a1(ind,1) = dAal(ik)/NVsa(ik)*(0:NVsa(ik)-1)' + asta(ik);
   	a2(ind,1) = dAal(ik)/NVsa(ik)*(1:NVsa(ik))' + asta(ik);
        
  	cnt = cnt + NVsa(ik);
end

cnt = 0;
for ik = 1 : Nc

    % start and end index
    ind = cnt+1:cnt+NVsa(ik);
    
    ang0 = (a2(ind,1) + a1(ind,1))/2; 
        
    Xs(ind) = rout(ik).*cos(ang0) + pt_2d(ik,1);
    Ys(ind) = rout(ik).*sin(ang0) + pt_2d(ik,2);
    %Xs(ind,3) = ones(NVsa(ik),1) * PoL(ik,3); 
    
    cnt = cnt + NVsa(ik);
end


end
