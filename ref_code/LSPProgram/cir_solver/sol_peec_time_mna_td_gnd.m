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
function [Ibt, Vnt] = sol_peec_time_mna_td_gnd( Rpeec,Lpeec,Ppeec, ...
    Rec,Lec,Cec, Lg,Pg, id_TdL,id_TdP, id_TdGL,id_TdGP, ...
    Amtx, id_sv, id_si, sr_t, dt, Nt, p_flag )

sr_max = max(abs(sr_t));
[Nn, Nb] = size(Amtx);  % No. of Nodes and Branches of All Components

Nb_peec = size(Rpeec,1);  % No. of Branches of PEEC network
Nn_peec = size(Ppeec,1);  % No. of Nodes of PEEC network
% Nb_ec_r = size(Rec,1);

% NtdL = max( max(max(NmTdL)), max(max(NmTgL)) );
% NtdP = max( max(max(NmTdP)), max(max(NmTgP)) );

NtdL = max( max(max(id_TdL)), max(max(id_TdGL)) );
if isempty(Ppeec)
    NtdP = 0;
else
    NtdP = max( max(max(id_TdP)), max(max(id_TdGP)) );
end

% Ins = zeros(Nn,1);
Ins = zeros(Nn_peec,1);
Ibt = zeros(Nb,Nt+NtdL);
Int = zeros(Nn,Nt+NtdP);
Vnt = zeros(Nn,Nt+NtdP);

ist = [ zeros(1,NtdP),  sr_t ];

dLt = Lpeec./dt;
dLg = Lg./dt;


%% Separate the Retarded Terms and No Retarded Terms into 2 Groups
% Separate L
dLnr = zeros(Nb_peec,Nb_peec);
dLr = zeros(Nb_peec,Nb_peec);
id_Lnr = id_TdL == 0;
id_Lr = ~id_Lnr;
dLnr(id_Lnr) = dLt(id_Lnr);
dLr(id_Lr) = dLt(id_Lr);

% Separate P
if ~isempty(Ppeec)
    Pnr = zeros(Nn_peec,Nn_peec);
    Pr = zeros(Nn_peec,Nn_peec);
    id_Cnr = id_TdP == 0;
    id_Cr = ~id_Cnr;
    Pnr(id_Cnr) = Ppeec(id_Cnr);
    Pr(id_Cr) = Ppeec(id_Cr);
else
    Pnr = [];
    Pr = [];
end

% Adding Electrical Components into Matrix
dLec = Lec./dt;
dCec = Cec./dt;
Zleft = blkdiag( (Rpeec+dLnr), diag(Rec), diag(dLec) );
Zright = blkdiag( dLnr, zeros(length(Rec)), diag(dLec) );
Cleft = blkdiag( eye(Nn_peec)./dt, diag(dCec) );
Pleft = blkdiag( Pnr, zeros(length(Cec)) );


%% %%%%%%% Backward Euler Method for Solving the Network %%%%%%%%%
% Not Change in Time Domain
LEFT = [ -Amtx'  -Zleft ; Cleft  -Pleft*Amtx ; ] ;
LEFT_INV = inv(LEFT);
% tol = max(max(abs(LEFT_INV)))*1e-15;
% LEFT_INV(abs(LEFT_INV)<tol) = 0;

Ibt0 = zeros(Nb_peec,1);
Ibt1 = zeros(Nb_peec,1);
Int0 = zeros(Nn_peec,1);

tt = (Nt-1)*dt;
disp(['Total Simulation Time: ', num2str(tt*1e6,'%.2f'),' us'])
disp('Simulation Start')
cnt = 1;
for k = 2:Nt
    
    if k == ceil(Nt*(10*cnt/100))
        disp(['Simulation Complete: ', num2str(10*cnt,'%d'),'% ...'])
        cnt = cnt +1;
    end
    
    vs = id_sv.*sr_t(k);
    
    RIGHT1 = [ vs - Zright*Ibt(:,NtdL+k-1); ...
        Cleft*Vnt(:,NtdP+k-1) ];
    
    
    dLr_ib = zeros(Nb_peec,1);
    for g = 1:Nb_peec
        ind = NtdL + k - id_TdL(g,:);
        for h = 1:Nb_peec
            Ibt0(h) = Ibt(h,ind(h));
            Ibt1(h) = Ibt(h,ind(h)-1);
        end
        dLr_ib(g) = dLr(g,:)*(Ibt0-Ibt1);
    end
    
    if ~isempty(Ppeec)
        Pr_ib = zeros(Nn_peec,1);
        for g = 1:Nn_peec
            ind = NtdP + k - id_TdP(:,g);
            for h = 1:Nn_peec
                if id_si(h) == 1 || id_si(h) == -1
                    Ins(h) = id_si(h).*ist(ind(h)) ;
                end
                Int0(h) = Int(h,ind(h));
            end
            Pr_ib(g) = Ppeec(g,:)*Ins + Pr(g,:)*Int0;
        end
    end
    
    dLg_ib = zeros(Nb_peec,1);
    for g = 1:Nb_peec
        ind = NtdL + k - id_TdGL(g,:);
        for h = 1:Nb_peec
            Ibt0(h) = Ibt(h,ind(h));
            Ibt1(h) = Ibt(h,ind(h)-1);
        end
        dLg_ib(g) = dLg(g,:)*(Ibt0-Ibt1);
    end
    
    Pg_ib = zeros(Nn_peec,1);
    for g = 1:Nn_peec
        ind = NtdP + k - id_TdGP(:,g);
        for h = 1:Nn_peec
            if id_si(h) == 1 || id_si(h) == -1
                Ins(h) = id_si(h).*ist(ind(h)) ;
            end
            Int0(h) = Int(h,ind(h));
        end
        %Pg_ib(g) = Pg(g,:)*Ins + Pg(g,:)*int0;
        Pg_ib(g) = Pg(g,:)*(Ins+Int0);
    end
    
    RIGHT2 = [ dLr_ib-dLg_ib; zeros(length(Rec),1); zeros(length(dLec),1); ...
        Pr_ib-Pg_ib; zeros(length(Cec),1); ];
    
    out = LEFT_INV*(RIGHT1+RIGHT2);
    %     out = LEFT\(RIGHT1+RIGHT2);
    %     tol = 1e-3;
    %     maxit = 50;
    %     [x0,fl0,rr0,it0,rv0] = gmres(A,b,10,tol,maxit,P);
    
    if find(abs(out(1:Nb_peec))>sr_max*1e8)
        disp('The result is damping');
        break;
    elseif isnan(out)
        disp('The result is sigular');
        break;
    end
    
    Ibt(1:Nb,NtdL+k) = out(Nn+1:end);
    Int(1:Nn,NtdP+k) = Amtx*Ibt(1:Nb,NtdL+k);
    Vnt(1:Nn,NtdP+k) = out(1:Nn);
end

Ibt = Ibt(1:Nb,NtdL+1:NtdL+Nt);
Vnt = Vnt(1:Nn,NtdP+1:NtdP+Nt);


end


