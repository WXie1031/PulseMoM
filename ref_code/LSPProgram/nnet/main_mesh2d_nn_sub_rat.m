function [RmeshMF,LmeshMF] = main_mesh2d_nn_sub_rat(RmeshMF,LmeshMF, ...
    shp_id, pt_2d, dim1, dim2, Rpul, sig,mur, len, frq_cal, frq_nn, Nnlayer)

frq_pt = [1 50 100 200 300 400 500 700 1e3 1.2e3 1.5e3 2e3 2.5e3 3e3 3.5e3 ...
    4e3 5e3 7e3 7.5e3 10e3 12e3 15e3 20e3 25e3 30e3 35e3 40e3 50e3 ...
    70e3 100e3 120e3 150e3 200e3 250e3 300e3 400e3 500e3 700e3 1e6 ...
    1.2e6 1.5e6 2e6 2.5e6 3.5e6 5e6];

mu0 = 4*pi*1e-7;

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

if Nfcal < 20
    idL = (frq_pt<=max(frq_cal))&(frq_pt>=10e3);
    frq_L = frq_pt(idL);
    idR = (frq_pt<=max(frq_cal))&(frq_pt>=500);
    frq_R = frq_pt(idR);
else
    frq_L = frq_cal(frq_cal>=10e3);
    frq_R = frq_cal(frq_cal>=500);
end

%frq_tr = frq_cal;

Rmtx = ones(Nc,Nc,Nfcal);
Lmtx = zeros(Nc,Nc,Nfcal);

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
re = sqrt(S/pi);

%sig = 1./(Rpul.*S);

[Rskin, Lskin] = para_self_multi_frq(shp_id, dim1, dim2, ...
    len, Rpul, sig, mur, frq);

[~, Ltmp] = para_main_fila(pt_start, pt_end, dv, re, len);

for ic = 1:Nf
    Lmtx(:,:,ic) = Ltmp;
end

for ic = 1:Nc
    Rmtx(ic,ic,1:Nf) = Rskin(ic,1:Nf);
    Lmtx(ic,ic,1:Nf) = Lskin(ic,1:Nf);
end

rat_R = zeros(Nc,Nc,Nf);
rat_L = zeros(Nc,Nc,Nf);
rat_R(:,:,1:Nfcal) = RmeshMF(:,:,1:Nfcal)./Rmtx(:,:,1:Nfcal);
rat_L(:,:,1:Nfcal) = LmeshMF(:,:,1:Nfcal)./Lmtx(:,:,1:Nfcal);

%Nseg = 20;
%% generate the input for the NN
for ik = 1:Nc
    
    if Nfcal < 20
        rat_Rtr = interp1(frq_cal, squeeze(rat_R(ik,ik,1:Nfcal))'-1, frq_R, 'linear');
    else
        rat_Rtr = squeeze(rat_R(ik,ik,1:Nfcal))'-1;
    end
    %net_R = nn_simpe(frq_cal,squeeze(rat_R(ik,ik,1:Nfcal))'-1,Nnlayer);
%     net_R = nn_simpe(1./sqrt(frq_R), rat_Rtr, Nnlayer);

    % find the Penetration frequency
    s_dep = 1./sqrt(pi*frq_R.*sig(ik).*mu0);
    if dim2(ik)>0
        id = s_dep<dim2(ik);
    elseif isempty((find(s_dep<dim1(ik))))
        id = s_dep>dim1(ik);
    else
        id = s_dep<dim1(ik);
    end    


    x_input = [1./sqrt(frq_R(id)); (frq_R(id)).^-(1/3)];
    net_R = nn_simpe(x_input, rat_Rtr(id), Nnlayer);
    
    x_nn = [1./sqrt(frq_nn); (frq_nn).^-(1/3)];
    rat_R(ik,ik,Nfcal+1:Nf) = net_R(x_nn)+1;
    
    % check the error
    for ih = Nfcal+1:Nf
        rat_R(ik,ik,ih) = min(max(rat_R(ik,ik,ih),rat_R(ik,ik,ih-1)), 5*rat_R(ik,ik,ih-1));
    end
    
    
    for ig = ik:Nc
        
        if Nfcal < 20
            rat_Ltr = interp1(frq_cal, squeeze(rat_L(ik,ig,1:Nfcal))', frq_L, 'linear');
        else
            idL = (frq_cal>=10e3);
            rat_Ltr = squeeze(rat_L(ik,ig,idL))';
        end

        net_L = nn_simpe(1./sqrt(frq_L), rat_Ltr, Nnlayer-1);
        rat_L(ik,ig,Nfcal+1:Nf) = net_L(1./sqrt(frq_nn));
        % check the error
        for ih = Nfcal+1:Nf
            if rat_L(ik,ig,ih)<=0
                rat_L(ik,ig,ih) = rat_L(ik,ig,ih-1);
            else
                rat_L(ik,ig,ih) = max(min(rat_L(ik,ig,ih),rat_L(ik,ig,ih-1)),1/4*rat_L(ik,ig,ih-1));
            end
        end
    end
end

for ih = Nfcal+1:Nf
    rat_L(:,:,ih) = rat_L(:,:,ih)+rat_L(:,:,ih)'-diag(diag(rat_L(:,:,ih)));
end


RmeshMF(:,:,Nfcal+1:Nf) = Rmtx(:,:,Nfcal+1:Nf).*rat_R(:,:,Nfcal+1:Nf);
LmeshMF(:,:,Nfcal+1:Nf) = Lmtx(:,:,Nfcal+1:Nf).*rat_L(:,:,Nfcal+1:Nf);


end

