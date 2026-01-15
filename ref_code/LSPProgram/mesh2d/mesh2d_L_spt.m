function [Xs,Ys, rs1,rs2, as1,as2, dS, Rs, NVst] = mesh2d_L_spt(pt_2d, ...
    rout, rin, Rp, p2Doth, f0)
%  Function:       mesh2d_L_spt
%  Description:    Mesh circular cross sections (2D). The centre is diged out            
%  Calls:          
%  Input:          pt_2d --  coordinate of source line (2D cross section)
%                  rout  --  outer radius of the conductor
%                  rin   --  inner radius of the conductor
%                  Rp    --  resistivity of the conductor (ohm/m)
%  Output:         Xs    --  [Ns*1] X axis of the center of segments (anulus section)
%                  Ys    --  [Ns*1] Y axis of the center of segments (anulus section)
%                  rs1   --  [Nc*1] outer radius of the segments (anulus section)
%                  rs2   --  [Nc*1] inner radius of the segments (anulus section)
%                  as1   --  [Ns*1] start angle of the segments (anulus section)
%                  as2   --  [Ns*1] end angle of the segments (anulus section)
%                  ls    --  [Nc*1] length of the segments (anulus section)
%                  Rs    --  [Ns*1] resistance of the segments (anulus section) (ohm)
%  Others:         
%  Author:         Chen Hongcai    
%  Email :         hc.chen@live.com      
%  Date:           2018-03-19
%  History:         


mu0 = 4*pi*1e-7;
Nc = size(pt_2d,1);  % the number of cable
f0 = min(f0, 1e6);
cond = 5.8e7;  % conductivity

% dl = 0.1;
% % NVst = ceil(2*pi*rout./dl);

S = pi .* (rout.^2-rin.^2);           % area of cross section
s_dep = 1./sqrt(pi*f0*cond.*mu0) * ones(Nc,1);     % skin depth

% X = 10*s_dep./rout;
X = s_dep./rout;
a = 3.519;
b = -0.7034;
c = 4.345;
NVst = min(max(ceil( a*X.^b+c ), 64), 4000);


%% 3. add exact coordinate and mesh all circular conductors
Ns = sum(NVst);
rs1 = zeros(Ns,1);
rs2 = zeros(Ns,1);
as1 = zeros(Ns,1);
as2 = zeros(Ns,1);
Xs = zeros(Ns,1);
Ys = zeros(Ns,1);
dS = zeros(Ns,1);
Rs = zeros(Ns,1);


cnt = 0;
for ik = 1:Nc
    
    angv = 2*pi/NVst(ik)*(1:NVst(ik));
    
    % start and end index
    ind = cnt+(1:NVst(ik));
    
    rs1(ind,1) = rout(ik)*ones(1,NVst(ik));
    rs2(ind,1) = rin(ik)*ones(1,NVst(ik));
    as1(ind,1) = [0 (angv(1:NVst(ik)-1))];
    as2(ind,1) = (angv);
    
    cnt = cnt + NVst(ik);
end

cnt = 0;
for ik = 1 : Nc
    % start and end index
    ind = cnt+(1:NVst(ik));
    
    % caculate the R of the segments
    dS(ind,1) = (as2(ind,1)-as1(ind,1)).*(rs2(ind,1).^2-rs1(ind,1).^2)/2;
    Rs(ind,1) = Rp(ik,1).*S(ik,1)./dS(ind,1);

    X_a = (as2(ind,1)+as1(ind,1))/2; 
    X_r = (rs2(ind,1)+rs1(ind,1)) /2;

    Xs(ind,1) = X_r.*cos(X_a) + pt_2d(ik,1);
    Ys(ind,1) = X_r.*sin(X_a) + pt_2d(ik,2);

    cnt = cnt + NVst(ik);
end

% figure(21)
% plot(Xs, Ys, '*');

end

