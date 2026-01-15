%% calculate the current in the Time domain using Back Eular Method
% So      Source
function Zbt = grid_net_zb_backup(Zmtx,Pmtx, Rrod,Crod, nod_start, nod_end, nod_P, frq)

Nf = length(frq);
Nb = size(nod_start,1);

Pmtx = full(Pmtx);

%% 2. generate A matrix
Amtx = sol_a_mtx(nod_start, nod_end, nod_P);


%% 3. generate nodal parameter matrix
Zbt = zeros(Nb,Nb,Nf);
if isempty(Pmtx)
    Zbt = Zmtx;
else% R + jwL + 1/(jw)*A*P*A'
    for ik = 1:Nf
        Zbt(:,:,ik) = Zmtx(:,:,ik)+1/(1j*2*pi*frq(ik))*(Amtx'*Pmtx(:,:,ik)*Amtx);
    end
end


end

