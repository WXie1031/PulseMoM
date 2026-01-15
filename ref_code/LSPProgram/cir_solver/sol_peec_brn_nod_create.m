% NOD_CREATE   generate the nodes of each conductor
% %%%%%% INPUT %%%%%%%%
% Po            [N*6] axis of the two nodes of conductors
%               node1 (:,1:3)  node2 (:,4:6)
% Po_old        previous nodes (excluding the Po)
% NODoff        NOD offset of previous cells of conductors
% %%%%% OUTPUT %%%%%%%%
% NOD           [N*2] the start and the end nodes of the conductors
% Mnod          (for updating NOD offset) maxium no. of the nodes
% %%%%%%%%%%%%%%%%%%%%%%%%%%%% revised by Chen Hongcai 2013.11

function [nod_start, nod_end, nod_P] = sol_peec_brn_nod_create( ...
    p_sta_L,p_end_L, pt_P, p_sta_old,p_end_old, nod_sta_old,nod_end_old, p_flag)

err = 1e-6;  % 1e-6 mm is the tolerance
err2 = err*err;

Nb = size(p_sta_L,1);
NnP = size(pt_P,1);
if nargin > 3
    Nb_old = size(p_sta_old,1);
else
    Nb_old = 0;
end


nod_start = zeros(Nb_old+Nb,1);
nod_end   = zeros(Nb_old+Nb,1); 
nod_P     = zeros(NnP,1);

%% check if the branch have the same start and end nodes
dis = sum( (p_sta_L-p_end_L).^2,2 );

ind = find( sum(dis,2) < err2);
if sum(ind) > 0
    error('The Branchhas the Same Start and End Nodes! Check the Input');
end

pt_old  = [p_sta_old; p_end_old; zeros(Nb*2,3);];
nod_old = [nod_sta_old; nod_end_old; zeros(Nb*2,1);];

%% 1. generate the nodes for L cell
if isempty(pt_old)
    Nnod = 2;       % node for 1st segment
    pt_old(1:2,:) = [p_sta_L(1,:);  p_end_L(1,:);];
    nod_old = [1; 2;];
else    
    Nnod = max(max(nod_old));
end

for k = 1:Nb

	ind1 = []; ind2 = [];
    dif1 = zeros(2*(Nb_old+k-1),3); 
    dif2 = zeros(2*(Nb_old+k-1),3); 
    
    ind_old = 1:2*(Nb_old+k-1);
    
   	% start point
   	dif1(:,1) =  p_sta_L(k,1) - pt_old(ind_old,1);
   	dif1(:,2) =  p_sta_L(k,2) - pt_old(ind_old,2);
   	dif1(:,3) =  p_sta_L(k,3) - pt_old(ind_old,3);
  	DP1 = sum(dif1.*dif1,2);
        
   	ind1 = find(DP1 < err2);
   	if isempty(ind1)
       	ind3 = []; ind4 = [];
      	dp1 = []; dp2=[];
       	dp1(:,1) = p_sta_L(1:k-1,1) - p_sta_L(k,1);
      	dp1(:,2) = p_sta_L(1:k-1,2) - p_sta_L(k,2);    
       	dp1(:,3) = p_sta_L(1:k-1,3) - p_sta_L(k,3); 
       	dps1 = sum(dp1.*dp1,2);         % find the difference among pts
            
       	dp2(:,1) = p_end_L(1:k-1,1) - p_sta_L(k,1);
       	dp2(:,2) = p_end_L(1:k-1,2) - p_sta_L(k,2);    
       	dp2(:,3) = p_end_L(1:k-1,3) - p_sta_L(k,3); 
       	dps2 = sum(dp2.*dp2,2);         % find the difference among pts
    
       	ind3 = find(dps1 < err2);
       	ind4 = find(dps2 < err2);
       	if ~isempty(ind3)
          	nod_start(k) = nod_start(ind3(1));
       	elseif ~isempty(ind4)
          	nod_start(k) = nod_end(ind4(1));
        else
          	Nnod = Nnod+1;
           	nod_start(k) = Nnod;
        end
    else
      	nod_start(k,1) = nod_old(ind1(1));
    end
        
  	% end point
  	dif2(:,1) =  p_end_L(k,1) - pt_old(ind_old,1);
   	dif2(:,2) =  p_end_L(k,2) - pt_old(ind_old,2);
   	dif2(:,3) =  p_end_L(k,3) - pt_old(ind_old,3);
   	DP2 = sum(dif2.*dif2,2);
        
   	ind2 = find(DP2 < err2);
  	if isempty(ind2) % no common pt and new pt is assigned
      	ind3 = []; ind4 = [];
       	dp1 = []; dp2=[];
      	dp1(:,1) = p_sta_L(1:k-1,1) - p_end_L(k,1);
      	dp1(:,2) = p_sta_L(1:k-1,2) - p_end_L(k,2);    
       	dp1(:,3) = p_sta_L(1:k-1,3) - p_end_L(k,3); 
       	dps1 = sum(dp1.*dp1,2);         % find the difference among pts
            
      	dp2(:,1) = p_end_L(1:k-1,1) - p_end_L(k,1);
       	dp2(:,2) = p_end_L(1:k-1,2) - p_end_L(k,2);    
       	dp2(:,3) = p_end_L(1:k-1,3) - p_end_L(k,3); 
      	dps2 = sum(dp2.*dp2,2);         % find the difference among pts
    
       	ind3 = find(dps1 < err2);
       	ind4 = find(dps2 < err2);
       	if ~isempty(ind3)
           	nod_end(k) = nod_start(ind3(1));
       	elseif ~isempty(ind4)
          	nod_end(k) = nod_end(ind4(1));
        else
         	Nnod = Nnod+1;
           	nod_end(k) = Nnod;
        end

   	else % have commom pt
      	nod_end(k) = nod_old(ind2(1));
    end
    
    pt_old(2*(Nb_old+k-1)+(1:2),:) = [p_sta_L(k,:); p_end_L(k,:);];
   	nod_old(2*(Nb_old+k-1)+(1:2),:) = [nod_start(k); nod_end(k); ];
    
end

%% 2. generate the nodes for P cell
if p_flag > 0
    
    for k = 1:NnP
        dif1 = zeros(2*(Nb_old+Nb),3);
        dif1(:,1) =  pt_P(k,1) - pt_old(:,1);
        dif1(:,2) =  pt_P(k,2) - pt_old(:,2);
        dif1(:,3) =  pt_P(k,3) - pt_old(:,3);
        DP1 = sum(dif1.*dif1,2);
        
        ind1 = find(DP1 < err2);
        if ~isempty(ind1)
            nod_P(k) = nod_old(ind1(1));
        else
            Nnod = Nnod+1;
            nod_P(k) = Nnod;
        end
    end
end

    
end    
    
    