% MNA_SOLVER   solve the matrix of the circuit using MNA method
% %%%%%% INPUT %%%%%%%%
% f0            specific frequency
% R             resistance matrix 
% L             inductance matrix (retarding time is calculated within the L)
% A             connection matrix of the circuit network
% Vs            voltage source adding to the network
% Is            current source adding to the network
% %%%%% OUTPUT %%%%%%%%
% Vo            output of node voltage
% Io            output of branch current
% %%%%%%%%%%%%%%%%%%%%%%%%%%%% revised by Chen Hongcai 2014.3
function [Ibt, Vnt] = MNA_TdGTsolver( Rs, Ls, Ps, Rec, Lec, Cec, Rcon, Lg, Pg, ...
 	ID_TrL, NmTdL, ID_TrP, NmTdP, ID_TgL, NmTgL, ID_TgP, NmTgP, ...
    A, ID_SV, ID_SI, SR, dt, Nt )

global NbL % No. of Branches of Wires
global NnP % No. of Nodes of Wires
global Nb  % No. of Branches of All Components
global Nn  % No. of Nodes of All Components

NtdL = max( max(max(NmTdL)), max(max(NmTgL)) );
NtdP = max( max(max(NmTdP)), max(max(NmTgP)) );

Ins = zeros(Nn,1);
Ibt = zeros(Nb,Nt+NtdL);
Int = zeros(Nn,Nt+NtdP);
Vnt = zeros(Nn,Nt+NtdP);

is = [ zeros(1,NtdP) SR ];

dLt = Ls./dt;
dLg = Lg./dt;

%% Separate the Retarded Terms and No Retarded Terms into 2 Groups
% Separate L
dLnr = zeros(NbL,NbL);
dLr = zeros(NbL,NbL);
ID_Lnr = ID_TrL == 0;
ID_Lr = ~ID_Lnr;
dLnr(ID_Lnr) = dLt(ID_Lnr);
dLr(ID_Lr) = dLt(ID_Lr);

% Separate C
Pnr = zeros(NnP,NnP);
Pr = zeros(NnP,NnP);
ID_Cnr = ID_TrP == 0;
ID_Cr = ~ID_Cnr;
Pnr(ID_Cnr) = Ps(ID_Cnr);
Pr(ID_Cr) = Ps(ID_Cr);

% Adding Electrical Components into Matrix
Zleft = blkdiag( (Rs+dLnr), diag(Rec), diag(Lec./dt), diag(Rcon) );
Zrigth = blkdiag( dLnr, zeros(length(Rec)), diag(Lec./dt), diag(Rcon) );
Cleft = blkdiag( eye(NnP)./dt, diag(Cec./dt) );
Pleft = blkdiag( Pnr, zeros(length(Cec)) );

%% %%%%%%% Backward Euler Method for Solving the Network %%%%%%%%%
% Not Change in Time Domain
LEFT = [ -A'  -Zleft ; Cleft  -Pleft*A ; ] ;


ibt0 = zeros(NbL,1);
ibt1 = zeros(NbL,1);
int0 = zeros(NnP,1);

for k = 2:Nt
    
    vs = ID_SV.*SR(k);

    RIGHT1 = [ vs - Zrigth*Ibt(:,NtdL+k-1); ...
        Cleft*Vnt(:,NtdP+k-1) ];
    
    dLr_ib = zeros(NbL,1);
    for g = 1:NbL
        ind = NtdL + k - ID_TrL(g,:);
        for h = 1:NbL
            ibt0(h) = Ibt(h,ind(h));
            ibt1(h) = Ibt(h,ind(h)-1);
        end
        dLr_ib(g) = dLr(g,:)*(ibt0-ibt1);  
    end
    
    Pr_ib = zeros(NnP,1);
    for g = 1:NnP
        ind = NtdP + k - ID_TrP(:,g);
        for h = 1:NnP
            if ID_SI(h) == 1 || ID_SI(h) == -1
                Ins(h) = ID_SI(h).*is(ind(h)) ;
            end
            int0(h) = Int(h,ind(h));
        end
        Pr_ib(g) = Ps(g,:)*Ins + Pr(g,:)*int0;
    end
    
    dLg_ib = zeros(NbL,1);
    for g = 1:NbL
        ind = NtdL + k - ID_TgL(g,:);
        for h = 1:NbL
            ibt0(h) = Ibt(h,ind(h));
            ibt1(h) = Ibt(h,ind(h)-1);
        end
        dLg_ib(g) = dLg(g,:)*(ibt0-ibt1);  
    end
    
    Pg_ib = zeros(NnP,1);
    for g = 1:NnP
        ind = NtdP + k - ID_TgP(:,g);
        for h = 1:NnP
            if ID_SI(h) == 1 || ID_SI(h) == -1
                Ins(h) = ID_SI(h).*is(ind(h)) ;
            end
            int0(h) = Int(h,ind(h));
        end
        %Pg_ib(g) = Pg(g,:)*Ins + Pg(g,:)*int0;
        Pg_ib(g) = Pg(g,:)*(Ins+int0);
    end
    
    RIGHT2 = [ dLr_ib-dLg_ib; zeros(length(Rec),1); zeros(length(Lec),1); zeros(length(Rcon),1); ...
        Pr_ib-Pg_ib; zeros(length(Cec),1); ];
    
    out = LEFT\(RIGHT1+RIGHT2);
    
    Ibt(1:Nb,NtdL+k) = out(Nn+1:end);
    Int(1:Nn,NtdP+k) = A*Ibt(1:Nb,NtdL+k);
    Vnt(1:Nn,NtdP+k) = out(1:Nn);
end

Ibt = Ibt(1:Nb,NtdL+1:NtdL+Nt);
Vnt = Vnt(1:Nn,NtdP+1:NtdP+Nt);


end


