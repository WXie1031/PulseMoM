function [RmeshMF,LmeshMF] = main_mesh2d_nn_sub_val(RmeshMF,LmeshMF, ...
    frq_cal, frq_nn, Nnlayer)

frq_pt = [1 50 100 200 300 400 500 700 1e3 1.2e3 1.5e3 2e3 2.5e3 3e3 3.5e3 4e3 5e3 7e3 7.5e3 10e3 ...
    12e3 15e3 20e3 25e3 30e3 35e3 40e3 50e3 70e3 100e3 120e3 150e3 200e3 250e3 300e3 400e3 500e3 700e3 1e6 ...
    1.2e6 1.5e6 2e6 2.5e6 3.5e6 5e6];


%% calculate the skin effect
if size(frq_cal,1)>1
    frq_cal = frq_cal';
end
if size(frq_nn,1)>1
    frq_nn = frq_nn';
end

frq = [frq_cal, frq_nn;];
Nc = size(RmeshMF,1);
Nf = length(frq);
Nfcal = length(frq_cal);

if Nfcal < 20
    idL = (frq_pt<=max(frq_cal))&(frq_pt>=1e3);
    frq_L = frq_pt(idL);
    idR = (frq_pt<=max(frq_cal));
    frq_R = frq_pt(idR);
else
    frq_L = frq_cal(frq_cal>=1e3);
    frq_R = frq_cal;
end

%frq_tr = frq_cal;


%Nseg = 20;
%% generate the input for the NN
for ik = 1:Nc
    
    if Nfcal < 20
        Rtmp = interp1(frq_cal, squeeze(RmeshMF(ik,ik,1:Nfcal))', frq_R, 'linear');
    else
        Rtmp = squeeze(RmeshMF(ik,ik,1:Nfcal))';
    end
    
    x_input = [1./sqrt(frq_R); (frq_R).^-(1/3); (frq_R).^-(1/5)];
    %net_R = nn_simpe(frq_cal,squeeze(rat_R(ik,ik,1:Nfcal))'-1,Nnlayer);
    net_R = nn_simpe(x_input, (Rtmp), Nnlayer);
    
    x_nn = [1./sqrt(frq_nn); (frq_nn).^-(1/3);(frq_nn).^-(1/5)];
    RmeshMF(ik,ik,Nfcal+1:Nf) = (net_R(x_nn));
    
%     % check the error
    for ih = Nfcal+1:Nf
        RmeshMF(ik,ik,ih) = max(RmeshMF(ik,ik,ih),RmeshMF(ik,ik,ih-1));
    end
    
    
    for ig = ik:Nc
        
        if Nfcal < 20
            Ltmp = interp1(frq_cal, squeeze(LmeshMF(ik,ig,1:Nfcal))'*1e6, frq_L, 'linear');
        else
            idL = find(frq_cal>=10e3);
            Ltmp = squeeze(LmeshMF(ik,ig,idL))'*1e6;
        end

        net_L = nn_simpe(1./sqrt(frq_L), (Ltmp), Nnlayer);
        LmeshMF(ik,ig,Nfcal+1:Nf) = (net_L(1./sqrt(frq_nn)))/1e6;
        % check the error
        for ih = Nfcal+1:Nf
            if LmeshMF(ik,ig,ih)<=0
                LmeshMF(ik,ig,ih) = LmeshMF(ik,ig,ih-1);
            else
                LmeshMF(ik,ig,ih) = min(LmeshMF(ik,ig,ih),LmeshMF(ik,ig,ih-1));
            end
        end
    end
end


for ih = Nfcal+1:Nf
    LmeshMF(:,:,ih) = LmeshMF(:,:,ih)+LmeshMF(:,:,ih)'-diag(diag(LmeshMF(:,:,ih)));
end



end

