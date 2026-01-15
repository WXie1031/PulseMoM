function [NVr, NMt, NVs, r1, r2] = mesh2d_circular_sub( rout, rin, s_dep, Ntan )
%  Function:       mesh2D_circular_sub
%  Description:    sub function of mesh2D_circular
%  Calls:          
%  Input:          rout --  outer radius of the conductor
%                  rin  --  inner radius of the conductor
%                  s_dep -  skin depth
%                  Ntan --  the number of segments in tangential direction
%                  dNtan -  decrease factor of tangential segments
%  Output:         NVs  --  (vector)the number of segments in each cable
%                  NVr  --  (vector)the number of segments in radius direction 
%                  NMl  --  (matrix)the number of segments along tangent 
%                           direction( row - cable no. col - scheme )
%                  r1   --  the inner radius of each layer
%                  r2   --  the outer radius of each layer
%  Others:         
%  Author:         Chen Hongcai    
%  Email :         hc.chen@live.com      
%  Date:           2013-11-13
%  History:         
%      <author>      <time>       <desc>
%      David         96/10/12     build this moudle  


Nc = size(rout,1);
NVs = zeros(Nc,1);

%% 1. the table of 10% decrease depth according to skin depth
dt_tab = s_dep*[0  0.10536  0.22314  0.35667  0.51083 0.69315  0.91629 ...   
   1.2040  1.6094  2.3026  4.61  6.9078]; 

Ntab = size(dt_tab,2);
NVr = zeros(1,Nc);
NMt = zeros(Nc,Ntab-1);
r1 = zeros(Nc,Ntab-1);
r2 = zeros(Nc,Ntab-1);

t_tab1 = cumsum(dt_tab,2);
t_tab2 = cumsum(dt_tab(:,2:Ntab),2);

cal_t = min(rout-rin, max(t_tab2,[],2));

%% 2. calculate meshing r and N
for k = 1:Nc
    
    for g = 1:Ntab-1
        if cal_t(k) < t_tab2(k,g) 
            break;
        end
    end
    r2(k,1:g) = rout(k)-t_tab1(k,1:g);
    NVr(k) = g;
    
    r1(k,1:NVr(k)) = [r2(k,2:NVr(k)) rout(k)-cal_t(k)];
    
    if NVr(k) >= 3 && r2(k,NVr(k))-r1(k,NVr(k)) < (r2(k,NVr(k)-1)-r1(k,NVr(k)-1))/5
        NVr(k) = NVr(k)-1;
        r1(k,NVr(k)) = rout(k)-cal_t(k);
    end
    
end
        
for k = 1:Nc
    if NVr(k) < 3
        NVr(k) = 3;
        r2(k,1:3) = cal_t(k)/3*(3:-1:1)+rout(k)-cal_t(k);
        r1(k,1:3) = cal_t(k)/3*(2:-1:0)+rout(k)-cal_t(k);
    end
end


%% 3. count the number of segments
for k = 1:Nc
    % old method with dNtan input
%     while Ntan(k) - dNtan(k)*(NVr(k)-1) < 6
%     	dNtan(k) = dNtan(k) - 1;
%     end
%     NMt(k,1:NVr(k)) = Ntan(k) - dNtan(k)*(0:NVr(k)-1);

%   new method to keep the dr is nearly the same of different layers
    NMt(k,1) = Ntan(k);
    NMt(k,2:NVr(k)) = ceil( Ntan(k)*r2(k,2:NVr(k))/r2(k,1) );
    
    NMt(k,1:NVr(k)) = max(NMt(k,1:NVr(k)), 6);
end


for k = 1:Nc
    NVs(k,1) = sum(NMt(k,1:NVr(k)),2);
end


end
