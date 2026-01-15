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
% %%%%%%%%%%%%%%%%%%%%%%%%%%%% revised by Chen Hongcai 2013.11
function [Ib, Vn] = MNA_GFsolver( Z, P, Rec, Lec, Cec, Rcon, ...
    A, ID_SV, ID_SI, SR_F, w )

global Nn % [Nn Nb] = size(A);

Vs = ID_SV.*SR_F;
Is = ID_SI.*SR_F; 


Zall = blkdiag(Z, diag(Rec), 1j*w*diag(Lec), diag(Rcon));
Call = blkdiag(1j*w*inv(P), 1j*w*diag(Cec));

size( [ -A' -Zall ; Call  -A; ])
out = [ -A' -Zall ; Call  -A; ]\[ Vs; Is ] ;

Ib = out(Nn+1:end);
Vn = out(1:Nn);


end


