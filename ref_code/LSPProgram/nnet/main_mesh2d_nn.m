function [Rmesh,Lmesh,Pmesh, Rself,Lself, RmeshMF,LmeshMF] = main_mesh2d_nn( ...
    shp_id, pt_2d, dim1, dim2, Rpul, sig,mur,epr, len, frq, flag_mth, flag_p)
%  Function:       main_mesh2d_nn
%  Description:    calculate cable group using NN method (for high frqeuncy)
%                  R is the resistance under 500Hz,
%                  L is the inductance under 20kHz.
%                  P is the coefficient of potential.
%                  In this version, the data must be rearranged by shape.
%                  1 block -- round
%                  2 block -- rectangle
%                  3 block -- angle steel
%                  4 block -- single pipe tower
%
%  Calls:          cal_RL_mesh2d_cmplt
%                  cal_P_mesh2d_cmplt
%                  main_mesh2d_nn_sub_rat
%                  main_mesh2d_nn_sub_ort
%                  main_mesh2d_nn_sub_val
%
%  Input:          shp_id   --  shape id
%                  pt_2d    --  point offset in 2D view (N*2) (m)
%                  dim1     --  w/rout (N*1) (m)
%                  dim2     --  t/rin (N*1) (m)
%                  Rpul     --  resistance of conductors (N*1) (ohm/m)
%                  sig      --  conductivity of conductors(N*1)(Sig/m)
%                  mur      --  relative permeability of conductors(N*1)
%                  len      --  length of the conductor
%                  frq      --  frequency
%                  flag_mth --  flag for which nn scheme is adopted.
%                               1. nn on the ratio of proximity and skin
%                               effects
%                               2. nn on the 
%                               3. nn on the value of the impedance.
%                  flag_p   --  flag for P calculation. P is calculated if >0
%  Output:         Rmesh    --  R matrix (N*N) (ohm)
%                  Lmesh    --  L matrix (N*N) (H)
%                  Pmesh    --  P matrix (N*N) (S)
%                  Rself    --  R matrix (N*Nf) (ohm)
%                  Lself    --  L matrix (N*Nf) (H)
%                  RmeshMF  --  R 3D matrix (N*N*Nf) (ohm)
%                  LmeshMF  --  L 3D matrix (N*N*Nf) (H)
%
%  Others:
%  Author:         Chen Hongcai
%  Email :         hc.chen@live.com
%  Date:           2017-03-25
%  Update:         Add P matrix calculation
%                  2018-06-03


fprintf ('Calculate the parameter matrix of the cable group (neural network mod).\n');

% fbreak = 100e3;
fbreak = 100e3;

frq_cal = frq(frq<=fbreak);
frq_nn = frq(frq>fbreak);
Nf = length(frq);
Nfcal = length(frq_cal);
Nc = size(pt_2d,1);
RmeshMF = zeros(Nc,Nc,Nf);
LmeshMF = zeros(Nc,Nc,Nf);


timerSta = tic;

%% 1. calculate the parameter matrix in low frequency range using exact
[~,~, Pmesh,~, ~, RmeshMF(:,:,1:Nfcal),LmeshMF(:,:,1:Nfcal)] = main_mesh2d_cmplt( ...
    shp_id, pt_2d, dim1, dim2, Rpul, sig,mur,epr, len, frq_cal, flag_p);


%% 2. calculate the parameter matrix at high freuqncy range using NN
if flag_mth == 1
    Nnlayer = 2;

    [RmeshMF,LmeshMF] = main_mesh2d_nn_sub_rat(RmeshMF,LmeshMF, ...
        shp_id, pt_2d, dim1, dim2, Rpul, sig,mur, len, frq_cal, frq_nn, Nnlayer);
    
elseif flag_mth == 2
    Nnlayer = 2;
    
    [RmeshMF,LmeshMF] = main_mesh2d_nn_sub_ort(RmeshMF,LmeshMF, ...
        shp_id, pt_2d, dim1, dim2, Rpul, sig,mur, len, frq_cal, frq_nn, Nnlayer);
    
elseif flag_mth == 3
    Nnlayer = 2;
    [RmeshMF,LmeshMF] = main_mesh2d_nn_sub_val(RmeshMF,LmeshMF, frq_cal, frq_nn, Nnlayer);
end


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
        
        
%         Rcof = Rself(ik,1:Nf)./Rskin;
%         Lcof = Lself(ik,1:Nf)./Lskin;
%         Rself(ik,1:Nf) = Rcof.*Rstmp;
%         Lself(ik,1:Nf) = Lcof.*Lstmp;
        
        Rself(ik,1:Nf) = Rself(ik,1:Nf)-Rskin+Rstmp;
        Lself(ik,1:Nf) = Lself(ik,1:Nf)-Lskin+Lstmp;
        
        RmeshMF(ik,ik,1:Nf) = Rself(ik,1:Nf);
        LmeshMF(ik,ik,1:Nf) = Lself(ik,1:Nf);
    end
end


%% 4. generate mutual parameter matrix on spicified frequency
% R value under 500Hz is perfered to simulate 10-350us case
if max(frq) <= 250e3
    fi = 500;
elseif max(frq)>=250e3 && max(frq)<1e6
    fi = 5e3;
elseif max(frq)>=1e6
    fi = 20e3;
end

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
if max(frq) <= 250e3
    fi = 20e3;
elseif max(frq)>=250e3 && max(frq)<1e6
    fi = 50e3;
elseif max(frq)>=1e6
    fi = 100e3;
end


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

%% 5. calculate the P matrix using meshing method
% if flag_p > 0
%     Pmesh = cal_RL_mesh2d_cmplt(p2Dcir,rocir,ricir,  p2Drec,wrec,hrec, ...
%         p2Dagi,wagi,tagi,  p2Dspt,rospt,rispt,  len(1));
%     
%     Pmesh = ind_mtx*Pmesh*ind_mtx';
% else
%     Pmesh = [];
% end



timerEnd = toc(timerSta);
if timerEnd < 60
    fprintf ('Total Group calculation time using NN: %.2f sec\n', timerEnd);
elseif timerEnd>60
    fprintf ('Total Group calculation time using NN: %.2f min\n', timerEnd/60);
end



end




