function [Rself, Lself] = update_self_para_on_length(Rself_pul, Lself_pul, ...
    shape, dim1, dim2, len, R_pul, sig, mur, frq)


[Nc, Nf] = size(Lself_pul);

Rself = zeros(Nc,Nf);
Lself = zeros(Nc,Nf);

%% update the self L of all conductors to its length
for ik = 1:Nc
    %         idrec = shape == 1002;
    %         idagi = shape == 1003;
    %         idcir = ~(idrec|idagi);
    
    if len(ik)>=0.8
        % L calculate with multiply a modification factor
        switch shape(ik)
            case 1002
                cof = len(ik).*(1 + log(len(ik))./(1.2-log(dim1(ik)+dim2(ik))));
                Lself(ik,1:Nf) = cof.*Lself_pul(ik,1:Nf);
                
            case 1003
                cof = len(ik).*(1 + log(len(ik))./(1.2-log(dim1(ik)+dim2(ik))));
                Lself(ik,1:Nf) = cof.*Lself_pul(ik,1:Nf);
                
            otherwise
                cof = len(ik).*(1 + log(len(ik))./(-0.3-log(dim1(ik))));
                Lself(ik,1:Nf) = cof.*Lself_pul(ik,1:Nf);
        end
        Rself(ik,1:Nf) = len(ik).*Rself_pul(ik,1:Nf);
        
    else
        [Rself(ik,1:Nf), Lself(ik,1:Nf)] = para_main_self_multi_frq(...
            shape(ik), dim1(ik), dim2(ik), len(ik), R_pul(ik), sig(ik), mur(ik,:), frq);
    end
end


end

