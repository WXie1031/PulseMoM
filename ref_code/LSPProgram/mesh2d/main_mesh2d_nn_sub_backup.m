function [RmeshMF,LmeshMF] = main_mesh2d_nn_sub_backup(RmeshCal,LmeshCal, ...
    shp_id, pt_2d, dim1, dim2, Rpul, mur, len, frq_cal, frq_nn, Nnlayer)


%% calculate the skin effect
if size(frq_cal,1)>1
    frq_cal = frq_cal';
end
if size(frq_nn,1)>1
    frq_nn = frq_nn';
end
frq = [frq_cal, frq_nn;];
Nc = length(dim1);
Nf = length(frq);
Nfcal = length(frq_cal);


Rmtx = ones(Nc,Nc,Nf);
Lmtx = zeros(Nc,Nc,Nf);

RmeshMF = zeros(Nc,Nc,Nf);
LmeshMF = zeros(Nc,Nc,Nf);

pt_start = [pt_2d, zeros(Nc,1)];
pt_end = [pt_2d, len*ones(Nc,1)];
[dv, len] = line_dv(pt_start, pt_end);

%mur = ones(Nc,1);
id_rec = shp_id == 1002;
id_agi = shp_id == 1003;
id_cir = ~(id_rec + id_agi);

S = zeros(Nc,1);
S(id_rec) = dim1(id_rec).*dim2(id_rec);
S(id_agi) = abs(2*dim1(id_agi).*dim2(id_agi)) - dim2(id_agi).*dim2(id_agi);
S(id_cir) = pi*(dim1(id_cir).^2 - dim2(id_cir).^2);

sig = 1./(Rpul.*S);

[Rskin, Lskin] = para_self_multi_frq(shp_id, dim1, dim2, ...
    len, Rpul, sig, mur, frq);

re = sqrt(S/pi);

[~, Ltmp] = para_main_fila(pt_start, pt_end, dv, re, len);

for ic = 1:Nf
    Lmtx(:,:,ic) = Ltmp;
end

for ic = 1:Nc
    Rmtx(ic,ic,:) = Rskin(ic,:);
    Lmtx(ic,ic,:) = Lskin(ic,:);
end

RmeshMF(:,:,1:Nfcal) = RmeshCal;
LmeshMF(:,:,1:Nfcal) = LmeshCal;

rat_Rsp = RmeshMF./Rmtx;
rat_Lsp = LmeshMF./Lmtx;

Nnn = min(Nnlayer,Nfcal);
%% generate the input for the NN
for ik = 1:Nc
    % self terms
    nn_input = zeros(1,Nfcal);
    nn_output = zeros(2,Nfcal);
    
    nn_input(1:Nfcal) = sqrt(frq_cal);
    nn_output(1,1:Nfcal)= rat_Rsp(ik,ik,1:Nfcal);
    nn_output(2,1:Nfcal)= rat_Lsp(ik,ik,1:Nfcal);
%     nn_output(1,1:Nfcal)= RmeshMF(ik,ik,1:Nfcal);
%     nn_output(2,1:Nfcal)= LmeshMF(ik,ik,1:Nfcal);
    
    net = trainNet(nn_input,nn_output, Nnn);
    
    rat_out = net(sqrt(frq_nn));
    RmeshMF(ik,ik,Nfcal+1:Nf) = Rskin(ik,Nfcal+1:Nf).*(rat_out(1,:));
    LmeshMF(ik,ik,Nfcal+1:Nf) = Lskin(ik,Nfcal+1:Nf).*(rat_out(2,:));
%     RmeshMF(ik,ik,Nfcal+1:Nf) = (rat_out(1,:));
%     LmeshMF(ik,ik,Nfcal+1:Nf) = (rat_out(2,:));
    % off diagonal terms
    for ig = ik+1:Nc
        nn_input = zeros(1,Nfcal);
        nn_output = zeros(2,Nfcal);
        
        nn_input(1:Nfcal) = sqrt(frq_cal);
        nn_output(1,1:Nfcal)= rat_Rsp(ik,ik,1:Nfcal);
        nn_output(2,1:Nfcal)= rat_Lsp(ik,ik,1:Nfcal);
%         nn_output(1,1:Nfcal)= RmeshMF(ik,ik,1:Nfcal);
%         nn_output(2,1:Nfcal)= LmeshMF(ik,ik,1:Nfcal);
        
        net = trainNet(nn_input,nn_output, Nnn);
        
        rat_out = net(sqrt(frq_nn));
        LmeshMF(ik,ig,Nfcal+1:Nf) = squeeze(Lmtx(ik,ig,Nfcal+1:Nf)).*(rat_out(2,:)');
%         LmeshMF(ik,ig,Nfcal+1:Nf) = (rat_out(2,:));
    end
    
end





