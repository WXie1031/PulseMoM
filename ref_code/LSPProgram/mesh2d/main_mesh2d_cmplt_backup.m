function [Rmesh,Lmesh, Rself,Lself, RmeshMF,LmeshMF] = main_mesh2d_cmplt_backup( ...
    shp_id, pt_2D, dim1, dim2, Rpul, sig,mur, len, frq)
%  Function:       main_mesh2d_cmplt
%  Description:    For meshing model prefered for 8-20us time domain simulation,
%                  R is the resistance under 500Hz,
%                  L is the inductance under 20kHz.
%                  In this version, the data must be rearranged by shape.
%                  First block -- round
%                  Second block -- rectangle
%                  Third block -- angle steel
%
%  Calls:          cal_RL_mesh2d_cmplt
%
%  Input:          shp_id  --  shape id of the conductor
%                  pt_2D   --  point offset in 2D view (N*2) (m)
%                  dim1    --  w/rout (N*1) (m)
%                  dim2    --  t/rin (N*1) (m)
%                  Rpul    --  resistance of conductors (N*1) (ohm/m)
%                  sig     --  conductivity of conductors(N*1)(Sig/m)
%                  mur     --  relative permeability of conductors(N*1)
%                  len     --  length of the conductor
%
%  Output:         Rmesh     --  R matrix (N*N) (ohm)
%                  Lmesh     --  L matrix (N*N) (H)
%                  Rself     --  R matrix (N*Nf) (ohm)
%                  Lself     --  L matrix (N*Nf) (H)
%                  RmeshMF   --  R 3D matrix (N*N*Nf) (ohm)
%                  LmeshMF   --  L 3D matrix (N*N*Nf) (H)
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

indrec = shp_id == 1002;
indagi = shp_id == 1003;
indspt = shp_id == 1100;
indcir = ~(indrec|indagi|indspt);

Nrec = sum(indrec);
Nagi = sum(indagi);
Ncir = sum(indcir);
Nspt = sum(indspt);
Nc = Nrec+Nagi+Ncir+Nspt;

%sig = zeros(Nc,1);

% #1 circular
p2Dcir=zeros(Ncir,2); rocir=zeros(Ncir,1); ricir=zeros(Ncir,1);
Rpmcir=zeros(Ncir,1); murcir=zeros(Ncir,1);
if Ncir > 0
    %     ind = (1:Ncir);
    p2Dcir = pt_2D(1:Ncir,1:2);
    rocir = dim1(1:Ncir);
    ricir = dim2(1:Ncir);
    Rpmcir = Rpul(1:Ncir);
    murcir = mur(1:Ncir,:);
    %sig(1:Ncir)  = 1./(Rpmcir.*(pi*(rocir.^2-ricir.^2)));
end

% #2 rectangle Ncir+(1:Nrec)
p2Drec=zeros(Nrec,2); wrec=zeros(Nrec,1); hrec=zeros(Nrec,1);
Rpmrec=zeros(Nrec,1); murrec=zeros(Nrec,1);
if Nrec > 0
    %     ind = (Ncir+(1:Nrec));
    p2Drec = pt_2D(Ncir+(1:Nrec),1:2);
    wrec = dim1(Ncir+(1:Nrec));
    hrec = dim2(Ncir+(1:Nrec));
    Rpmrec = Rpul(Ncir+(1:Nrec));
    murrec = mur(Ncir+(1:Nrec),:);
    %sig(Ncir+(1:Nrec))  = abs(1./(Rpmrec*(wrec.*hrec)));
end

% #3 angle iron Ncir+Nrec+(1:Nagi)
p2Dagi=zeros(Nagi,2); wagi=zeros(Nagi,1); tagi=zeros(Nagi,1);
Rpmagi=zeros(Nagi,1); muragi=zeros(Nagi,1);
if Nagi > 0
    %     ind = (Ncir+Nrec+(1:Nagi));
    p2Dagi = pt_2D(Ncir+Nrec+(1:Nagi),1:2);
    wagi = dim1(Ncir+Nrec+(1:Nagi));
    tagi = dim2(Ncir+Nrec+(1:Nagi));
    Rpmagi = Rpul(Ncir+Nrec+(1:Nagi));
    muragi = mur(Ncir+Nrec+(1:Nagi),:);
    %sig(Ncir+Nrec+(1:Nagi))  = 1./(Rpmagi.*(2*abs(wagi.*tagi)-tagi.^2));
end

% #4 single tube tower Ncir+Nrec+Nagi+(1:Nspt)
p2Dspt=zeros(Nspt,2); rospt=zeros(Nspt,1); rispt=zeros(Nspt,1);
Rpmspt=zeros(Nspt,1); murspt=zeros(Nspt,1);
if Nspt > 0
    %     ind = (Ncir+Nrec+(1:Nagi));
    p2Dspt = pt_2D(Ncir+Nrec+Nagi+(1:Nspt),1:2);
    rospt = dim1(Ncir+Nrec+Nagi+(1:Nspt));
    rispt = dim2(Ncir+Nrec+Nagi+(1:Nspt));
    Rpmspt = Rpul(Ncir+Nrec+Nagi+(1:Nspt));
    murspt = mur(Ncir+Nrec+Nagi+(1:Nspt),:);
    %sig(Ncir+Nrec+(1:Nagi))  = 1./(Rpmagi.*(2*abs(wagi.*tagi)-tagi.^2));
end




%% 2. calculate R and L, using meshing method in multi frequencies
timerSat = tic;

Nf = length(frq);
RmeshMF = zeros(Nc,Nc,Nf);
LmeshMF = zeros(Nc,Nc,Nf);

for ig = 1:Nf

    [RmeshMF(:,:,ig), LmeshMF(:,:,ig)] = cal_RL_mesh2d_cmplt(...
        p2Dcir, rocir, ricir, Rpmcir, murcir, ...
        p2Drec, wrec, hrec, Rpmrec, murrec, ...
        p2Dagi, wagi, tagi, Rpmagi, muragi, ...
        p2Dspt, rospt, rispt, Rpmspt, murspt, len(1), frq(ig));
    
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
        [Rskin, Lskin] = para_self_multi_frq(shp_id(ik), dim1(ik), dim2(ik), ...
            len(1), Rpul(ik), sig(ik), 1, frq);
        [Rstmp, Lstmp] = para_self_multi_frq(shp_id(ik), dim1(ik), dim2(ik), ...
            len(1), Rpul(ik), sig(ik), mur(ik,:), frq);
        
%         Rcof = Rself(ik,1:Nf)./Rskin;
%         Lcof = Lself(ik,1:Nf)./Lskin;
%         Rself(ik,1:Nf) = Rcof.*Rstmp;
%         Lself(ik,1:Nf) = Lcof.*Lstmp;
        
        dR = Rstmp-Rskin;
        dL = Lstmp-Lskin;
        
        Rself(ik,1:Nf) = Rself(ik,1:Nf)+dR;
        Lself(ik,1:Nf) = Lself(ik,1:Nf)+dL;
        
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
else
    fi = max(frq)/50;
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
elseif max(frq)>=1e6 && max(frq)<2e6
    fi = 100e3;
elseif max(frq)>=2e6 && max(frq)<5e6
    fi = 200e3;
else
    fi = max(frq)/10;
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


timerEnd = toc(timerSat);
if timerEnd < 60
    fprintf ('Group calculation time: %.2f sec\n', timerEnd);
elseif timerEnd>60
    fprintf ('Group calculation time: %.2f min\n', timerEnd/60);
end


%fprintf ('\n')


end




