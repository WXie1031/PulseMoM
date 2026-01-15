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
function [Ibt, Vnt] = sol_peec_time_mna_k_bk_xyz( Rvx,Rvy,Rvz, Lmx,Lmy,Lmz, ...
    Cxyz,Sxyz, Rec,Lec,Cec, Axyz,Aec, id_sv,id_si, sr_t,dt,Nt )
    
%% not accurate when the mesh is dense

% special designed for orthogonal PEEC
dh = 1/dt;

K_thr = 0.2/100;

[Nn, Nb_peec] = size(Axyz);
[~, Nb_ec] = size(Aec);

Nb_r = size(Rec,1);
Nb_l = size(Lec,1);

%Nn = Nn;
Nb = Nb_peec+Nb_ec;

Ibt = zeros(Nb,Nt);
Vnt = zeros(Nn,Nt);

Kpeec = blkdiag( inv(Lmx), inv(Lmy), inv(Lmz) );
Ks = diag(Kpeec);
for ik = 1:Nb_peec-1
    kij = sqrt( Kpeec(ik+1:end,ik).^2./(abs(Ks(ik).*Ks(ik+1:end))) );
    ind = kij>K_thr;
    
    Kpeec(ik+1:end,ik) = Kpeec(ik+1:end,ik).*ind;
    Kpeec(ik,ik+1:end) = Kpeec(ik+1:end,ik)';
end


Rpeec = diag( [Rvx; Rvy; Rvz] ) ;

dLec = Lec./dt;

Ncce = size(Cec,1);
if isempty(Cxyz)
    Cleft = blkdiag( zeros(Nn-Ncce), diag(Cec./dt) );
else
    Cleft = blkdiag( Cxyz./dt, diag(Cec./dt) );
end

% Not Change in Time Domain
if Nb_ec == 0
    LEFT = [ -Kpeec*Axyz'  -Kpeec*Rpeec-dh*eye(Nb_peec);
             Cleft         -Sxyz*Axyz           ;] ;
else
    LEFT = [ -Kpeec*Axyz'  -Kpeec*Rpeec-dh*eye(Nb_peec)       zeros(Nb_peec,Nb_ec);
             -Aec'         zeros(Nb_ec,Nb_peec)  blkdiag( diag(Rec), diag(dLec) );
             Cleft         -Sxyz*Axyz            -Aec;          ] ;
end

INV_LEFT = inv(LEFT);  % inverse the matrix once

% Backward Euler Method for Solving the Network
for k = 2:Nt
    Vs = id_sv.*sr_t(k);
   	Is = id_si.*sr_t(k);
    
    Vstmp = [Kpeec*Vs(1:Nb_peec) - dh.*Ibt(1:Nb_peec,k-1); Vs(Nb_peec+1:Nb);];

    if isempty(dLec)
        Vstmp(Nb_peec+Nb_r+(1:Nb_l)) = Vstmp(Nb_peec+Nb_r+(1:Nb_l));
    else
        Vstmp(Nb_peec+Nb_r+(1:Nb_l)) = Vstmp(Nb_peec+Nb_r+(1:Nb_l)) - dLec.*Ibt(Nb_peec+Nb_r+(1:Nb_l),k-1);
    end
    
    RIGHT = [ Vstmp; 
              Sxyz*Is + Cleft*Vnt(:,k-1) ];
            
    out = INV_LEFT*RIGHT;
    
    Ibt(1:Nb,k) = out(Nn+1:end);
    Vnt(1:Nn,k) = out(1:Nn);
end


end


