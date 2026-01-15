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
function [Ibt, Vnt] = sol_peec_time_na( Rt, Lt, Ct, Ss,  ...
    Amtx, id_sv, id_si, sr_t, dt, Nt )
    
[Nn, Nb] = size(Amtx);

Ibt = zeros(Nb,Nt);
Vnt = zeros(Nn,Nt);

dLt = Lt./dt;

Zleft = blkdiag( Rt+dLt );
Cleft = blkdiag( Ct./dt );

Zrigth = blkdiag( dLt );

% Not Change in Time Domain
% inverse the matrix once
% LEFT = [ -Amtx'  -Zleft; Cleft  -Ss*Amtx; ] ;
INV_LEFT = inv( 2/dt*P+dt/2*Amtx*inv( [-Amtx'  -Zleft; Cleft  -Ss*Amtx; ]);  

% Backward Euler Method for Solving the Network
for k = 2:Nt
    Vs = id_sv.*sr_t(k);
   	Is = id_si.*sr_t(k);
    
    RIGHT = [ Vs - Zrigth*Ibt(:,k-1); 
              Ss*Is + Cleft*Vnt(:,k-1) ];
    out = INV_LEFT*RIGHT;
    
    Ibt(1:Nb,k) = out(Nn+1:end);
    Vnt(1:Nn,k) = out(1:Nn);
end


end


