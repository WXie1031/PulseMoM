% MNA_SOLVER   solve the matrix of the circuit using MNA method and
% backEular
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
function [Ibt, Vnt] = sol_peec_time_mna_bu( Rt, Lt, Ct, Rec,Lec,Cec, ...
    Amtx, id_sv, id_si, sr_t, dt, Nt )
    
[Nn, Nb] = size(Amtx);
Nn_peec = size(Ct,1);
Nn_Cec = size(Cec,1);

Ibt = zeros(Nb,Nt);
Vnt = zeros(Nn,Nt);

dLt = Lt./dt;
dLec = Lec./dt;
dCec = Cec./dt;

Zleft = blkdiag(blkdiag( Rt+dLt ), diag(Rec), diag(dLec) );
Cleft = blkdiag( Ct./dt, diag(dCec), zeros(Nn-Nn_peec-Nn_Cec) );
% if isempty(Cleft)
%     Cleft = zeros(Nn,Nn);
% end

Zright = blkdiag( dLt, zeros(length(Rec)), diag(dLec)  );

% Not Change in Time Domain
% inverse the matrix once
% LEFT = [ -Amtx'  -Zleft; Cleft  -Ss*Amtx; ] ;
INV_LEFT = inv([ -Amtx'  -Zleft; 
                  Cleft  -Amtx; ]);  

% Backward Euler Method for Solving the Network
for k = 2:Nt
    Vs = id_sv.*sr_t(k);
   	Is = id_si.*sr_t(k);
    
    RIGHT = [ Vs - Zright*Ibt(:,k-1); 
              Is + Cleft*Vnt(:,k-1) ];
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
end


end


