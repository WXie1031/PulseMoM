function [Xs,Ys, ws,hs, dS, Rs, NVsr] = mesh2d_box(pt_2d, wo,ho, Ro, f0)
%  Function:       mesh2D_box
%  Description:    Mesh box cross sections (2D). The centre of the box is
%                  diged out
%  Calls:          mesh2D_box_sub
%  Input:          p2D  --  coordinate of source line (2D cross section)
%                  wo   --  width of the conductor
%                  ho   --  higth of the conductor
%                  lo   --  length of the conductor
%                  Ro   --  resistivity of the conductor (ohm/m)
%                  Sig  --  coordinate of end point of field line
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
%  Date:           2013-11-13
%  History:         



mu0 = 4*pi*1e-7;
% for nonmagnetic body (mur = 1)

Nc = size(pt_2d,1);

%% 1. cal_t is the exact calculation thickness(adding the skin effect)
if f0 < 100e3
    f0 = min(2*f0,500e3);
else
    f0 = min(f0,2e6);
end

% shin depth
cond = 1e7; % conductivity

s_dep = 1./sqrt(pi*f0.*cond*mu0) * ones(Nc,1);

Sr = abs(wo.*ho);

%% 2. segment method
[NVw1, NVw2, NVh1, NVh2, NVs, w1, w2, h1, h2] = mesh2d_box_sub( wo, ho, s_dep );

%% 3. generate axis of quarter of the rect

tw1 = cumsum(w1,2);
tw2 = cumsum(w2,2);
th1 = cumsum(h1,2);
th2 = cumsum(h2,2);

Dxy = zeros(sum(sum(NVs)),2);
Dh  = zeros(sum(sum(NVs)),1);
Dw  = zeros(sum(sum(NVs)),1);
Dw1 = tw1-w1/2;
Dw2 = sum(w1,2)*ones(1,length(w2(1,:)))+tw2-w2/2;
Dh1 = th1-h1/2;
Dh2 = sum(h1,2)*ones(1,length(h2(1,:)))+th2-h2/2;
    
sdim = 0;
for ik = 1 : Nc
    
    for g = 1:NVw1(ik)
        ind = sdim+(g-1)*NVh1(ik)+1:sdim+g*NVh1(ik);
        Dxy(ind,:) = [Dw1(ik,g)*ones(NVh1(ik),1) Dh1(ik,1:NVh1(ik))'];
        Dh(ind,:) = h1(ik,1:NVh1(ik))';
        Dw(ind,:) = w1(ik,g)*ones(NVh1(ik),1);
    end
    sdim = sdim + sum(NVs(ik,1));
    
    if NVw2(ik) ~= 0
        for g = 1:NVw2(ik)
            ind = sdim+(g-1)*NVh1(ik)+1:sdim+g*NVh1(ik);
            Dxy(ind,:) = [Dw2(ik,g)*ones(NVh1(ik),1) Dh1(ik,1:NVh1(ik))'];
            Dh(ind,:) = h1(ik,1:NVh1(ik))';
            Dw(ind,:) = w2(ik,g)*ones(NVh1(ik),1);
        end
    end
    sdim = sdim + sum(NVs(ik,2));
    
    if NVh2(ik) ~= 0
        for g = 1:NVh2(ik)
            ind = sdim+(g-1)*NVw1(ik)+1:sdim+g*NVw1(ik);
            Dxy(ind,:) = [Dw1(ik,1:NVw1(ik))' Dh2(ik,g)*ones(NVw1(ik),1)];
            Dh(ind,:) = h2(ik,g)*ones(NVw1(ik),1);
            Dw(ind,:) = w1(ik,1:NVw1(ik));
        end
    end
    sdim = sdim + sum(NVs(ik,3));

end
    
%% 4. generate the exact axis of the whole rect
Ns = sum(sum(NVs));
Xp = zeros(4*Nc,2);
Xs = zeros(Ns,1);
Ys = zeros(Ns,1);
dS = zeros(Ns,1);
ws = zeros(Ns,1);
hs = zeros(Ns,1);
Rs = zeros(Ns,1);

sdim = 0;
ddim = 0;
for ik = 1:Nc
    
    pdim = (ik-1)*4; 
    dind = ddim+1:ddim+sum(NVs(ik,:));
    
    % 4--1
    xind = sdim+1:sdim+sum(NVs(ik,:));
    Xp(pdim+1,1) = pt_2d(ik,1) - wo(ik)/2;
    Xp(pdim+1,2) = pt_2d(ik,2) - ho(ik)/2;

    Xs(xind,:) = Xp(pdim+1,1)+Dxy(dind,1); 
    Ys(xind,:) = Xp(pdim+1,2)+Dxy(dind,2); 
    ws(xind) = Dw(dind);
    hs(xind) = Dh(dind);
    dS(xind) = ws(xind).*hs(xind);
    Rs(xind) = Ro(ik)*Sr(ik)./dS(xind);
    sdim = sdim+sum(NVs(ik,:));
    
    % 4--2
    xind = sdim+1:sdim+sum(NVs(ik,:));
    Xp(pdim+2,1) = Xp(pdim+1,1);
    Xp(pdim+2,2) = ho(ik)+Xp(pdim+1,2);
    
    Xs(xind) = Xp(pdim+2,1)+Dxy(dind,1); 
    Ys(xind) = Xp(pdim+2,2)-Dxy(dind,2); 
    ws(xind) = Dw(dind);
    hs(xind) = Dh(dind);
    dS(xind) = ws(xind).*hs(xind);
    Rs(xind) = Ro(ik)*Sr(ik)./dS(xind);
    sdim = sdim+sum(NVs(ik,:));
    
    % 4--3
    xind = sdim+1:sdim+sum(NVs(ik,:));
    Xp(pdim+3,1) = wo(ik)+Xp(pdim+2,1);
    Xp(pdim+3,2) = Xp(pdim+2,2);
    
    Xs(xind) = Xp(pdim+3,1)-Dxy(dind,1); 
    Ys(xind) = Xp(pdim+3,2)-Dxy(dind,2); 
    ws(xind) = Dw(dind);
    hs(xind) = Dh(dind);
    dS(xind) = ws(xind).*hs(xind);
    Rs(xind) = Ro(ik)*Sr(ik)./dS(xind);
    sdim = sdim+sum(NVs(ik,:));
    
    % 4--4
    xind = sdim+1:sdim+sum(NVs(ik,:));
    Xp(pdim+4,1) = Xp(pdim+3,1);
    Xp(pdim+4,2) = Xp(pdim+3,2)-ho(ik);
    
    Xs(xind,:) = Xp(pdim+4,1)-Dxy(dind,1); 
    Ys(xind,:) = Xp(pdim+4,2)+Dxy(dind,2); 
    ws(xind) = Dw(dind);
    hs(xind) = Dh(dind);
    dS(xind) = ws(xind).*hs(xind);
    Rs(xind) = Ro(ik)*Sr(ik)./dS(xind);
    sdim = sdim+sum(NVs(ik,:));
    
    % % %
    ddim = ddim + sum(NVs(ik,:));
end

%NVsr = 4*NVs;
NVsr = sum(4*NVs,2);

end


