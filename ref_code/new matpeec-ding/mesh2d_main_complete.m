function [Rmesh,Lmesh, Rself,Lself, RmeshMF,LmeshMF] = mesh2d_main_complete( ...
    shape, pt2D, dim1, dim2, R_pul, Lin_pul, mur, len, frq)
%  Function:       mesh2d_main_complete
%  Description:    For meshing model prefered for 8-20us time domain simulation,
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
%  Date:           2014-4-18
%  History:        Add magnetic meta update part
%                  2016-10-17


fprintf ('Calculate the parameter matrix of the cable group (complete mod).\n');

%% 1. classify the data based on shape
% In C++ part, type ID is defined as:
% DEVICE_TYPESIGN_CONDUCTOR_BCF = 1002;
% DEVICE_TYPESIGN_CONDUCTOR_BCL = 1003;

indrec = shape == 1002;
indagi = shape == 1003;
indcir = ~(indrec|indagi);

Nrec = sum(indrec);
Nagi = sum(indagi);
Ncir = sum(indcir);
Nc = Nrec+Nagi+Ncir;

sig = zeros(Nc,1);

% #1 circular
p2Dcir=zeros(Ncir,2); rocir=zeros(Ncir,1); ricir=zeros(Ncir,1);
Rpmcir=zeros(Ncir,1); murcir=zeros(Ncir,1);
if Ncir > 0
    %     ind = (1:Ncir);
    p2Dcir = pt2D(1:Ncir,1:2);
    rocir = dim1(1:Ncir);
    ricir = dim2(1:Ncir);
    Rpmcir = R_pul(1:Ncir);
    murcir = mur(1:Ncir,:);
    sig(1:Ncir)  = 1./(Rpmcir.*(pi*(rocir.^2-ricir.^2)));
end

% #2 rectangle Ncir+(1:Nrec)
p2Drec=zeros(Nrec,2); wrec=zeros(Nrec,1); hrec=zeros(Nrec,1);
Rpmrec=zeros(Nrec,1); murrec=zeros(Nrec,1);
if Nrec > 0
    %     ind = (Ncir+(1:Nrec));
    p2Drec = pt2D(Ncir+(1:Nrec),1:2);
    wrec = dim1(Ncir+(1:Nrec));
    hrec = dim2(Ncir+(1:Nrec));
    Rpmrec = R_pul(Ncir+(1:Nrec));
    murrec = mur(Ncir+(1:Nrec),:);
    sig(Ncir+(1:Nrec))  = 1./(Rpmrec*(wrec.*hrec));
end

% #3 angle iron Ncir+Nrec+(1:Nagi)
p2Dagi=zeros(Nagi,2); wagi=zeros(Nagi,1); tagi=zeros(Nagi,1);
Rpmagi=zeros(Nagi,1); muragi=zeros(Nagi,1);
if Nagi > 0
    %     ind = (Ncir+Nrec+(1:Nagi));
    p2Dagi = pt2D(Ncir+Nrec+(1:Nagi),1:2);
    wagi = dim1(Ncir+Nrec+(1:Nagi));
    tagi = dim2(Ncir+Nrec+(1:Nagi));
    Rpmagi = R_pul(Ncir+Nrec+(1:Nagi));
    muragi = mur(Ncir+Nrec+(1:Nagi),:);
    sig(Ncir+Nrec+(1:Nagi))  = 1./(Rpmagi.*(2*wagi.*tagi-tagi.^2));
end


%% 2. calculate R and L, using meshing method in multi frequencies
tic

Nf = length(frq);
RmeshMF = zeros(Nc,Nc,Nf);
LmeshMF = zeros(Nc,Nc,Nf);

for ig = 1:Nf
    
    [RmeshMF(:,:,ig), LmeshMF(:,:,ig)] = cal_RL_mesh2d_complete(...
        p2Dcir, rocir, ricir, Rpmcir, murcir, ...
        p2Drec, wrec, hrec, Rpmrec, murrec, ...
        p2Dagi, wagi, tagi, Rpmagi, muragi, len(1), frq(ig));
    
end


%% 3. generate self parameter vector and update magnetic meta
Rself = zeros(Nc,Nf);
Lself = zeros(Nc,Nf);
for ik = 1:Nc
    Rself(ik,1:Nf) = squeeze(RmeshMF(ik,ik,:));
    Lself(ik,1:Nf) = squeeze(LmeshMF(ik,ik,:));
end

% update magnetic material using linear magnetic assuption
for ik = 1:Nc
    if find(mur(ik,:)>1)
        [Rskin, Lskin] = para_self_multi_frq(shape(ik), dim1(ik), dim2(ik), ...
            len(1), R_pul(ik), sig(ik), 1, frq);
        [Rstmp, Lstmp] = para_self_multi_frq(shape(ik), dim1(ik), dim2(ik), ...
            len(1), R_pul(ik), sig(ik), mur(ik,:), frq);
        
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
elseif frq(Nf)<fi
    Rmesh = RmeshMF(:,:,Nf);
else
    for ik = 1:Nc
        for ig = 1:Nc
            Rmesh(ik,ig) = interp1(frq, squeeze(RmeshMF(ik,ig,:)), fi, 'linear');
        end
    end
end

Rmesh = diag(diag(Rmesh));

% % L value under 20kHz is perfered to simulate 8-20us case
fi = 20e3;

ind = find(frq == fi, 1);
Lmesh = zeros(Nc,Nc);

if ~isempty(ind)
    Lmesh = LmeshMF(:,:,ind);
elseif frq(Nf)<fi
    Lmesh = LmeshMF(:,:,Nf);
else
    for ik = 1:Nc
        for ig = 1:Nc
            Lmesh(ik,ig) = interp1(frq, squeeze(LmeshMF(ik,ig,:)), fi, 'linear');
        end
    end
end


fprintf ('Group calculation time: ')
toc
%fprintf ('\n')


end




