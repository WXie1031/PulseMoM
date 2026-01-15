function [Rmesh,Lmesh,Pmesh,Rs_mesh,Ls_mesh] = update_mesh_para_on_length( ...
    Rmesh_pul, Lmesh_pul, Pmesh_pul, Rs_mesh_pul, Ls_mesh_pul, ...
    shape, dim1, dim2, len, flag_p)

disp('Update the length for MatrixPerUnit.');

[Nc, Nf] = size(Rs_mesh_pul);
Ls_mesh = zeros(Nc,Nf);
Lmesh = zeros(Nc,Nc);


%% update the R of all conductors to its length
Rmesh = Rmesh_pul*len;
Rs_mesh = Rs_mesh_pul*len;


%% update the self L of all conductors to its length
cof_len = zeros(Nc,1);
for ik = 1:Nc
    %         idrec = shape == 1002;
    %         idagi = shape == 1003;
    %         idcir = ~(idrec|idagi);

    % L calculate with multiply a modification factor
    switch shape(ik)
        case 1002
            cof_len(ik) = len.*(1 + log(len)./(1.2-log(dim1(ik)+dim2(ik))));
        case 1003
            cof_len(ik) = len.*(1 + log(len)./(1.2-log(dim1(ik)+dim2(ik)))); 
        otherwise
            cof_len(ik) = len.*(1 + log(len)./(-0.3-log(dim1(ik))));
    end
    
     Lmesh(ik,ik) = cof_len(ik).*Lmesh_pul(ik,ik);
     Ls_mesh(ik,1:Nf) = cof_len(ik).*Ls_mesh_pul(ik,1:Nf);
end


%% update the mutual L of all conductors to its length
%d0 = 30e-3;
%cof_len = len.*(1 + log(len)./(-0.3-log(d0)));

for ik = 1:Nc-1
    cofm = min(cof_len(ik), cof_len((ik+1):Nc),[], 1);
    Lmesh((ik+1):Nc,ik) = cofm.*Lmesh_pul((ik+1):Nc,ik);
end

Lmesh = Lmesh+Lmesh'-diag(diag(Lmesh));

%% update the P matrix to its length
if flag_p>0
    
    
    
end

end



