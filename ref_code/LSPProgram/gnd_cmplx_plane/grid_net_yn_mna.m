%% calculate the current in the Time domain using Back Eular Method
% So      Source
function [Yn, nod_Pnew] = grid_net_yn_mna(Pgrid, nod_start,nod_end, nod_P, frq)

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
        Yn = zeros(Nn_new,Nn_new,Nf);
        for ik = 1:Nf
            Yn(:,:,ik) = inv(Pgrid(:,:,ik));
        end
    else
        Yn = zeros(Nn_new,Nn_new,Nf);
        for ik = 1:Nf
            Yn(:,:,ik) = RvP'*inv(Pgrid(:,:,ik))*RvP;
            %Ptmp = inv(RvP'*inv(Pmtx)*RvP);
        end
    end
else
    Yn  = [];
    nod_Pnew = unique([nod_start;nod_end],'rows','stable');
end


end


