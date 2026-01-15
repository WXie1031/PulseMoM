function [Xs, Ys, rs1, rs2, as1, as2, dS, Rs, NVsa] = mesh2d_L_cir...
    (pt_2d, rout, rin, Rp, mur, f0)
%  Function:       mesh2d_L_cir
%  Description:    Mesh circular cross sections (2D). The centre is diged out            
%  Calls:          mesh2d_L_cir_sub
%  Input:          pt_2d --  coordinate of source line (2D cross section)
%                  rout --  outer radius of the conductor
%                  rin  --  inner radius of the conductor
%                  Rp   --  resistivity of the conductor (ohm/m)
%                  f0   --  frequency
%  Output:         Xs   --  [Ns*1] X axis of the center of segments (anulus section)
%                  Ys   --  [Ns*1] Y axis of the center of segments (anulus section)
%                  rs1  --  [Nc*1] outer radius of the segments (anulus section)
%                  rs2  --  [Nc*1] inner radius of the segments (anulus section)
%                  as1  --  [Ns*1] start angle of the segments (anulus section)
%                  as2  --  [Ns*1] end angle of the segments (anulus section)
%                  ls   --  [Nc*1] length of the segments (anulus section)
%                  Rs   --  [Ns*1] resistance of the segments (anulus section) (ohm)
%                  NVsa --  [Nc*1] number of segment in each conductor
%  Others:         
%  Author:         Chen Hongcai    
%  Email :         hc.chen@live.com      
%  Date:           2013-11-13
%  History:         


% for nonmagnetic body (mur = 1)
mu0 = 4*pi*1e-7;

Nc = size(pt_2d,1);  % the number of cable

if max(rout) > 2e-3
    f0 = min(f0, 1e6);
else
    f0 = min(f0, 10e6);
end
cond = 5.8e7;  % conductivity

S = pi .* (rout.^2-rin.^2);           % area of cross section
s_dep = 1./sqrt(pi*f0*cond.*mu0*max(mur)) * ones(Nc,1);     % skin depth

%% 1. empirical formula of mesh number in tangential direction
X = s_dep./rout;
a = 3.519;
b = -0.7034;
c = 4.345;
Ntan = max(ceil( a*X.^b+c ), 20);

% id1 = Ntan>=32;
% id2 = Ntan>24 & Ntan<32;
% dNtan = zeros(1,Nc);
% dNtan(id1) = 4;
% dNtan(id2) = 2;
% [rout(1) Ntan(1)]

%% 2. mesh cross section number of each ( the centre is (0,0) )
[NVr, NMt, NVsa, r1, r2] = mesh2d_L_cir_sub( rout, rin, s_dep, Ntan );


%% 3. add exact coordinate and mesh all circular conductors
Ns = sum(NVsa);
rs1 = zeros(Ns,1);
rs2 = zeros(Ns,1);
as1 = zeros(Ns,1);
as2 = zeros(Ns,1);
Xs = zeros(Ns,1);
Ys = zeros(Ns,1);
dS = zeros(Ns,1);
Rs = zeros(Ns,1);

cnt = 0;
for k = 1:Nc
    for g = 1:NVr(k)
        % start and end index
        ind1 = cnt+1;
        ind2 = cnt+NMt(k,g);
        
        rs1( ind1:ind2,1) = r1(k,g)*ones(1,NMt(k,g));
        rs2( ind1:ind2,1) = r2(k,g)*ones(1,NMt(k,g));
        as1(ind1:ind2,1) = 2*pi/NMt(k,g)*(0:NMt(k,g)-1)';
        as2(ind1:ind2,1) = 2*pi/NMt(k,g)*(1:NMt(k,g))';
        
        cnt = cnt + NMt(k,g);
    end
end

cnt = 0;
for k = 1 : Nc

    % start and end index
    %ind = cnt+1:cnt+NVsa(k);
    
    % caculate the R of the segments
    dS(cnt+1:cnt+NVsa(k),1) = (as2(cnt+1:cnt+NVsa(k),1)-as1(cnt+1:cnt+NVsa(k),1))...
        .*(rs2(cnt+1:cnt+NVsa(k),1).^2-rs1(cnt+1:cnt+NVsa(k),1).^2)/2;
    %Rs(ind,1) = Rcal(k)*S(k)*ones(num_s(k),1)./dS ;
    Rs(cnt+1:cnt+NVsa(k),1) = Rp(k,1).*S(k,1)./dS(cnt+1:cnt+NVsa(k),1);

    X_a = (as2(cnt+1:cnt+NVsa(k),1)+as1(cnt+1:cnt+NVsa(k),1))/2; 
    X_r = (rs2(cnt+1:cnt+NVsa(k),1)+rs1(cnt+1:cnt+NVsa(k),1)) /2;
        
    Xs(cnt+1:cnt+NVsa(k),1) = X_r.*cos(X_a) + pt_2d(k,1);
    Ys(cnt+1:cnt+NVsa(k),1) = X_r.*sin(X_a) + pt_2d(k,2);
    
    cnt = cnt + NVsa(k,1);

end


end
