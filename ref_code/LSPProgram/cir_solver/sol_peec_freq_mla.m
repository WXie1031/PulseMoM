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
function Ibf = sol_peec_freq_mla( Rpeec,Lpeec,Ppeec, TdL,TdP, Rec,Lec,Cec, ...
    Amtx, id_sv,id_si, sr_f, frq )

Rpeec = full(Rpeec);
Lpeec = full(Lpeec);
Ppeec = full(Ppeec);

[Nn, Nb] = size(Amtx);


Nf = length(frq);
if (rem(Nf,2)==0)
    Nfh = Nf/2+1;
else
    Nfh = (Nf+1)/2;
end
w = 2*pi*frq(1:Nfh);

Ibh1 = zeros(Nb, Nfh);

disp(['Total Simulation Frequencies: ', num2str(frq(Nfh),'%d'), ' to ', num2str(frq(Nfh),'%.2f'),' Hz'])
disp('Simulation Start')
cnt = 1;
for ik = 1:Nfh
    
    if ik == ceil(Nfh*(10*cnt/100))
        disp(['Simulation Complete: ', num2str(10*cnt,'%d'),'% ...'])
        cnt = cnt +1;
    end
    
    
    Vs = id_sv.*sr_f(ik);
    Is = id_si.*sr_f(ik);
    
    Z = Rpeec + 1j*w(ik)*Lpeec.*exp(-1j*w(ik)*TdL);
    P = Ppeec.*exp(-1j*w(ik)*TdP);
    
    Zall = blkdiag(Z, diag(Rec), 1j*w(ik)*diag(Lec), 1./(1j*w(ik)*diag(Cec)));
    
    %Call = blkdiag(1j*w*inv(P), 1./1j*w*diag(Cec));
    
    M = -1/(1j*w(ik))*Amtx'*P*Amtx-Zall;
    Ms = Vs+1/(1j*w(ik))*Amtx'*P*Is;
    
    Ibh1(:,ik) = M\Ms;
    
end


% only half frequency points need to calculate
if (rem(Nf,2)==0)
    Ibh2 = conj(Ibh1(:,Nfh-1:-1:2));
else
    Ibh2 = conj(Ibh1(:,Nfh:-1:2));
end
Ibf = cat(2,Ibh1(:,1:Nfh),Ibh2);


end


