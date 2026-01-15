function [Rmesh_pul, Lmesh_pul, Rself_pul, Lself_pul] = main_mesh2d_cmplt_pul(...
    shape, pt2D, dim1, dim2, Rin_pul, sig,mur, freq)
%  Function:       para_group_mesh2D
%  Description:    For meshing model prefered for 10-350us time domain simulation,
%                  R is the resistance under 500Hz,
%                  L is the inductance under 10kHz.
%                  length=10m is used to normalize the parameters.
%                  In this version, the data must be rearranged by shape.
%                  First block -- round
%                  Second block -- rectangle
%                  Third block -- angle steel
%
%  Calls:          cal_RL_mesh2D
%
%  Input:          pt2D     --  point offset in 2D view (N*2) (m)
%                  dim1     --  w/rout (N*1) (m)
%                  dim2     --  t/rin (N*1) (m)
%                  Rin_pul  --  resistance of conductors (N*1) (ohm/m)
%                  Lin_pul
%                  Sig      --  conductivity of conductors(N*1)(Sig/m)
%                  
%  Output:         Rmtx_pul      --  R matrix per unit (ohm/m)
%                  Lmtx_pul      --  L matrix per unit (H/m)
%  Others:
%  Author:         Chen Hongcai
%  Email :         hc.chen@live.com
%  Date:           2014-4-18
%  History:

fprintf ('Calculate the parameter matrix of the cable group (p.u.l mod).\n');

len_pul = 1;

%% 1. classify the data based on shape
% In C++ part, type ID is defined as:
% DEVICE_TYPESIGN_CONDUCTOR_BCF = 1002;
% DEVICE_TYPESIGN_CONDUCTOR_BCL = 1003;

indrec = shape == 1002;
indagi = shape == 1003;
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
    p2Dcir = pt2D(1:Ncir,1:2);
    rocir = dim1(1:Ncir);
    ricir = dim2(1:Ncir);
    Rpmcir = Rin_pul(1:Ncir);
    murcir = mur(1:Ncir);
    %sig(1:Ncir)  = 1./(Rpmcir.*(pi*(rocir.^2-ricir.^2)));
end


% #2 rectangle Ncir+(1:Nrec)
p2Drec=zeros(Nrec,2); wrec=zeros(Nrec,1); hrec=zeros(Nrec,1); 
Rpmrec=zeros(Nrec,1); murrec=zeros(Nrec,1); 
if Nrec > 0
    %     ind = (Ncir+(1:Nrec));
    p2Drec = pt2D(Ncir+(1:Nrec),1:2);
    wrec = dim1(Ncir+(1:Nrec));
    hrec = dim2(Ncir+(1:Nrec));
    Rpmrec = Rin_pul(Ncir+(1:Nrec));
    murrec = mur(Ncir+(1:Nrec));
    %sig(Ncir+(1:Nrec))  = 1./abs(Rpmrec*(wrec.*hrec));
end

% #3 angle iron Ncir+Nrec+(1:Nagi)
p2Dagi=zeros(Nagi,2); wagi=zeros(Nagi,1); tagi=zeros(Nagi,1); 
Rpmagi=zeros(Nagi,1); muragi=zeros(Nagi,1); 
if Nagi > 0
    %     ind = (Ncir+Nrec+(1:Nagi));
    p2Dagi = pt2D(Ncir+Nrec+(1:Nagi),1:2);
    wagi = dim1(Ncir+Nrec+(1:Nagi));
    tagi = dim2(Ncir+Nrec+(1:Nagi));
    Rpmagi = Rin_pul(Ncir+Nrec+(1:Nagi));
    muragi = mur(Ncir+Nrec+(1:Nagi));
    %sig(Ncir+Nrec+(1:Nagi))  = 1./(Rpmagi.*(abs(2*wagi.*tagi)-tagi.^2));
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

Nf = length(freq);
RmeshMF_pul = zeros(Nc,Nc,Nf);
LmeshMF_pul = zeros(Nc,Nc,Nf);

for ig = 1:Nf

    [RmeshMF_pul(:,:,ig), LmeshMF_pul(:,:,ig)] = cal_RL_mesh2d_cmplt(...
        p2Dcir, rocir, ricir, Rpmcir, murcir, ...
        p2Drec, wrec, hrec, Rpmrec, murrec, ...
        p2Dagi, wagi, tagi, Rpmagi, muragi, ...
        p2Dspt, rospt, rispt, Rpmspt, murspt, len_pul, frq(ig));
    
end

%% 3. generate self parameter vector and update magnetic meta
Rself_pul = zeros(Nc,Nf);
Lself_pul = zeros(Nc,Nf);
for ik = Nc
    Rself_pul(ik,1:Nf) = squeeze(RmeshMF_pul(ik,ik,:));
    Lself_pul(ik,1:Nf) = squeeze(LmeshMF_pul(ik,ik,:));
end

% update magnetic material using linear magnetic assuption
for ik = 1:Nc
    if find(mur(ik,:)>1)
        [Rskin, Lskin] = para_self_multi_frq(shape(ik), dim1(ik), dim2(ik), ...
            len(1), R_pul(ik), sig(ik), 1, frq);
        [Rstmp, Lstmp] = para_self_multi_frq(shape(ik), dim1(ik), dim2(ik), ...
            len(1), R_pul(ik), sig(ik), mur(ik,:), frq);
        
%         Rcof = Rself(ik,1:Nf)./Rskin;
%         Lcof = Lself(ik,1:Nf)./Lskin; 
%         Rself_pul(ik,1:Nf) = Rcof.*Rstmp;
%         Lself_pul(ik,1:Nf) = Lcof.*Lstmp;
        
        dR = Rstmp-Rskin;
        dL = Lstmp-Lskin;
        
        Rself_pul(ik,1:Nf) = Rself_pul(ik,1:Nf)+dR;
        Lself_pul(ik,1:Nf) = Lself_pul(ik,1:Nf)+dL;
        
        RmeshMF_pul(ik,ik,1:Nf) = Rself_pul(ik,1:Nf);
        LmeshMF_pul(ik,ik,1:Nf) = Lself_pul(ik,1:Nf);
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

Rmesh_pul = diag(diag(Rmesh_pul));


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


timerEnd = toc(timerSat);
if timerEnd < 60
    fprintf ('Group calculation time: %.2f sec\n', timerEnd);
elseif timerEnd>60
    fprintf ('Group calculation time: %.2f min\n', timerEnd/60);
end


end




