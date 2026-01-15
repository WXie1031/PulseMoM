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
function [Ibt, Vnt] = emtp_conv_t_sol( Rt, Lt, Cs, Ss, Rec, Lec, Cec, Rcon, ...
    Aspc, Ht, ID_SV, ID_SI, SRt, dt, Nt )
    

global Nb % [Nn Nb] = size(A);
global Nn

Ibt = zeros(Nb,Nt);
Vnt = zeros(Nn,Nt);
Icov = zeros(Nn,Nt);

NTcov = size(Ht,2);

dLt = Lt./dt;
dLec = Lec./dt;
dCec = Cec./dt;

Rt = 1/Ht(:,1); 

Zleft = blkdiag( Rt, dLt, diag(Rec), diag(dLec), diag(Rcon) );
Cleft = blkdiag( Cs./dt, diag(dCec./dt) );

Zrigth = blkdiag( dLt, zeros(length(Rec)), diag(dLec), zeros(length(Rcon)) );

% Not Change in Time Domain
LEFT = [ -Aspc'  -Zleft; Cleft  -Ss*Aspc; ] ;

% Backward Euler Method for Solving the Network
for k = 2:Nt
    
    Vs = ID_SV.*SRt(k);
   	Is = ID_SI.*SRt(k);

    RIGHT = [ Vs - Zrigth*Ibt(:,k-1) - Icov; Ss*Is + Cleft*Vnt(:,k-1) ];

    out = LEFT\RIGHT;
    Ibt(1:Nb,k) = out(Nn+1:end);
    Vnt(1:Nn,k) = out(1:Nn);

    if k<NTcov
        Icov = sum( Aspc*Vnt(:,1:k).*Ht(Nt-(1:k)+1),2 );
    else
        Icov = sum( Aspc*Vnt(:,1:NTcov).*Ht(Nt-(1:NTcov)+1),2 );
    end
        
    %Vcov = conv(Vnt(:,1:k),Ht(:,2:NTcov), 2);

end


end


