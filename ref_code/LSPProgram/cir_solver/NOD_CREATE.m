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

function NOD = NOD_CREATE(Po, Pold, Nold)

err = 1e-7;  % 1e-3 mm is the tolerance
err2 = err*err;

Nc = size(Po,1);
NOD = zeros(Nc,2);
len=1:Nc;

PoN1=Po(len,1:3);
PoN2=Po(len,4:6);
    
%% check if the branch have the same start and end nodes
DIF = sum( (PoN1-PoN2).^2,2 );

%dif = (PoN1(:,1)-PoN2(:,1)).^2 + (PoN1(:,2)-PoN2(:,2)).^2 + (PoN1(:,3)-PoN2(:,3)).^2;

ind = find( sum(DIF,2) < err2);
if sum(ind) > 0
    error('The Branchhas the Same Start and End Nodes! Check the Input');
end

%% generate the nodes
if isempty(Pold)
    Nnod = 2;       % node for 1st segment
    PoldAll = [ Po(1,1:3); Po(1,4:6); ];
    NoldAll = [ 1; 2 ]; 
else    
    Nnod = max(max(Nold));
    PoldAll = [ Pold(:,1:3); Pold(:,4:6); ];
    NoldAll = [ Nold(:,1); Nold(:,2); ];
end

for k = 1:Nc

	ind1 = []; ind2 = [];
    dif1 = []; dif2 = [];
    
   	% start point
   	dif1(:,1) =  PoN1(k,1) - PoldAll(:,1);
   	dif1(:,2) =  PoN1(k,2) - PoldAll(:,2);
   	dif1(:,3) =  PoN1(k,3) - PoldAll(:,3);
  	DP1 = sum(dif1.*dif1,2);
        
   	ind1 = find(DP1 < err2);
   	if isempty(ind1)
       	ind3 = []; ind4 = [];
      	dp1 = []; dp2=[];
       	dp1(:,1) = PoN1(1:k-1,1) - PoN1(k,1);
      	dp1(:,2) = PoN1(1:k-1,2) - PoN1(k,2);    
       	dp1(:,3) = PoN1(1:k-1,3) - PoN1(k,3); 
       	dps1 = sum(dp1.*dp1,2);         % find the difference among pts
            
       	dp2(:,1) = PoN2(1:k-1,1) - PoN1(k,1);
       	dp2(:,2) = PoN2(1:k-1,2) - PoN1(k,2);    
       	dp2(:,3) = PoN2(1:k-1,3) - PoN1(k,3); 
       	dps2 = sum(dp2.*dp2,2);         % find the difference among pts
    
       	ind3 = find(dps1 < err2);
       	ind4 = find(dps2 < err2);
       	if ~isempty(ind3)
          	NOD(k,1) = NOD(ind3(1),1);
       	elseif ~isempty(ind4)
          	NOD(k,1) = NOD(ind4(1),2);
        else
          	Nnod=Nnod+1;
           	NOD(k,1)=Nnod;
        end
    else
      	NOD(k,1) = NoldAll(ind1(1));
    end
        
  	% end point
  	dif2(:,1) =  PoN2(k,1) - PoldAll(:,1);
   	dif2(:,2) =  PoN2(k,2) - PoldAll(:,2);
   	dif2(:,3) =  PoN2(k,3) - PoldAll(:,3);
   	DP2 = sum(dif2.*dif2,2);
        
   	ind2 = find(DP2 < err2);
  	if isempty(ind2) % no common pt and new pt is assigned
      	ind3 = []; ind4 = [];
       	dp1 = []; dp2=[];
      	dp1(:,1) = PoN1(1:k-1,1) - PoN2(k,1);
      	dp1(:,2) = PoN1(1:k-1,2) - PoN2(k,2);    
       	dp1(:,3) = PoN1(1:k-1,3) - PoN2(k,3); 
       	dps1 = sum(dp1.*dp1,2);         % find the difference among pts
            
      	dp2(:,1) = PoN2(1:k-1,1) - PoN2(k,1);
       	dp2(:,2) = PoN2(1:k-1,2) - PoN2(k,2);    
       	dp2(:,3) = PoN2(1:k-1,3) - PoN2(k,3); 
      	dps2 = sum(dp2.*dp2,2);         % find the difference among pts
    
       	ind3 = find(dps1 < err2);
       	ind4 = find(dps2 < err2);
       	if ~isempty(ind3)
           	NOD(k,2) = NOD(ind3(1),1);
       	elseif ~isempty(ind4)
          	NOD(k,2) = NOD(ind4(1),2);
        else
         	Nnod=Nnod+1;
           	NOD(k,2)=Nnod;
        end

   	else % have commom pt
      	NOD(k,2) = NoldAll(ind2(1));
    end
        
   	PoldAll = [ PoldAll; Po(k,1:3); Po(k,4:6); ];
   	NoldAll = [ NoldAll; NOD(k,1); NOD(k,2);  ];
    
end


end

    
    
    
    