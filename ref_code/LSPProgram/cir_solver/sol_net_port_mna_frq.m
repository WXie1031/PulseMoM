%% calculate the current in the Time domain using Back Eular Method
% So      Source
function [Zport, Amtx, id_sv,id_si,  nod_new] ...
    = sol_net_port_mna_frq(Lmtx,Pmtx, Rec,Lec,Cec, ...
      nod_start,nod_end, nod_P, nod_gnd,nod_sr, nod_ec, nod_port, sr_type)

Nb_peec = size(Lmtx,1);
Nb_ec = size(Rec,1)+size(Lec,1)+size(Cec,1);


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
    else
%         Ctmp = RvP'*(Pmtx\RvP);
    %Ptmp = inv(RvP'*inv(Pmtx)*RvP);
%         Pnew = inv(Ctmp);
        Pnew = inv(RvP'*(Pmtx\RvP));
    end
 
else
    Pnew  = [];
end


%% 4. remove the gnd nod P
% nod_peec = unique([nod_start;nod_end],'stable');
if ~isempty(Pmtx) && ~isempty(nod_gnd)
    
    id_ngnd = ~ismember(nod_Ptmp, nod_gnd, 'rows');
    nod_Pnew = nod_Ptmp(id_ngnd);

    Pnew = Pnew(id_ngnd,id_ngnd);
else
    nod_Pnew = []; 
end

%% 2. generate node
nod_new = unique([nod_Pnew;nod_start;nod_end;nod_ec; ],'stable');
if ~isempty(nod_gnd)
    id_gnd = ismember(nod_new, nod_gnd, 'rows');
    for ik = length(id_gnd):-1:1
        if id_gnd(ik) == 1
            nod_new(ik) = [];
        end
    end
end

Nn = size(nod_new,1);
Nb = Nb_peec+Nb_ec;
    
Nn_peec_new = size(Pnew,1);
if ~isempty(Pnew)
    Pnew = blkdiag( Pnew, zeros(Nn-Nn_peec_new) );
end
    


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
%   	id11 = nod_sr(:,1) == nod_start;
%     id12 = nod_sr(:,1) == nod_end;
    id11 = ismember(nod_start,nod_sr(:,1),'rows');  
    id12 = ismember(nod_end,nod_sr(:,1),'rows');
    
%     id21 = nod_sr(:,2) == nod_start;
%     id22 = nod_sr(:,2) == nod_end;
    id21 = ismember(nod_start,nod_sr(:,2),'rows');  
    id22 = ismember(nod_end,nod_sr(:,2),'rows');
    
  	idtmp = 2 == sum(id11+id12,2);
    id_sv(idtmp) = 1;
    
 	idtmp = 2 == sum(id21+id22,2);
    id_sv(idtmp) = 1;
end

id_si = sparse(id_si);
id_sv = sparse(id_sv);


%% 5. generate A matrix

Amtx = sol_a_mtx([nod_start; ], [nod_end;], nod_new);

Amtx = sparse(Amtx);



Zmtx = zeros(Nbt,Nbt,Nf);
Pmtx = zeros(Nn,Nn,Nf);
for ik = 1:Nf
    w0 = 2*pi*frq(ik);
    Zmtx(:,:,ik) = Rpeec + 1j*w0*Lpeec.*exp(-1j*w0*TdL);
    if p_flag>0
        Pmtx(:,:,ik) = Ppeec*exp(-1j*w0*TdP);
    end
end

Np = 
Zport = zeros(Np,Np,Nf);
% B - select matrix for the port - Nc*Np
for ik = 1:Nf
    w0 = 2*pi*frq(ik);
    Zport(:,:,ik) = inv(B' * (Amtx*inv(Zmtx(:,:,ik))*Amtx' + 1j*w0*inv(Pmtx(:,:,ik))) * B);
end


end




