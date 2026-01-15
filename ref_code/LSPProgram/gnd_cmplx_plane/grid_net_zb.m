%% calculate the current in the Time domain using Back Eular Method
% So      Source
function [Rgn,Cgn, nod_Pnew] = grid_net_zb(Rrod,Crod, nod_start,nod_end, nod_P)


if ~isempty(Rrod)
    nod_Pnew = unique(nod_P,'rows','stable');
    Nn_new = size(nod_Pnew,1);
    Nn_peec = size(Rrod,1);
    
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
        Rgn = Rrod;
        Cgn = Crod;
    else
        Rgn = diag(inv(RvP'*inv(diag(Rrod))*RvP));
        if ~isempty(Crod) && isvector(Crod)
            Cgn = diag(RvP'*(diag(Crod))*RvP);
        elseif ~isempty(Crod) && ismatrix(Crod)
            Cgn = diag(RvP'*(Crod)*RvP);
        end
    end
else
    Rgn = [];
    Cgn = [];
    nod_Pnew = unique([nod_start;nod_end],'rows','stable');
end



end

