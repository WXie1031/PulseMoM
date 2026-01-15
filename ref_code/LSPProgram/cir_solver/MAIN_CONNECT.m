

function [BNL NdP NdSR NdGD BNspd BNec BNcon BNct Ndvt BN NOD Rec Lec Cec Rcon SPDtype ... 
    ID_SV ID_SI A id_CT id_VT]  = MAIN_CONNECT ...
    (NODsht, ECsht, BNMsht, SPDsht, BNL, NdP, Stype, GGflag)
   

global NbL
global NnP

global Nb
global Nn

%% Initial

Rec=[]; Lec=[]; Cec=[]; BNec=[]; NdEC=[]; 
BNcon=[]; %Acon=[]; Rcon=[];

NbL = size(BNL,1);

NdPtmp = unique(NdP,'rows','stable');
NnP = length(NdPtmp);

BNtmp = BNL(:,2:3);

Ncol = size(NODsht,2);
NODsht = NaNDEL(NODsht,2);

%% %%%%%%%%%%%%%%%%%%%%%%%% Source Nodes %%%%%%%%%%%%%%%%%%%%%%%%%
NdSR = NaNDEL(NODsht(:,1:2),2);
if isempty(NdSR)
    error('Please Add Source Nodes in Sheet3 !');
end

%% %%%%%%%%%%%%%%%%%%%%%%%% Ground Nodes %%%%%%%%%%%%%%%%%%%%%%%%%
if size(NODsht,2) < 3
    NdGD = [];
else
    NdGD = unique(NaNDEL(NODsht(:,3),2)); 
end

%% %%%%%%%%%%%%%%%%%%% The Same Position Connection %%%%%%%%%%%%%%%%%%%%%%%
% NdPtab = unique(NdPtmp(:,2));

BNSPC = [];
% for ik = 1:length(NdPtab)
%     ind = find(NdPtmp(:,2)==NdPtab(ik));
%     Nspc = length(ind);
%     if Nspc > 1
%         BNSPC = [ BNSPC; [NdPtmp(ind(1),1)*ones(Nspc-1,1) NdPtmp(ind(2:end),1)]; ];
%     end
% end
NdSPC = unique(BNSPC);
% if size(NdSPC,2)>1
%     NdSPC = NdSPC';
% end
    
%% %%%%%%%%%%%%%%%%% Electrical Component Nodes %%%%%%%%%%%%%%%%%%
if ~isempty(ECsht)
    
    % R
    indR = ECsht(:,1) == 1;
  	Rec = ECsht(indR,4);
  	BNecR = ECsht(indR,2:3);
         
    % L
    indL = ECsht(:,1) == 2;
  	Lec = ECsht(indL,4)*1e-6;
  	BNecL = ECsht(indL,2:3);          

    % C
    indC = ECsht(:,1) == 3;
  	Cec = ECsht(indC,4)*1e-6;
  	BNecC = ECsht(indC,2:3);

    if sum( indR+indL+indC ) < length(ECsht(:,1))
        error('Wrong Electrical Components Input ! Check Sheet3 !');
    end

    BNec = [BNecR; BNecL; BNecC;];
    NdEC = unique(BNec,'stable');
    if size(NdEC,2) > 1
        NdEC = NdEC';
    end

end

%% %%%%%%%%%%%%%%%%% Merging Nodes is Needed %%%%%%%%%%%%%%%%%%%%%
if Ncol > 3
    TBmeg = NaNDEL(NODsht(:,4:end),2); % Table of Merging Nodes
    Nrmeg = size(TBmeg,1);    % Size of the table    
    for ik = 1:Nrmeg
        
        % Get Ref Node
        Ndrf = TBmeg(ik,1);  % ref. node name                     
        % all nodes for one ref. node;
        Ndori = unique(TBmeg(ik,2:end));           
        Ncmeg = sum(~(isnan(Ndori))); % total # of common nodes Eliminate blank (Colunm)
        BNcon = [BNcon; Ndrf*ones(Ncmeg,1) Ndori(1:Ncmeg)';];
        clear Ndori
    end

end


%% %%%%%%%%%%%%%%%%%%%%%%%% Adding SPD %%%%%%%%%%%%%%%%%%%%%%%%%%%
if ~isempty(SPDsht)
    BNspd = SPDsht(:,2:3);
    SPDtype = SPDsht(:,1);
else
    BNspd = [];
    SPDtype = [];
end

BNL(:,2:3) = BNtmp;

BN = unique([BNL(:,2:3); BNec; BNSPC; BNcon],'rows','stable');

NOD = unique([NdP; NdEC; NdSPC;],'stable');
%ind = ~ismember(NdEC,NdP);
%NOD = [NdP; NdEC(ind);];

if ~GGflag
    ind = ismember(NOD, NdGD);
    for ik = length(ind):-1:1
        if ind(ik) == 1
            NOD(ik) = [];
        end
    end
end

Rcon = zeros(size(BNSPC,1)+size(BNcon,1),1)+1e-8;

Nb = size(BN,1);
Nn = length(NOD);

%% %%%%%%%%%%%%%% Add Source to NdSR %%%%%%%%%%%%%%%%%%%%%%%%%%%%%
[ID_SV ID_SI] = SR_IND( NOD, NdSR, BN, Stype);

%%  Calculate A matrix (connection matrix) %%%%%%%%%%%%%%%%
A = A_MATRIX(BN, NOD, NdGD);

%% %%%%%%%% Index of Measurment Branch (I) and Node (V) %%%%%%%%%%
id_VT = zeros(NnP,1);
id_CT = zeros(NbL,1);

for ik = 1:NbL
    if BNMsht(ik,4) == 1
        ind = BNMsht(ik,1) == BNL(:,1);
        id_CT = id_CT + ind;
    end
end
id_CT = id_CT&id_CT;
ind = id_CT == 1;
BNct = BNL(ind,2:3);


if size(BNMsht,2) > 5
    for ik = 1:length(BNMsht(:,4))
        if BNMsht(ik,6) == 1;
            ind = BNMsht(ik,5) == NdP;
            id_VT = id_VT + ind;
        end
    end
    id_VT = id_VT&id_VT;
else
    id_VT = zeros(NnP,1);
end
ind = id_VT == 1;
Ndvt = NdP(ind);


end

