%% calculate the current in the Time domain using Back Eular Method
% So      Source
function [Ynt,Rgn,Cgn, nod_Pnew] = grid_net_yn_na(Zgrid,Pgrid,Rrod,Crod, nod_start,nod_end, nod_P, frq)

%Nn = size(nod_P,1);
Nf = length(frq);

%% 1. merge the same node from different branches
if ~isempty(Pgrid)
    nod_Pnew = unique(nod_P,'rows','stable');
    Nn_new = size(nod_Pnew,1);
    Nn_peec = size(Pgrid,1);
    
    RvP = zeros(Nn_peec,Nn_new);
    
    if isnumeric(nod_P)
        for ik = 1:Nn_new
            ind = (nod_P == nod_Pnew(ik));
            RvP(ind,ik) = 1;
        end
    else
        Nn_old = size(nod_P,1);
        for ik = 1:Nn_new
            for ig = 1:Nn_old
                if strcmp(nod_Pnew(ik,:),nod_P(ig,:))
                    RvP(ig,ik) = 1;
                end
            end
        end
    end
        
    if size(RvP,1) == size(RvP,2)
        Cn = zeros(Nn_new,Nn_new,Nf);
        for ik = 1:Nf
            Cn(:,:,ik) = inv(Pgrid(:,:,ik));
        end
        Rgn = Rrod;
        Cgn = Crod;
    else
        Cn = zeros(Nn_new,Nn_new,Nf);
        for ik = 1:Nf
            Cn(:,:,ik) = RvP'*inv(Pgrid(:,:,ik))*RvP;
            %Ptmp = inv(RvP'*inv(Pmtx)*RvP);
        end
        Rgn = 1./ diag((RvP'*((diag(1./Rrod)))*RvP));
        Cgn =  diag((RvP'*((diag(Crod)))*RvP));
    end
else
    Cn  = [];
    nod_Pnew = unique([nod_start;nod_end],'rows','stable');
end


%% 2. generate A matrix
Amtx = sol_a_mtx(nod_start, nod_end, nod_Pnew);


%% 3. generate nodal parameter matrix
Yn = zeros(Nn_new,Nn_new,Nf);
Ynt = zeros(Nn_new,Nn_new,Nf);
if isempty(Pgrid)
    for ik = 1:Nf
        Ynt(:,:,ik) = Amtx*inv(Zgrid(:,:,ik))*Amtx';
    end
else  % A*inv(R+jwL)*A' + (jw)*P
    for ik = 1:Nf
        Yn(:,:,ik) = Amtx*inv(Zgrid(:,:,ik))*Amtx';
        Ynt(:,:,ik) = Yn(:,:,ik) + (Cn(:,:,ik));
        %Znt(:,:,ik) = inv( Ynt(:,:,ik) );  % nodal analysis method (NA)
    end
end


end


