%% calculate the current in the Time domain using Back Eular Method
% So      Source
function [Rec,Lec,Cec, Cp,Sp, Apeec,Aec, id_sv,id_si, id_TdPr] = sol_net_mna_td_test(Lmtx,Pmtx, ...
    Rfit,Lfit, Rec,Lec,Cec, nod_P,id_TdP, nod_start,nod_end, nod_gnd,nod_sr,sr_type)

Nb_peec = size(Lmtx,1);

Noff_fit = 50000;

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

%% 2. generate node
nod_tmp = unique([nod_start;nod_end;nod_start_vf;nod_end_vf],'stable');
id_gnd = ismember(nod_tmp, nod_gnd, 'rows');
for ik = length(id_gnd):-1:1
    if id_gnd(ik) == 1
        nod_tmp(ik) = [];
    end
end

Nn = size(nod_tmp,1);
Nb = Nb_peec+Nb_fit*2;

%%  3. remove the gnd nod P
nod_peec = unique([nod_start;nod_end],'stable');
id_ngnd = ~ismember(nod_peec, nod_gnd, 'rows');

if ~isempty(Pmtx)
    Ptmp = Pmtx(id_ngnd,id_ngnd);
    Cp = diag(1./diag(Ptmp));
    id_TdPr = id_TdP(id_ngnd,id_ngnd);
    
    Nn_peec = size(Ptmp,1);
    
    Sp = zeros(Nn_peec,Nn_peec);
    for ik = 1:Nn_peec
        Sp(1:Nn_peec,ik) = Ptmp(:,ik)./Ptmp(ik,ik);
    end
else
    Cp = [];
    Sp = eye(Nn);
end


%% 3. Build Rt
Rec = [Rec, Rfittmp];
Lec = [Lec, Lfittmp];

% Ncce = size(Cec,1);
% Cec = blkdiag( diag(Cec), zeros(Nn-Nn_peec-Ncce));
% Cec = blkdiag( diag(Cec));


%% 4. add source index
id_si = zeros(Nn,1);
id_sv = zeros(Nb,1);
if sr_type == 0  % current source
   	idtmp = nod_sr(:,1) == nod_tmp;
 	id_si(idtmp) = 1;
  	idtmp = nod_sr(:,2) == nod_tmp;
 	id_si(idtmp) = -1;
    
elseif sr_type == 1  % voltage source
  	id11 = nod_sr(:,1) == nod_start;
    id12 = nod_sr(:,1) == nod_end;
    
    id21 = nod_sr(:,2) == nod_start;
    id22 = nod_sr(:,2) == nod_end;
    
  	idtmp = 2 == sum(id11+id12,2);
    id_sv(idtmp) = 1;
    
 	idtmp = 2 == sum(id21+id22,2);
    id_sv(idtmp) = 1;
end
    

%% 5. generate A matrix

Apeec = sol_a_mtx(nod_start, nod_end_tmp, nod_tmp);

Aec = sol_a_mtx([nod_start_vf; nod_start_vf], [nod_end_vf;nod_end_vf], nod_tmp);


end




