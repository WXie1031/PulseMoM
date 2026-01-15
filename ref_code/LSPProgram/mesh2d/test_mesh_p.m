%% 1. configuratoin
frq = 20e3;

shp_id = [1100; 2100; 2100; 1002; 1002;];
pt_2D = [0 0; 0 460; 0 480; 0 0; 0 -300;]*1e-3;
dim1 = [500; 5; 5; 40;  40]*1e-3;
dim2 = [490; 0; 0; 4;  4;]*1e-3;
sig = [1.5e6;   5.8e7;  5.8e7; 5.8e7;   1e6;];

shp_id = [1100; 2100; 2100;];
pt_2D = [0 0; 0 460; 0 480; ]*1e-3;
dim1 = [500; 5; 5; ]*1e-3;
dim2 = [490; 0; 0; ]*1e-3;
sig = [1.5e6; 5.8e7; 5.8e7;];


S = pi*(dim1.^2-dim2.^2);
Rpul = 1./(sig.*S);

mur = [1;1;1;];


len = 1;

%% 2. data arrangment
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
Rpmcir=zeros(Ncir,1); murcir=zeros(Ncir,1); idcir_mtx = zeros(Nc, Ncir);
if Ncir > 0
    %     ind = (1:Ncir);
    p2Dcir = pt_2D(indcir,1:2);
    rocir = dim1(indcir);
    ricir = dim2(indcir);
    Rpmcir = Rpul(indcir);
    murcir = mur(indcir,:);
    %sig(1:Ncir)  = 1./(Rpmcir.*(pi*(rocir.^2-ricir.^2)));
    idtmp = find(indcir==1);
    for ik = 1:Ncir
        idcir_mtx(idtmp(ik),ik) = 1;
    end
end

% #2 rectangle Ncir+(1:Nrec)
p2Drec=zeros(Nrec,2); wrec=zeros(Nrec,1); hrec=zeros(Nrec,1);
Rpmrec=zeros(Nrec,1); murrec=zeros(Nrec,1); idrec_mtx = zeros(Nc, Nrec);
if Nrec > 0
    %     ind = (Ncir+(1:Nrec));
    p2Drec = pt_2D(indrec,1:2);
    wrec = dim1(indrec);
    hrec = dim2(indrec);
    Rpmrec = Rpul(indrec);
    murrec = mur(indrec,:);
    %sig(Ncir+(1:Nrec))  = abs(1./(Rpmrec*(wrec.*hrec)));
    idtmp = find(indrec==1);
    for ik = 1:Nrec
        idrec_mtx(idtmp(ik),ik) = 1;
    end
end

% #3 angle iron Ncir+Nrec+(1:Nagi)
p2Dagi=zeros(Nagi,2); wagi=zeros(Nagi,1); tagi=zeros(Nagi,1);
Rpmagi=zeros(Nagi,1); muragi=zeros(Nagi,1); idagi_mtx = zeros(Nc, Nagi);
if Nagi > 0
    %     ind = (Ncir+Nrec+(1:Nagi));
    p2Dagi = pt_2D(indagi,1:2);
    wagi = dim1(indagi);
    tagi = dim2(indagi);
    Rpmagi = Rpul(indagi);
    muragi = mur(indagi,:);
    %sig(Ncir+Nrec+(1:Nagi))  = 1./(Rpmagi.*(2*abs(wagi.*tagi)-tagi.^2));
    idtmp = find(indagi==1);
    for ik = 1:Nagi
        idagi_mtx(idtmp(ik),ik) = 1;
    end
end

% #4 single tube tower Ncir+Nrec+Nagi+(1:Nspt)
p2Dspt=zeros(Nspt,2); rospt=zeros(Nspt,1); rispt=zeros(Nspt,1);
Rpmspt=zeros(Nspt,1); murspt=zeros(Nspt,1); idspt_mtx = zeros(Nc, Nspt);
if Nspt > 0
    %     ind = (Ncir+Nrec+(1:Nagi));
    p2Dspt = pt_2D(indspt,1:2);
    rospt = dim1(indspt);
    rispt = dim2(indspt);
    Rpmspt = Rpul(indspt);
    murspt = mur(indspt,:);
    %sig(Ncir+Nrec+(1:Nagi))  = 1./(Rpmagi.*(2*abs(wagi.*tagi)-tagi.^2));
    idtmp = find(indspt==1);
    for ik = 1:Nagi
        idspt_mtx(idtmp(ik),ik) = 1;
    end
end

% order matrix for condcutors
ind_mtx = ([idcir_mtx, idrec_mtx, idagi_mtx, idspt_mtx]);

%% 3. calculation

Pmesh = cal_P_mesh2d_cmplt(p2Dcir,rocir,ricir,  p2Drec,wrec,hrec, ...
    p2Dagi,wagi,tagi,  p2Dspt,rospt,rispt,  len(1));






