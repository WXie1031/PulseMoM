%% calculate the current in the Time domain using Back Eular Method
% So      Source
function [Rec,Lec,Cec, Pnew, Pgnew, Amtx, id_sv,id_si, id_TdPr,id_TdGPr, nod_new] = sol_net_mna_td_gnd(Lmtx,Pmtx,Pg, ...
    Rfit,Lfit, Rec,Lec,Cec, nod_P,id_TdP,id_TdGP, nod_start,nod_end, nod_gnd,nod_sr, nod_ec, sr_type)

Nb_peec = size(Lmtx,1);
Nb_ec = size(Rec,1)+size(Lec,1)+size(Cec,1);
Noff_fit = max(50000, max([nod_P; nod_ec;]+100));

%% 1. Add vector fitting circuit
Nb_fit = 0;
Nfit = zeros(Nb_peec,1);
if size(Rfit,1)>0
    for ik = 1:Nb_peec
        if find(Rfit(ik,:), 1, 'last')
            Nfit(ik) = find(Rfit(ik,:), 1, 'last');
            Nb_fit = Nb_fit+Nfit(ik);
        end
    end
end

Rfittmp = zeros(Nb_fit,1);
Lfittmp = zeros(Nb_fit,1);
nod_start_vf = zeros(Nb_fit,1);
nod_end_vf = zeros(Nb_fit,1);
nod_end_tmp = nod_end;


cnt = 0;
for ik = 1:Nb_peec

    if Nfit(ik)==1
        cnt = cnt+1;
        nod_start_vf(cnt,:) = Noff_fit+cnt;
        nod_end_vf(cnt,:)   = nod_end(ik,:);
        nod_end_tmp(ik,:)   = Noff_fit+cnt;

    elseif Nfit(ik) > 1
        for ih = 1:Nfit(ik)
            if ih == 1
                cnt = cnt+1;
                nod_start_vf(cnt,:) = Noff_fit+cnt;
                nod_end_vf(cnt,:)   = Noff_fit+cnt+1;
                nod_end_tmp(ik,:)   = Noff_fit+cnt;
            elseif ih == Nfit(ik)
                cnt = cnt+1;
                nod_start_vf(cnt,:) = Noff_fit+cnt;
                nod_end_vf(cnt,:)   = nod_end(ik,:);
            else
                cnt = cnt+1;
                nod_start_vf(cnt,:) = Noff_fit+cnt;
                nod_end_vf(cnt,:)   = Noff_fit+cnt+1;
            end
            
            Rfittmp(cnt,:) = Rfit(ik,ih);
            Lfittmp(cnt,:) = Lfit(ik,ih);
        end
    end
end



%% 3. merge the same node from different branches
if ~isempty(Pmtx)
    [nod_Ptmp, id_sel] = unique(nod_P,'stable');
    Nn_new = length(nod_Ptmp);
    Nn_peec = size(Pmtx,1);
    
    RvP = zeros(Nn_peec,Nn_new);
    for k = 1:Nn_new
        ind = (nod_P == nod_Ptmp(k));
        RvP(ind,k) = 1;
    end
    
    if size(RvP,1) == size(RvP,2)
        Pnew = Pmtx;
        Pgnew = Pg;
        id_TdPr = id_TdP;
        id_TdGPr = id_TdGP;
    else
%         Ctmp = RvP'*(Pmtx\RvP);
    %Ptmp = inv(RvP'*inv(Pmtx)*RvP);
%         Pnew = inv(Ctmp);
        Pnew = inv(RvP'*(Pmtx\RvP));
        Pgnew = inv(RvP'*(Pg\RvP));
        id_TdPr = id_TdP(id_sel,id_sel);
        id_TdGPr = id_TdGP(id_sel,id_sel);
    end
 
else
    Pnew  = [];
    Pgnew = [];
    id_TdPr = [];
    id_TdGPr = [];
end


%% 4. remove the gnd nod P
% nod_peec = unique([nod_start;nod_end],'stable');
if ~isempty(Pmtx) && ~isempty(nod_gnd)
    
    id_ngnd = ~ismember(nod_Ptmp, nod_gnd, 'rows');
    nod_Pnew = nod_Ptmp(id_ngnd);

    Pnew = Pnew(id_ngnd,id_ngnd);
    id_TdPr = id_TdP(id_ngnd,id_ngnd);
    Pgnew = Pgnew(id_ngnd, id_ngnd);
    id_TdGPr = id_TdGPr(id_ngnd,id_ngnd);
else
    nod_Pnew = []; 
end

%% 2. generate node
nod_new = unique([nod_Pnew;nod_start;nod_end;nod_ec; nod_start_vf;nod_end_vf],'stable');
if ~isempty(nod_gnd)
    id_gnd = ismember(nod_new, nod_gnd, 'rows');
    for ik = length(id_gnd):-1:1
        if id_gnd(ik) == 1
            nod_new(ik) = [];
        end
    end
end

Nn = size(nod_new,1);
Nb = Nb_peec+Nb_fit*2+Nb_ec;
    
Nn_peec_new = size(Pnew,1);
if ~isempty(Pnew)
    Pnew = blkdiag( Pnew, zeros(Nn-Nn_peec_new) );
    id_TdPr = blkdiag( id_TdPr, zeros(Nn-Nn_peec_new) );
    if ~isempty(Pgnew)
        Pgnew = blkdiag( Pgnew, zeros(Nn-Nn_peec_new) );
        id_TdGPr = blkdiag( id_TdGPr, zeros(Nn-Nn_peec_new) );
    end
end
    
%% 3. Build Rt
Rec = [Rec; Rfittmp];
Lec = [Lec; Lfittmp];


%% 4. add source index
id_si = zeros(Nn,1);
id_sv = zeros(Nb,1);
if sr_type == 0  % current source
   	%idtmp = nod_sr(:,1) == nod_tmp;
    [idtmp] = ismember(nod_new,nod_sr(:,1),'rows');
 	id_si(idtmp) = 1;
  	%idtmp = nod_sr(:,2) == nod_tmp;
    [idtmp] = ismember(nod_new,nod_sr(:,2),'rows');
 	id_si(idtmp) = -1;
    
elseif sr_type == 1  % voltage source
%     id11 = nod_sr(:,1) == nod_start;
%     id12 = nod_sr(:,1) == nod_end;
    id11 = ismember(nod_start,nod_sr(:,1),'rows');  
    id12 = ismember(nod_end,nod_sr(:,1),'rows');
    
%     id21 = nod_sr(:,2) == nod_start;
%     id22 = nod_sr(:,2) == nod_end;
    id21 = ismember(nod_start,nod_sr(:,2),'rows');  
    id22 = ismember(nod_end,nod_sr(:,2),'rows');
    
  	idtmp = 2 == sum(id11+id22,2);
    id_sv(idtmp) = 1;
    
 	idtmp = 2 == sum(id12+id21,2);
    id_sv(idtmp) = 1;
end

id_si = sparse(id_si);
id_sv = sparse(id_sv);


%% 5. generate A matrix

Amtx = sol_a_mtx([nod_start; nod_start_vf; nod_start_vf], [nod_end_tmp;nod_end_vf;nod_end_vf], nod_new);

Amtx = sparse(Amtx);


end




