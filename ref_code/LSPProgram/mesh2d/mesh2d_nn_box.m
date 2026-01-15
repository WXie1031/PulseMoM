function [Xm,Ym, X1,X2, wh,ls, NVsr, Xp] = mesh2d_nn_box(pt_2d, wo,ho, dl)
%  Function:       mesh2d_P_box
%  Description:    Mesh box cross sections (2D) on the surface
%  Calls:          
%  Input:          p2D  --  coordinate of source line (2D cross section)
%                  wo   --  width of the conductor
%                  ho   --  higth of the conductor
%                  sig  --  coordinate of end point of field line
%                  f0   --  frequency
%  Output:         Xs   --  axis of the rectangle segments
%                  ws   --  width of the segments(x-axis)
%                  hs   --  higth of the segments(y-axis)
%                  Rs   --  DC resistance of the segments
%                  ls   --  length of the segments
%                  NVsr --  (vector)the number of the segments for each rectangle
%                  Xp   --  axises of the four corners in the box (used for plotting)
%  Others:         
%  Author:         Chen Hongcai    
%  Email :         hc.chen@live.com      
%  Date:           2017-03-13
%  History:         


mu0 = 4*pi*1e-7;
% for nonmagnetic body (mur = 1)

Xorec = pt_2d(:,1);
Yorec = pt_2d(:,2);

Nc = size(Xorec,1);
NVw = zeros(Nc,1);
NVh = zeros(Nc,1);


%% %%%%%%%%%%%%%%%%%%%%%%%%%%% Segment Scheme %%%%%%%%%%%%%%%%%%%%%%%%%%%%
f0 = max(f0,200e3);

% shin depth
SKD = 1./sqrt(pi*f0.*sig*mu0);
ds = 0.1054*SKD;
% used in h2 and w2 deciding the interval length
gama = 1.2;

% minimum no. of segments
Nmin = 1;

%% h1 h2

% % no. of hs
for ik = 1:Nc
    cnt = 0;
    dh = ho(ik)/2;
    while 1
        dh = dh - gama^cnt*ds(ik);
        cnt = cnt+1;
        if dh <= 0
            break
        end
    end
    NVh(ik) = cnt;
end

% % hs
Nhm = max(NVh);
hs = zeros(Nc,Nhm);
for ik = 1:Nc
    for ig = 1:NVh(ik)-1
        hs(ik,ig) = gama^(ig-1)*ds(ik);
    end
    ig = NVh(ik);
    dh = ho(ik)/2-sum(hs(ik,:));
    if dh < hs(ik,ig-1)/2
        hs(ik,NVh(ik)) = 0;
        NVh(ik) = NVh(ik)-1;
        hs(ik,NVh(ik)) = dh+hs(ik,NVh(ik));
    else
        hs(ik,NVh(ik)) = dh;
    end
end

% % no. of ws
for ik = 1:Nc
    cnt = 0; 
    dw = wo(ik)/2;
    while 1
        dw = dw - gama^cnt*ds(ik);
        cnt = cnt+1;
        if dw <= 0
            break
        end
    end
    NVw(ik) = cnt;
end

% % ws
Nwm = max(NVw);
ws = zeros(Nc,Nwm);
for ik = 1:Nc
    for ig = 1:NVw(ik)-1
        ws(ik,ig) = gama^(ig-1)*ds(ik);
    end
    ig = NVw(ik);
    dw = wo(ik)/2-sum(ws(ik,:));
    if dw < ws(ik,ig-1)/2
        ws(ik,NVw(ik)) = 0;
        NVw(ik) = NVw(ik)-1;
        ws(ik,NVw(ik)) = dw+ws(ik,NVw(ik));
    else
        ws(ik,NVw(ik)) = dw;
    end
end


for ik = 1:Nc
    if NVh(ik) < Nmin
        NVh(ik) = Nmin;
        hs(ik,1:Nmin) = ho(ik)/Nmin;
    end
    if NVw(ik) < Nmin
        NVw(ik) = Nmin;
        ws(ik,1:Nmin) = wo(ik)/Nmin;
    end
end


%% %%%%%%%%%%%% Generate Axis of Left Below Quarter of REC %%%%%%%%%%%%%%%%
% set the Left Below corner point as (0,0)
tws = zeros(Nc,Nwm);
ths = zeros(Nc,Nhm);
for ik = 1:Nwm
    tws(:,ik) = sum(ws(:,1:ik),2);
end
for ik = 1:Nhm
    ths(:,ik) = sum(hs(:,1:ik),2);
end

Dws = tws-ws/2;
Dhs = ths-hs/2;

NVs = NVw+NVh;

Dxym = zeros(sum(NVs),2); % middle point of the line
Dxy1 = zeros(sum(NVs),2); % start point of the line
Dxy2 = zeros(sum(NVs),2); % end point of the line
whtmp = zeros(sum(NVs),1); % length in the cross section

sdim = 0;
for ik = 1 : Nc
    
 	ind = sdim+1:sdim+NVh(ik);
 	Dxym(ind,:) = [ zeros(NVh(ik),1) Dhs(ik,1:NVh(ik))' ];
    Dxy1(ind,:) = [ zeros(NVh(ik),1) [0 ths(ik,1:NVh(ik)-1)]' ];
    Dxy2(ind,:) = [ zeros(NVh(ik),1) ths(ik,1:NVh(ik))' ];
    whtmp(ind,:) = hs(ik,1:NVh(ik))' ;
    
    sdim = sdim + NVh(ik);

 	ind = sdim+1:sdim+NVw(ik);
	Dxym(ind,:) = [ Dws(ik,1:NVw(ik))'       zeros(NVw(ik),1)];
  	Dxy1(ind,:) = [ [0 tws(ik,1:NVw(ik)-1)]' zeros(NVw(ik),1)];
 	Dxy2(ind,:) = [ tws(ik,1:NVw(ik))'       zeros(NVw(ik),1)];
    whtmp(ind,:) = ws(ik,1:NVw(ik))' ;

    sdim = sdim + NVw(ik);

end
    
%% %%%%%%%%%%%%% Generate the Exact Axis of the Whole REC %%%%%%%%%%%%%%%%%
Xp = zeros(4*Nc,2);
Ns = sum(NVs);
Xm = zeros(Ns,1);
Ym = zeros(Ns,1);
X1 = zeros(Ns,2);
X2 = zeros(Ns,2);
wh = zeros(Ns,1);


sdim = 0;
ddim = 0;
for ik = 1:Nc
    
    pdim = (ik-1)*4; 
    dind = ddim+1:ddim+NVs(ik);
    
    % 4--1
    xind = sdim+1:sdim+NVs(ik);
    Xp(pdim+1,1) = Xorec(ik) - wo(ik)/2;
    Xp(pdim+1,2) = Yorec(ik) - ho(ik)/2;

    Xm(xind,:) = Xp(pdim+1,1)+Dxym(dind,1);  
    Ym(xind,:) = Xp(pdim+1,2)+Dxym(dind,2);  
    X1(xind,:) = [ Xp(pdim+1,1)+Dxy1(dind,1) Xp(pdim+1,2)+Dxy1(dind,2) ]; 
    X2(xind,:) = [ Xp(pdim+1,1)+Dxy2(dind,1) Xp(pdim+1,2)+Dxy2(dind,2) ]; 
    ls(xind) = lo(ik)*ones(NVs(ik),1);
    wh(xind) = whtmp(dind);

    sdim = sdim+NVs(ik);
    
    % 4--2
    xind = sdim+1:sdim+NVs(ik);
    Xp(pdim+2,1) = Xorec(ik) - wo(ik)/2;
    Xp(pdim+2,2) = Yorec(ik) + ho(ik)/2;
    
    Xm(xind,:) = Xp(pdim+2,1)+Dxym(dind,1); 
    Ym(xind,:) = Xp(pdim+2,2)-Dxym(dind,2); 
    X1(xind,:) = [ Xp(pdim+2,1)+Dxy1(dind,1) Xp(pdim+2,2)-Dxy1(dind,2)]; 
    X2(xind,:) = [ Xp(pdim+2,1)+Dxy2(dind,1) Xp(pdim+2,2)-Dxy2(dind,2)]; 

    wh(xind) = whtmp(dind);

    sdim = sdim+NVs(ik);
    
    % 4--3
    xind = sdim+1:sdim+NVs(ik);
    Xp(pdim+3,1) = Xorec(ik) + wo(ik)/2;
    Xp(pdim+3,2) = Yorec(ik) + ho(ik)/2;
    
    Xm(xind,:) = Xp(pdim+3,1)-Dxym(dind,1); 
    Ym(xind,:) = Xp(pdim+3,2)-Dxym(dind,2); 
    X1(xind,:) = [ Xp(pdim+3,1)-Dxy1(dind,1) Xp(pdim+3,2)-Dxy1(dind,2)]; 
    X2(xind,:) = [ Xp(pdim+3,1)-Dxy2(dind,1) Xp(pdim+3,2)-Dxy2(dind,2)]; 

    wh(xind) = whtmp(dind);

    sdim = sdim+NVs(ik);
    
    % 4--4
    xind = sdim+1:sdim+NVs(ik);
    Xp(pdim+4,1) = Xorec(ik) + wo(ik)/2;
    Xp(pdim+4,2) = Yorec(ik) - ho(ik)/2;
    
    Xm(xind,:) = Xp(pdim+4,1)-Dxym(dind,1);
    Ym(xind,:) = Xp(pdim+4,2)+Dxym(dind,2); 
    X1(xind,:) = [ Xp(pdim+4,1)-Dxy1(dind,1) Xp(pdim+4,2)+Dxy1(dind,2)]; 
    X2(xind,:) = [ Xp(pdim+4,1)-Dxy2(dind,1) Xp(pdim+4,2)+Dxy2(dind,2)]; 
    
    wh(xind) = whtmp(dind);

    sdim = sdim+NVs(ik);
    
    % % %
    ddim = ddim + NVs(ik);
end

NVsr = 4*NVs;

end


