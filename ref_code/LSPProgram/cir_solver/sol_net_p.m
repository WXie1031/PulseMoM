%% calculate the current in the Time domain using Back Eular Method
% So      Source
function [Cmtx, nod_Pnew, pt_Pnew] = sol_net_p(Pmtx, pt_start,pt_end, pt_P,...
    nod_start,nod_end, nod_P)


%% 1. merge the same node from different branches
if ~isempty(Pmtx)
    [nod_Pnew, ind] = unique(nod_P,'rows','stable');
    pt_Pnew = pt_P(ind,1:3);
    Nn_new = size(nod_Pnew,1);
    Nn_peec = size(Pmtx,1);
    
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
        Cmtx = inv(Pmtx);
    else
        Cmtx = RvP'*inv(Pmtx)*RvP;
    end
else
    Cmtx  = [];
    [nod_Pnew, ind] = unique([nod_start;nod_end],'rows','stable');
    pt_tmp = [pt_start;pt_end];
    pt_Pnew = pt_tmp(ind,:);
end


%% 2. generate A matrix
%Amtx = sol_a_mtx(nod_start, nod_end, nod_Pnew);




end


