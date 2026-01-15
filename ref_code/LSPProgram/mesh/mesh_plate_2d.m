function [pt_sta_L,pt_end_L,pt_mid_L, dv_L, shpid_L,d1_L,d2_L, R_L,sig_L,mur_L,epr_L, ...
    pt_P,pt_mid_P, dv_P, shape_P,d1_P,d2_P, Nxt,Nyt] = mesh_plate_2d...
    (pt_mid, dv, shp_id, dim1,dim2, R_pul, sig, mur,epr, dlx,dly, dl_type, p_flag)

% p_mid_xxx  is the center of the tape, to be used for calculating delay
% time.

% determine dl according the source
% dt of lightning will be considered afterwards

%% add new branches and nodes due to segmenting
Nc = size(pt_mid,1);

switch dl_type
    case 1  % input the interval length
        Nsx = ceil(dim1./dlx);
        Nsy = ceil(dim2./dly);
    case 2  % input the number of the segment
        if length(dlx)==1 && Nc~=1
            Nsx = dlx*ones(Nc,1);
            Nsy = dly*ones(Nc,1);
        else
            Nsx = dlx;
            Nsy = dly;
        end
end
Nxt = sum(Nsx.*(Nsy+1));
Nyt = sum(Nsy.*(Nsx+1));

% NL_all = sum(Nsx.*(Nsy+1)) + sum(Nsy.*(Nsx+1));
NL_all = Nxt + Nyt;

%% 1. mesh the L-cell
pt_mid_L = zeros(NL_all,3);
pt_sta_L = zeros(NL_all,3);
pt_end_L = zeros(NL_all,3);
dv_L    = zeros(NL_all,3);
shpid_L = zeros(NL_all,1);
d1_L    = zeros(NL_all,1);
d2_L    = zeros(NL_all,1);
R_L     = zeros(NL_all,1);
sig_L   = zeros(NL_all,1);
mur_L   = zeros(NL_all,1);
epr_L   = zeros(NL_all,1);

S = dim1.*dim2;

cnt = 0;
for ik = 1:Nc
    dlx_tmp = dim1(ik)./Nsx(ik);
    dly_tmp = dim2(ik)./Nsy(ik);

    dlx_all = [dlx_tmp/2; dlx_tmp*ones(Nsx(ik)-1,1); dlx_tmp/2]; 
    dly_all = [dly_tmp/2; dly_tmp*ones(Nsy(ik)-1,1); dly_tmp/2]; 
    
    xps = pt_mid(ik,1)-dim1(ik)/2 + cumsum(dlx_all)-dlx_all/2;
    yps = pt_mid(ik,2)-dim2(ik)/2 + cumsum(dly_all)-dly_all/2;
    
    xs = pt_mid(ik,1)-dim1(ik)/2 + dlx_tmp*((0:Nsx(ik)-1)'+1/2);
    ys = pt_mid(ik,2)-dim2(ik)/2 + dly_tmp*((0:Nsy(ik)-1)'+1/2);

    xnod = pt_mid(ik,1)-dim1(ik)/2 + dlx_tmp*((0:Nsx(ik))');
    ynod = pt_mid(ik,2)-dim2(ik)/2 + dly_tmp*((0:Nsy(ik))');
    
    Nsx_all = Nsx(ik)*(Nsy(ik)+1);
    Nsy_all = Nsy(ik)*(Nsx(ik)+1);
    
    [Xxgrd, Yxgrd] = meshgrid(xs, yps);
    [Xygrd, Yygrd] = meshgrid(xps, ys);
    [Xnod, Ynod] = meshgrid(xnod, ynod);
    
    % x-cell
    idx = cnt+(1:Nsx_all);

    pt_mid_L(idx,1) = reshape(Xxgrd',Nsx_all,1);
    pt_mid_L(idx,2) = reshape(Yxgrd',Nsx_all,1);
    pt_mid_L(idx,3) = pt_mid(ik,3)*ones(Nsx_all,1);

    % other parameters
    shpid_L(idx) = shp_id(ik);
    d1_L(idx) = dlx_tmp*ones(Nsx_all,1);
    d2_L(idx) = reshape(repmat(dly_all',Nsx(ik),1),Nsx_all,1);
    R_L(idx) = R_pul(ik);
    dv_L(idx,1:3) = ones(Nsx_all,1)*[1 0 0];
    sig_L(idx) = sig(ik)*ones(Nsx_all,1);
    mur_L(idx) = mur(ik)*ones(Nsx_all,1);
    epr_L(idx) = epr(ik)*ones(Nsx_all,1);
    
    pt_sta_L(idx,1) = pt_mid_L(idx,1)-d1_L(idx)/2;
    pt_sta_L(idx,2) = reshape(Ynod(:,1:Nsx(ik))',Nsx_all,1);
    pt_sta_L(idx,3) = pt_mid(ik,3);
    
    pt_end_L(idx,1) = pt_mid_L(idx,1)+d1_L(idx)/2;
    pt_end_L(idx,2) = reshape(Ynod(:,2:Nsx(ik)+1)',Nsx_all,1);
    pt_end_L(idx,3) = pt_mid(ik,3);
    
    cnt = cnt+Nsx_all;
    
    % y-cell
    idy = cnt+(1:Nsy_all);

    pt_mid_L(idy,1) = reshape(Xygrd',Nsy_all,1);
    pt_mid_L(idy,2) = reshape(Yygrd',Nsy_all,1);
    pt_mid_L(idy,3) = pt_mid(ik,3);

     % other parameters
    shpid_L(idy) = shp_id(ik);
   
    d1_L(idy) = repmat(dlx_all,Nsy(ik),1);
    d2_L(idy) = dly_tmp*ones(Nsy_all,1);
    R_L(idy) = R_pul(ik);
    dv_L(idy,1:3) = ones(Nsy_all,1)*[0 1 0];
    sig_L(idy) = sig(ik)*ones(Nsy_all,1);
    mur_L(idy) = mur(ik)*ones(Nsy_all,1);
    epr_L(idy) = epr(ik)*ones(Nsy_all,1);
    
    pt_sta_L(idy,1) = reshape(Xnod(1:Nsy(ik),:)',Nsy_all,1);
    pt_sta_L(idy,2) = pt_mid_L(idy,2)-d2_L(idy)/2;
    pt_sta_L(idy,3) = pt_mid(ik,3);
    
    pt_end_L(idy,1) = reshape(Xnod(2:Nsy(ik)+1,:)',Nsy_all,1);
    pt_end_L(idy,2) = pt_mid_L(idy,2)+d2_L(idy)/2;
    pt_end_L(idy,3) = pt_mid(ik,3);
    
    cnt = cnt+Nsy_all;
end % g = 1:Nc


%% 2. mesh the P-cell if nessesary
if p_flag>0
    Ns_all = (Nsx+1).*(Nsy+1);
    NP_all = sum(Ns_all);
    
    pt_P     = zeros(NP_all,3);
    pt_mid_P = zeros(NP_all,3);
    dv_P    = zeros(NP_all,3);
    shape_P = zeros(NP_all,1);
    d1_P    = zeros(NP_all,1);
    d2_P    = zeros(NP_all,1);
    
    % generate the initial node for capacitance reduction
    cnt = 0;
    for ik = 1:Nc
        
        dlx_tmp = dim1(ik)./Nsx(ik);
        dly_tmp = dim2(ik)./Nsy(ik);
        
        dlx_all = [dlx_tmp/2; dlx_tmp*ones(Nsx(ik)-1,1); dlx_tmp/2];
        dly_all = [dly_tmp/2; dly_tmp*ones(Nsy(ik)-1,1); dly_tmp/2];
        
        xps = pt_mid(ik,1)-dim1(ik)/2 + cumsum(dlx_all)-dlx_all/2;
        yps = pt_mid(ik,2)-dim2(ik)/2 + cumsum(dly_all)-dly_all/2;
        
        xnod = pt_mid(ik,1)-dim1(ik)/2 + dlx_tmp*((0:Nsx(ik))');
        ynod = pt_mid(ik,2)-dim2(ik)/2 + dly_tmp*((0:Nsy(ik))');
        
        [Xpgrd, Ypgrd] = meshgrid(xps, yps);
        [Xnod, Ynod] = meshgrid(xnod, ynod);
    
        idp = cnt+(1:Ns_all(ik));

        pt_mid_P(idp,1) = reshape(Xpgrd',Ns_all(ik),1);
        pt_mid_P(idp,2) = reshape(Ypgrd',Ns_all(ik),1);
        pt_mid_P(idp,3) = pt_mid(ik,3)*ones(Ns_all(ik),1);
        
        pt_P(idp,1) = reshape(Xnod',Ns_all(ik),1);
        pt_P(idp,2) = reshape(Ynod',Ns_all(ik),1);
        pt_P(idp,3) = pt_mid(ik,3)*ones(Ns_all(ik),1);
        
        % other parameters
        shape_P(idp) = shp_id(ik);
        d1_P(idp) = repmat(dlx_all,Nsy(ik)+1,1);
        d2_P(idp) = reshape(repmat(dly_all',Nsx(ik)+1,1),Ns_all(ik),1);
        dv_P(idp,1:3) = ones(Ns_all(ik),1)*dv(ik,1:3);
        
        cnt = cnt+Ns_all(ik);
    end  % g = 1:Nc
else
    pt_mid_P = [];
    pt_P = [];
    dv_P    = [];
    shape_P = [];
    d1_P    = [];
    d2_P    = [];
end
   

    
end



