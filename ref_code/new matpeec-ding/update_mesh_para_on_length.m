function [Rmesh, Lmesh] = update_mesh_para_on_length(Rmesh_pul, Lmesh_pul, ...
    shape, dim1, dim2, len)

disp('Update the length for MatrixPerUnit.');

Nc = size(Lmesh_pul,1);

Rmesh = Rmesh_pul*len;

d0 = 30e-3;
cof = len.*(1 + log(len)./(-0.3-log(d0)));
Lmesh = cof*(Lmesh_pul-diag(diag(Lmesh_pul)));

%% update the self L of all conductors to its length
for ik = 1:Nc
    %         idrec = shape == 1002;
    %         idagi = shape == 1003;
    %         idcir = ~(idrec|idagi);
    
    % L calculate with multiply a modification factor
    switch shape(ik)
        case 1002
            cof = len.*(1 + log(len)./(1.2-log(dim1(ik)+dim2(ik))));
            
            Lmesh(ik,ik) = cof.*Lmesh_pul(ik,ik);
            
        case 1003
            cof = len.*(1 + log(len)./(1.2-log(dim1(ik)+dim2(ik))));
            
            Lmesh(ik,ik) = cof.*Lmesh_pul(ik,ik);
            
        otherwise
            cof = len.*(1 + log(len)./(-0.3-log(dim1(ik))));
            
            Lmesh(ik,ik) = cof.*Lmesh_pul(ik,ik);
    end
     
end



end

