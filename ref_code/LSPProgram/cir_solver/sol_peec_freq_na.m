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
function [Ibf, Vnf] = sol_peec_freq_na( Rpeec,Lpeec,Ppeec, TdL,TdP, Rec,Lec,Cec, ...
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


Vnh1 = zeros(Nn, Nfh);
Ibh1 = zeros(Nb, Nfh);
% Cs = diag(1./diag(Ppeec));
% Ss = zeros(Nn_peec,Nn_peec);
% for ik = 1:Nn_peec
%     Ss(1:Nn_peec,ik) = Ppeec(:,ik)./Ppeec(ik,ik);
% end


disp(['Total Simulation Frequencies: ', num2str(frq(Nfh),'%d'), ' to ', num2str(frq(Nfh),'%.2f'),' Hz'])
disp('Simulation Start')
cnt = 1;
for ik = 1:Nfh
    
    if ik == ceil(Nfh*(10*cnt/100))
        disp(['Simulation Complete: ', num2str(10*cnt,'%d'),'% ...'])
        cnt = cnt +1;
    end
    
    
    Is = id_si.*sr_f(ik);
    Vs = id_sv.*sr_f(ik);
    
    Z = Rpeec + 1j*w(ik)*Lpeec.*exp(-1j*w(ik)*TdL);
    C = inv(Ppeec.*exp(-1j*w(ik)*TdP));
    
    Yall = inv(blkdiag(Z, diag(Rec), 1j*w(ik)*diag(Lec)));
    Call = blkdiag(1j*w(ik)*C, 1j*w(ik)*diag(Cec));

    
    %out = [ -A' -Z ; 1j*w*F  -S*A; ]\[ Vs; S*Is ] ;
    Y = Amtx*Yall*Amtx' + Call;
    
    out = Y\(Is - Amtx*Yall*Vs);
    
    if isinf(out)
        disp('The result is damping');
        break;
    elseif isnan(out)
        disp('The result is sigular');
        break;
    end

    Vnh1(:,ik) = out;
    Ibh1(:,ik) = Yall*( -Amtx'*out-Vs );
end


% only half frequency points need to calculate
if (rem(Nf,2)==0)
    Vnh2 = conj(Vnh1(:,Nfh-1:-1:2));
    Ibh2 = conj(Ibh1(:,Nfh-1:-1:2));
else
    Vnh2 = conj(Vnh1(:,Nfh:-1:2));
    Ibh2 = conj(Ibh1(:,Nfh:-1:2));
end


Vnf = cat(2,Vnh1(:,1:Nfh),Vnh2);
Ibf = cat(2,Ibh1(:,1:Nfh),Ibh2);


end


