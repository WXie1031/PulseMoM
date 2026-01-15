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
function [Ibt, Vnt] = sol_peec_time_mna_td_test( Rpeec,Lpeec,Cpeec,Speec, ...
    Rec,Lec,Cec, id_TdL,id_TdP, Apeec,Aec, id_sv,id_si, sr_t, dt,Nt )

[Nn, Nb_peec] = size(Apeec);
[~, Nb_ec] = size(Aec);

Nb = Nb_peec+Nb_ec;

Nb_peec = size(Rpeec,1);
Nn_peec = size(Cpeec,1);
Nb_ec_r = size(Rec,1);

NtdL = max(max(id_TdL));
NtdP = max(max(id_TdP));

Ins = zeros(Nn_peec,1);
Ibt = zeros(Nb,Nt+NtdL);
Int = zeros(Nn,Nt+NtdP);
Vnt = zeros(Nn,Nt+NtdP);

ist = [ zeros(1,NtdP) sr_t ];
% vst = [ zeros(1,NtdL) sr_t ];

dLp = Lpeec./dt;
dCp = Cpeec./dt;

%% Separate the Retarded Terms and No Retarded Terms into 2 Groups
dLnr = zeros(Nb_peec,Nb_peec);
dLr = zeros(Nb_peec,Nb_peec);
id_Lnr = id_TdL == 0;
id_Lr = ~id_Lnr;
dLnr(id_Lnr) = dLp(id_Lnr);
dLr(id_Lr) = dLp(id_Lr);

Snr = zeros(Nn_peec,Nn_peec);
Sr = zeros(Nn_peec,Nn_peec);
id_Snr = id_TdP == 0;
id_Sr = ~id_Snr;
Snr(id_Snr) = Speec(id_Snr);
Sr(id_Sr) = Speec(id_Sr);

dLec = Lec./dt;

Zleft = (Rpeec+dLnr);
Cleft = blkdiag(dCp, zeros(Nn-Nn_peec));

clear id_Lnr id_Lr id_Snr id_Sr

% Not Change in Time Domain
if Nb_ec == 0
    INV_LEFT = inv([-Apeec'            -Zleft 
                    Cleft    -Snr*Apeec(1:Nn_peec,:); ]) ;
else
    INV_LEFT = inv([-Apeec'       -Zleft             zeros(Nb_peec,Nb_ec); 
                    -Aec'    zeros(Nb_ec,Nb_peec)    -blkdiag(diag(Rec),diag(dLec));
                    Cleft    -[ Snr*[Apeec(1:Nn_peec,:) Aec(1:Nn_peec,:)];[Apeec(Nn_peec+1:Nn,:) Aec(Nn_peec+1:Nn,:)];];
                    ]) ;           
end

%% %%%%%%% Backward Euler Method for Solving the Network %%%%%%%%%
Ibt0 = zeros(Nb_peec,1);
Ibt1 = zeros(Nb_peec,1);
Int0 = zeros(Nn_peec,1);
for k = 2:Nt
    
    vs = id_sv.*sr_t(k);
    RIGHT1 = [  vs(1:Nb_peec) - dLnr*Ibt(1:Nb_peec,NtdL+k-1);
                vs(Nb_peec+1:Nb_peec+Nb_ec_r,1);
                vs(Nb_peec+Nb_ec_r+1:Nb,1) - dLec.*Ibt(Nb_peec+Nb_ec_r+1:Nb,NtdL+k-1);
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
    
    Sr_ib = zeros(Nn_peec,1);
    for g = 1:Nn_peec
        ind = NtdP + k - id_TdP(g,:);
        for h = 1:Nn_peec
            if id_si(h) == 1 || id_si(h) == -1
                Ins(h) = id_si(h).*ist(ind(h)) ;
            end
            Int0(h) = Int(h,ind(h));
        end
        Sr_ib(g) = Speec(g,:)*Ins + Sr(g,:)*Int0;
    end
       
    RIGHT2 = [ dLr_ib; zeros(length(Rec),1); zeros(length(Lec),1);
        Sr_ib; zeros(Nn-Nn_peec,1);  ];
    
    out = INV_LEFT*(RIGHT1+RIGHT2);
    
    Ibt(1:Nb,NtdL+k) = out(Nn+1:end);
    Int(1:Nn,NtdP+k) = [Apeec Aec]*Ibt(1:Nb,NtdL+k);
    Vnt(1:Nn,NtdP+k) = out(1:Nn);
end

Ibt = Ibt(1:Nb,NtdL+1:NtdL+Nt);
Vnt = Vnt(1:Nn,NtdP+1:NtdP+Nt);


end


