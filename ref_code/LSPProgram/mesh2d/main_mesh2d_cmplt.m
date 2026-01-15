function [Rmesh,Lmesh,Pmesh, Rself,Lself, RmeshMF,LmeshMF] = main_mesh2d_cmplt( ...
    shp_id, pt_2D, dim1, dim2, Rpul, sig,mur,epr, len, frq, flag_p)
%  Function:       main_mesh2d_cmplt
%  Description:    For meshing model prefered for 8-20us time domain simulation,
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
%
%  Input:          shp_id  --  shape id of the conductor
%                  pt_2D   --  point offset in 2D view (N*2) (m)
%                  dim1    --  w/rout (N*1) (m)
%                  dim2    --  t/rin (N*1) (m)
%                  Rpul    --  resistance of conductors (N*1) (ohm/m)
%                  sig     --  conductivity of conductors(N*1)(Sig/m)
%                  mur     --  relative permeability of conductors(N*1)
%                  len     --  length of the conductor
%                  frq     --  frequency
%                  flag_p  --  flag for P calculation. P is calculated if >0
%
%  Output:         Rmesh     --  R matrix (N*N) (ohm)
%                  Lmesh     --  L matrix (N*N) (H)
%                  Rself     --  R matrix (N*Nf) (ohm)
%                  Lself     --  L matrix (N*Nf) (H)
%                  Pmesh     --  P matrix (N*N) (S)
%                  RmeshMF   --  R 3D matrix (N*N*Nf) (ohm)
%                  LmeshMF   --  L 3D matrix (N*N*Nf) (H)
%
%  Others:
%  Author:         Chen Hongcai
%  Email :         hc.chen@live.com
%  Date:           2014-4-18
%  Update:         Add magnetic meta update part
%                  2016-10-17
%                  Add P matrix calculation
%                  2018-06-03

% flag_p = 1;  % SPT is not well supported for capacitance calculation

if nargin < 11
    flag_p = 0;
end

fprintf ('Calculate the parameter matrix of the cable group (complete mod).\n');

%% 1. classify the data based on shape
% In C++ part, type ID is defined as:
% DEVICE_TYPESIGN_CONDUCTOR_BCF = 1002;
% DEVICE_TYPESIGN_CONDUCTOR_BCL = 1003;
% DEVICE_TYPESIGN_CONDUCTOR_SPT = 1100;

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
Rpmcir=zeros(Ncir,1); murcir=zeros(Ncir,1); eprcir=zeros(Ncir,1);idcir_mtx = zeros(Nc, Ncir);
if Ncir > 0
    %     ind = (1:Ncir);
    p2Dcir = pt_2D(indcir,1:2);
    rocir = dim1(indcir);
    ricir = dim2(indcir);
    Rpmcir = Rpul(indcir);
    murcir = mur(indcir,:);
    eprcir = epr(indcir,:);
    %sig(1:Ncir)  = 1./(Rpmcir.*(pi*(rocir.^2-ricir.^2)));
    idtmp = find(indcir==1);
    for ik = 1:Ncir
        idcir_mtx(idtmp(ik),ik) = 1;
    end
end

% #2 rectangle Ncir+(1:Nrec)
p2Drec=zeros(Nrec,2); wrec=zeros(Nrec,1); hrec=zeros(Nrec,1);
Rpmrec=zeros(Nrec,1); murrec=zeros(Nrec,1); eprrec=zeros(Nrec,1);idrec_mtx = zeros(Nc, Nrec);
if Nrec > 0
    %     ind = (Ncir+(1:Nrec));
    p2Drec = pt_2D(indrec,1:2);
    wrec = dim1(indrec);
    hrec = dim2(indrec);
    Rpmrec = Rpul(indrec);
    murrec = mur(indrec,:);
    eprrec = epr(indrec,:);
    %sig(Ncir+(1:Nrec))  = abs(1./(Rpmrec*(wrec.*hrec)));
    idtmp = find(indrec==1);
    for ik = 1:Nrec
        idrec_mtx(idtmp(ik),ik) = 1;
    end
end

% #3 angle iron Ncir+Nrec+(1:Nagi)
p2Dagi=zeros(Nagi,2); wagi=zeros(Nagi,1); tagi=zeros(Nagi,1);
Rpmagi=zeros(Nagi,1); muragi=zeros(Nagi,1); epragi=zeros(Nagi,1);idagi_mtx = zeros(Nc, Nagi);
if Nagi > 0
    %     ind = (Ncir+Nrec+(1:Nagi));
    p2Dagi = pt_2D(indagi,1:2);
    wagi = dim1(indagi);
    tagi = dim2(indagi);
    Rpmagi = Rpul(indagi);
    muragi = mur(indagi,:);
    epragi = mur(indagi,:);
    %sig(Ncir+Nrec+(1:Nagi))  = 1./(Rpmagi.*(2*abs(wagi.*tagi)-tagi.^2));
    idtmp = find(indagi==1);
    for ik = 1:Nagi
        idagi_mtx(idtmp(ik),ik) = 1;
    end
end

% #4 single tube tower Ncir+Nrec+Nagi+(1:Nspt)
p2Dspt=zeros(Nspt,2); rospt=zeros(Nspt,1); rispt=zeros(Nspt,1);
Rpmspt=zeros(Nspt,1); murspt=zeros(Nspt,1); eprspt=zeros(Nspt,1);idspt_mtx = zeros(Nc, Nspt);
if Nspt > 0
    %     ind = (Ncir+Nrec+(1:Nagi));
    p2Dspt = pt_2D(indspt,1:2);
    rospt = dim1(indspt);
    rispt = dim2(indspt);
    Rpmspt = Rpul(indspt);
    murspt = mur(indspt,:);
    eprspt = epr(indspt,:);
    %sig(Ncir+Nrec+(1:Nagi))  = 1./(Rpmagi.*(2*abs(wagi.*tagi)-tagi.^2));
    idtmp = find(indspt==1);
    for ik = 1:Nspt
        idspt_mtx(idtmp(ik),ik) = 1;
    end
end

% order matrix for condcutors
ind_mtx = ([idcir_mtx, idrec_mtx, idagi_mtx, idspt_mtx]);

%% 2. calculate R and L, using meshing method in multi frequencies
timerSat = tic;

Nf = length(frq);
RmeshMF = zeros(Nc,Nc,Nf);
LmeshMF = zeros(Nc,Nc,Nf);

for ig = 1:Nf
    [RmeshMF(:,:,ig), LmeshMF(:,:,ig)] = cal_RL_mesh2d_cmplt(...
        p2Dcir, rocir, ricir, Rpmcir, murcir, eprcir, ...
        p2Drec, wrec, hrec, Rpmrec, murrec, eprrec, ...
        p2Dagi, wagi, tagi, Rpmagi, muragi, epragi, ...
        p2Dspt, rospt, rispt, Rpmspt, murspt, eprspt, len(1), frq(ig));
end

% update the matrix using the order of conductors
for ig = 1:Nf
    RmeshMF(:,:,ig) = ind_mtx*RmeshMF(:,:,ig)*ind_mtx';
    LmeshMF(:,:,ig) = ind_mtx*LmeshMF(:,:,ig)*ind_mtx';
end


%% 3. generate self parameter vector and update magnetic meta
Rself = zeros(Nc,Nf);
Lself = zeros(Nc,Nf);
for ik = 1:Nc
    Rself(ik,1:Nf) = squeeze(RmeshMF(ik,ik,:));
    Lself(ik,1:Nf) = squeeze(LmeshMF(ik,ik,:));
end

% update magnetic material using linear magnetic assuption
% for ik = 1:Nc
%     if find(mur(ik,:)>1)
%         [Rskin, Lskin] = para_self_multi_frq(shp_id(ik), dim1(ik), dim2(ik), ...
%             len(1), Rpul(ik), sig(ik), 1, frq);
%         [Rstmp, Lstmp] = para_self_multi_frq(shp_id(ik), dim1(ik), dim2(ik), ...
%             len(1), Rpul(ik), sig(ik), mur(ik,:), frq);
%         
% %         Rcof = Rself(ik,1:Nf)./Rskin;
% %         Lcof = Lself(ik,1:Nf)./Lskin;
% %         Rself(ik,1:Nf) = Rcof.*Rstmp;
% %         Lself(ik,1:Nf) = Lcof.*Lstmp;
%         
%         dR = Rstmp-Rskin;
%         dL = Lstmp-Lskin;
%         
%         Rself(ik,1:Nf) = Rself(ik,1:Nf)+dR;
%         Lself(ik,1:Nf) = Lself(ik,1:Nf)+dL;
%         
%         RmeshMF(ik,ik,1:Nf) = Rself(ik,1:Nf);
%         LmeshMF(ik,ik,1:Nf) = Lself(ik,1:Nf);
%     end
% end

% check if zero resistance exsit (it may be appear for large conductors)
for ik = 1:Nc
    if find(RmeshMF(ik,ik,1:Nf)<0)
        [Rskin, ~] = para_self_multi_frq(shp_id(ik), dim1(ik), dim2(ik), ...
            len(1), Rpul(ik), sig(ik), 1, frq);
        RmeshMF(ik,ik,1:Nf) = Rskin;
    end
end
        

%% 4. generate mutual parameter matrix on spicified frequency
% R value under 500Hz is perfered to simulate 10-350us case
if max(frq) <= 250e3
    fi = 500;
elseif max(frq)>=250e3 && max(frq)<1e6
    fi = 5e3;
elseif max(frq)>=1e6 && max(frq)<2e6
    fi = 20e3;
elseif max(frq)>=2e6 && max(frq)<5e6
    fi = 50e3;
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


%% 5. calculate the P matrix using meshing method
if flag_p > 0
    Pmesh = cal_P_mesh2d_cmplt(p2Dcir,rocir,ricir, eprcir, p2Drec,wrec,hrec, eprrec, ...
        p2Dagi,wagi,tagi, epragi, p2Dspt,rospt,rispt, eprspt, len(1));
    
    Pmesh = ind_mtx*Pmesh*ind_mtx';
else
    Pmesh = [];
end


timerEnd = toc(timerSat);
if timerEnd < 60
    fprintf ('Group calculation time: %.2f sec\n', timerEnd);
elseif timerEnd>60
    fprintf ('Group calculation time: %.2f min\n', timerEnd/60);
end


%fprintf ('\n')


end




