function [Rmesh_pul, Lmesh_pul, Rself_pul, Lself_pul] = main_mesh2d_part_pul(...
    shape, pt2D, dim1, dim2, Rin_pul, Lin_pul, Sig, mur, freq)
%  Function:       mesh2d_main_partial_pul
%  Description:    For meshing model prefered for 10-350us time domain simulation,
%                  R is the resistance under 500Hz,
%                  L is the inductance under 10kHz.
%                  length=1m is used to normalize the parameters.
%                  In this version, the data must be rearranged by shape.
%                  First block -- round
%                  Second block -- rectangle
%                  Third block -- angle steel
%
%  Calls:          cal_RL_mesh2d_partial
%
%  Input:          pt_cs2D       --  point offset in 2D view (N*2) (m)
%                  dim1_cs2D     --  w/rout (N*1) (m)
%                  dim2_cs2D     --  t/rin (N*1) (m)
%                  Rin_pul_cs2D  --  resistance of conductors (N*1) (ohm/m)
%                  conduct_cs2D  --  conductivity of conductors(N*1)(Sig/m)
%  Output:         Rmtx_pul      --  R matrix per unit (ohm/m)
%                  Lmtx_pul      --  L matrix per unit (H/m)
%  Others:
%  Author:         Chen Hongcai
%  Email :         hc.chen@live.com
%  Date:           2014-4-18
%  History:


%% 1. classify the data based on shape
% In C++ part, type ID is defined as:
% DEVICE_TYPESIGN_CONDUCTOR_BCF = 1002;
% DEVICE_TYPESIGN_CONDUCTOR_BCL = 1003;

% in per unit length version, length is set to be 1
% update_mesh_para_on_length will be used in update later.
len_pul = 1;

indrec = shape == 1002;
indagi = shape == 1003;
indcir = ~(indrec|indagi);

Nrec = sum(indrec);
Nagi = sum(indagi);
Ncir = sum(indcir);
Nc = Nrec+Nagi+Ncir;


% #1 circular
p2Dcir=zeros(Ncir,2); rocir=zeros(Ncir,1); ricir=zeros(Ncir,1); 
Rpmcir=zeros(Ncir,1); murcir=zeros(Ncir,1); 
if Ncir > 0
    %     ind = (1:Ncir);
    p2Dcir = pt2D(1:Ncir,1:2);
    rocir = dim1(1:Ncir);
    ricir = dim2(1:Ncir);
    Rpmcir = Rin_pul(1:Ncir);
    murcir = mur(1:Ncir);
end

% #2 rectangle Ncir+(1:Nrec)
p2Drec=zeros(Nrec,2); wrec=zeros(Nrec,1); hrec=zeros(Nrec,1); 
Rpmrec=zeros(Nrec,1); Sigrec=zeros(Nrec,1); murrec=zeros(Nrec,1); 
if Nrec > 0
    %     ind = (Ncir+(1:Nrec));
    p2Drec = pt2D(Ncir+(1:Nrec),1:2);
    wrec = dim1(Ncir+(1:Nrec));
    hrec = dim2(Ncir+(1:Nrec));
    Rpmrec = Rin_pul(Ncir+(1:Nrec));
    Sigrec = Sig(Ncir+(1:Nrec));
    murrec = mur(Ncir+(1:Nrec));
end

% #3 angle iron Ncir+Nrec+(1:Nagi)
p2Dagi=zeros(Nagi,2); wagi=zeros(Nagi,1); tagi=zeros(Nagi,1); 
Rpmagi=zeros(Nagi,1); Sigagi=zeros(Nagi,1); muragi=zeros(Nagi,1); 
if Nagi > 0
    %     ind = (Ncir+Nrec+(1:Nagi));
    p2Dagi = pt2D(Ncir+Nrec+(1:Nagi),1:2);
    wagi = dim1(Ncir+Nrec+(1:Nagi));
    tagi = dim2(Ncir+Nrec+(1:Nagi));
    Rpmagi = Rin_pul(Ncir+Nrec+(1:Nagi));
    Sigagi = Sig(Ncir+Nrec+(1:Nagi));
    muragi = mur(Ncir+Nrec+(1:Nagi));
end

%% 2. calculate R and L, using meshing method in multi frequencies

tic

Nf = length(freq);
RmeshMF_pul = zeros(Nc,Nc,Nf);
LmeshMF_pul = zeros(Nc,Nc,Nf);

for ig = 1:Nf
    % R value under 500Hz is perfered to simulate 10-350us case
    %f0 = 500;
    [RmeshMF_pul(:,:,ig), LmeshMF_pul(:,:,ig)] = cal_RL_mesh2d_partial(...
        p2Dcir, rocir, ricir, Rpmcir, murcir, ...
        p2Drec, wrec, hrec, Rpmrec, Sigrec, murrec, ...
        p2Dagi, wagi, tagi, Rpmagi, Sigagi, muragi, len_pul, freq(ig));
    
end

%% 3. generate multi-frequency self parameter matrix

Rself_pul = zeros(Nc,Nf);
Lself_pul = zeros(Nc,Nf);
for ik = Nc
    Rself_pul(ik,1:Nf) = squeeze(RmeshMF_pul(ik,ik,:));
    Lself_pul(ik,1:Nf) = squeeze(LmeshMF_pul(ik,ik,:));
end


%% 4. generate mutual parameter matrix on spicified frequency
% R value under 500Hz is perfered to simulate 10-350us case
fi = 500;

ind = find(freq == fi, 1);
Rmesh_pul = zeros(Nc,Nc);

if ~isempty(ind)
    Rmesh_pul = RmeshMF_pul(:,:,ind);
elseif freq(Nf)<fi
    Rmesh_pul = RmeshMF_pul(:,:,Nf);
else
    for ik = 1:Nc
        for ig = 1:Nc
            Rmesh_pul(ik,ig) = interp1(freq, squeeze(RmeshMF_pul(ik,ig,:)), fi, 'linear');
        end
    end
end

% % L value under 10kHz is perfered to simulate 10-350us case
fi = 10e3;

ind = find(freq == fi, 1);
Lmesh_pul = zeros(Nc,Nc);

if ~isempty(ind)
    Lmesh_pul = LmeshMF_pul(:,:,ind);
elseif freq(Nf)<fi
    Lmesh_pul = LmeshMF_pul(:,:,Nf);
else
    for ik = 1:Nc
        for ig = 1:Nc
            Lmesh_pul(ik,ig) = interp1(freq, squeeze(LmeshMF_pul(ik,ig,:)), fi, 'linear');
        end
    end
end


disp('Group calculation time:')
toc

end




