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
function [Ibt, Vnt] = sol_peec_time_mna_tr( Rt, Lt, Ct,  ...
    Amtx, id_sv, id_si, sr_t, dt, Nt )
    
[Nn, Nb] = size(Amtx);
Nn_peec = size(Ct,1);

Ibt = zeros(Nb,Nt);
Vnt = zeros(Nn,Nt);
Is_old  = zeros(Nn,1);
Vs_old  = zeros(Nb,1);

d2Lt = 2*Lt./dt;

Zleft = blkdiag( Rt+d2Lt );
Cleft = blkdiag( 2*Ct./dt, zeros(Nn-Nn_peec) );
if isempty(Cleft)
    Cleft = zeros(Nn,Nn);
end

Zrigth = blkdiag( d2Lt );

% Not Change in Time Domain
% inverse the matrix once
% LEFT = [ -Amtx'  -Zleft; Cleft  -Ss*Amtx; ] ;
INV_LEFT = inv([ -Amtx'  -Zleft; Cleft  -Amtx; ]);  

% Backward Euler Method for Solving the Network
for k = 2:Nt
    Vs = id_sv.*sr_t(k);
   	Is = id_si.*sr_t(k);
    
    RIGHT = [ Vs+Vs_old - Zrigth*Ibt(:,k-1) + Amtx'*Vnt(:,k-1); 
              Is+Is_old + Cleft*Vnt(:,k-1) + Amtx*Ibt(:,k-1); ];
    out = INV_LEFT*RIGHT;

    if find(abs(out(1:Nb))>1e20)
        disp('The result is damping');
        break;
    elseif isnan(out)
        disp('The result is sigular');
        break;
    end
    
    Ibt(1:Nb,k) = out(Nn+1:end);
    Vnt(1:Nn,k) = out(1:Nn);
    
    Is_old = Is;
    Vs_old = Vs;
end


end


