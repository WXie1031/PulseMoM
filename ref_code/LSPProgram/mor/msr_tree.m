function Lmor = msr_tree(Rmtx,Lmtx, nd_start,nd_end, nod_name, nod_sr,nod_gnd, ...
    fmor, mor_ord)

disp('Model size reduction is adopted.');
% A - [node, branch]
Nc = size(Lmtx,1);

wmor = 2*pi*fmor;

Lmtx_diag = zeros(Nc,Nc);
for ik = 1:mor_ord
    Lmtx_diag = Lmtx_diag + diag(diag(Lmtx,ik),ik);
end
Lmtx_diag = Lmtx_diag+Lmtx_diag';
Lmtx_diag = Lmtx_diag+diag(diag(Lmtx));


% required to kown source and ground node
%     [~,id_sr] = ismember(nod_sr,nd_Pnew,'rows');
%     [~,id_gnd] = ismember(nod_gnd,nd_Pnew,'rows');

id_gnd = find(ismember(nod_name,nod_gnd,'rows'));

nd_tmp = nod_name;
nd_tmp(id_gnd,:) = [];
Amtx = sol_a_mtx(nd_start, nd_end, nd_tmp);

[Nn, Nb] = size(Amtx);
id_sr = find(ismember(nd_tmp,nod_sr,'rows'));

Vs = zeros(Nb,1);
Is = zeros(Nn,1);
Is(id_sr) = 1;
%     Imor = zero(Nb,1);
%     Vmor = zeros(Nn,1);
Ztmp = Rmtx + 1j*wmor*Lmtx_diag;
out = [ -Amtx' -Ztmp ; zeros(Nn,Nn)  -Amtx; ]\[ Vs; Is ] ;
Imor = out(Nn+1:end,:);

Lmor = Lmtx;
for ik = 1:Nc
    V = 1j*wmor*Lmtx(ik,:) .* Imor';
    Vt = abs(sum(V));
    id_remove = abs(V) < 0.005*Vt;
    id_remove(ik) = 0;
    Lmor(ik, id_remove) = 0;
end

Nmor = sum(sum(Lmor~=0));
Nori = sum(sum(Lmtx~=0));


disp(['The number of inductance term is reduced from ', num2str(Nori), ' to ', num2str(Nmor)]);

end



