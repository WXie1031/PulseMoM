%% calculate the current in the Time domain
% So      Source
function [Ibt Vnt] = MAIN_SOLVER_T(Rt, Lt, Pt, Lg, Pg, Rec, Lec, Cec, Rcon, ...
    TrLT, NmTrL, TrPT, NmTrP, TrLg, NmTgL, TrPg, NmTgP, ...
    NdP, NdGD, ID_SV, ID_SI, A, SRt, t, dt, Pflag, Trflag, Gtype)
    
   
global NnP % No. of Nodes for Conductors

%% Adding Ground Effect and Retarded Time
if Trflag == 0
    if Gtype > 0
     	Lt = Lt-Lg;
      	Pt = Pt-Pg;
    end
end

%%  %%%%%%%%%%%%%%%%%%% Ground Node Remove P %%%%%%%%%%%%%%%%%%%%%
if Pflag > 0
    ind = ismember(NdP, NdGD);
   	for ik = length(ind):-1:1
      	if ind(ik) == 1
            NdP(ik) = [];
        end
    end
   	NnP = length(NdP);
    
    ind = ~ind;
   	Pc = Pt(ind,ind);
        
   	if Trflag > 0 && Gtype > 0 
        Pgc = Pg(ind,ind);
        TrPTc = TrPT(ind,ind);
        TrPgc = TrPg(ind,ind);

    elseif Trflag > 0 && Gtype == 0   
        TrPTc = TrPT(ind,ind);
        
        Cs = diag(1./diag(Pc));
        Ss = zeros(NnP,NnP);
        for ik = 1:NnP
            Ss(:,ik) = Pc(:,ik)./Pc(ik,ik);
        end
        
    elseif Trflag == 0
        
        Cs = diag(1./diag(Pc));
        Ss = zeros(NnP,NnP);
        for ik = 1:NnP
            Ss(:,ik) = Pc(:,ik)./Pc(ik,ik);
        end
    end
    
end


tSta = tic;
%%  Network Solving using MNA Method %%%%%%%%%%%%%%%%%%%%
Nt = length(t);

if Trflag > 0 && Gtype > 0
    % R, L, P and Ground Effect (retarded time) 
  	[Ibt Vnt] = MNA_TdGTsolver( Rt, Lt, Pc, Rec, Lec, Cec, Rcon, Lg, Pgc, ...
     	TrLT, NmTrL, TrPTc, NmTrP, TrLg, NmTgL, TrPgc, NmTgP, ...
        A, ID_SV, ID_SI, SRt, dt, Nt );
elseif Trflag > 0 && Gtype == 0
    % R, L and P (retarded time) 
 	[Ibt Vnt] = MNA_TdTsolver( Rt, Lt, Cs, Ss, Rec, Lec, Cec, Rcon, ...
     	TrLT, NmTrL, TrPTc, NmTrP, A, ID_SV, ID_SI, SRt, dt, Nt );
elseif Trflag == 0
    % R, L and P solver    
  	[Ibt Vnt] = MNA_Tsolver( Rt, Lt, Cs, Ss, Rec, Lec, Cec, Rcon, ...
     	A, ID_SV, ID_SI, SRt, dt, Nt );
end


tSto = toc(tSta);
str = sprintf('Time for Time Domain Network Solving is %f s', tSto);
disp(str);



end




