function [Rmesh,Lmesh, Rself,Lself, RmeshMF,LmeshMF] = main_mesh2d_nn_backup( ...
    shp_id, pt_2d, dim1, dim2, Rpul, mur, len, frq)
%  Function:       mesh2d_main_NN
%  Description:    calculate cable group using NN method (for high frqeuncy)
%                  R is the resistance under 500Hz,
%                  L is the inductance under 20kHz.
%                  In this version, the data must be rearranged by shape.
%                  First block -- round
%                  Second block -- rectangle
%                  Third block -- angle steel
%
%  Calls:          cal_RL_mesh2d_complete
%
%  Input:          pt2D     --  point offset in 2D view (N*2) (m)
%                  dim1     --  w/rout (N*1) (m)
%                  dim2     --  t/rin (N*1) (m)
%                  Rin_pul  --  resistance of conductors (N*1) (ohm/m)
%                  Lin_pul
%                  sig      --  conductivity of conductors(N*1)(Sig/m)
%                  mur      --  relative permeability of conductors(N*1)
%
%  Output:         Rmesh      --  R matrix (N*N) (ohm)
%                  Lmesh      --  L matrix (N*N) (H)
%                  Rself      --  R matrix (N*Nf) (ohm)
%                  Lself      --  L matrix (N*Nf) (H)
%                  RmeshMF    --  R 3D matrix (N*N*Nf) (ohm)
%                  LmeshMF    --  L 3D matrix (N*N*Nf) (H)
%
%  Others:
%  Author:         Chen Hongcai
%  Email :         hc.chen@live.com
%  Date:           2017-03-07


fprintf ('Calculate the parameter matrix of the cable group (neural network mod).\n');

timerSta = tic;

frq_cal = frq(frq<=100e3);
frq_nn = frq(frq>100e3);

Nnlayer = 2;

%% 1. calculate the parameter matrix in low frequency range using exact
[~,~, ~,~, RmeshMF,LmeshMF] = main_mesh2d_cmplt( ...
    shp_id, pt_2d, dim1, dim2, Rpul, mur, len, frq_cal);


%% 2. calculate the parameter matrix at high freuqncy range using NN
[RmeshMF,LmeshMF] = main_mesh2d_nn_sub(RmeshMF,LmeshMF, ...
    shp_id, pt_2d, dim1, dim2, Rpul, mur, len, frq_cal, frq_nn, Nnlayer);


%% 3. generate self parameter vector and update magnetic meta
Nc = length(dim1);
Nf = length(frq);
Rself = zeros(Nc,Nf);
Lself = zeros(Nc,Nf);
for ik = 1:Nc
    Rself(ik,1:Nf) = squeeze(RmeshMF(ik,ik,:));
    Lself(ik,1:Nf) = squeeze(LmeshMF(ik,ik,:));
end

% update magnetic material using linear magnetic assuption
for ik = 1:Nc
    if find(mur(ik,:)>1)
        [Rskin, Lskin] = para_self_multi_frq(shp_id(ik), dim1(ik), dim2(ik), ...
            len(1), Rpul(ik), sig(ik), 1, frq);
        [Rstmp, Lstmp] = para_self_multi_frq(shp_id(ik), dim1(ik), dim2(ik), ...
            len(1), Rpul(ik), sig(ik), mur(ik,:), frq);
        
        Rcof = Rself(ik,1:Nf)./Rskin;
        Lcof = Lself(ik,1:Nf)./Lskin;
        
        Rself(ik,1:Nf) = Rcof.*Rstmp;
        Lself(ik,1:Nf) = Lcof.*Lstmp;
        
        RmeshMF(ik,ik,1:Nf) = Rself(ik,1:Nf);
        LmeshMF(ik,ik,1:Nf) = Lself(ik,1:Nf);
    end
end


%% 4. generate mutual parameter matrix on spicified frequency
% R value under 500Hz is perfered to simulate 10-350us case
fi = 500;

ind = find(frq == fi, 1);
Rmesh = zeros(Nc,Nc);

if ~isempty(ind)
    Rmesh = RmeshMF(:,:,ind);
elseif frq(Nf)<fi || Nf==1
    Rmesh = RmeshMF(:,:,Nf);
else
    for ik = 1:Nc
        for ig = 1:Nc
            Rmesh(ik,ig) = interp1(frq, squeeze(RmeshMF(ik,ig,:)), fi, 'linear');
        end
    end
end

Rmesh = diag(diag(Rmesh));

% % L value under 10kHz is perfered to simulate 8-20us case
fi = 10e3;

ind = find(frq == fi, 1);
Lmesh = zeros(Nc,Nc);

if ~isempty(ind)
    Lmesh = LmeshMF(:,:,ind);
elseif frq(Nf)<fi || Nf==1
    Lmesh = LmeshMF(:,:,Nf);
else
    for ik = 1:Nc
        for ig = 1:Nc
            Lmesh(ik,ig) = interp1(frq, squeeze(LmeshMF(ik,ig,:)), fi, 'linear');
        end
    end
end


timerEnd = toc(timerSta);
if timerEnd < 60
    fprintf ('Total Group calculation time using NN: %.2f sec\n', timerEnd);
elseif timerEnd>60
    fprintf ('Total Group calculation time using NN: %.2f min\n', timerEnd/60);
end


end




