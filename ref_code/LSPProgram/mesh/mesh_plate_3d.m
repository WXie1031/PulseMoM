function [pt_sta_L,pt_end_L,pt_mid_L, dv_L, shpid_L,d1_L,d2_L,d3_L, R_L, ...
    sig_L,mur_L,epr_L, pt_P,pt_mid_P, dv_P, shpid_P,d1_P,d2_P, ...
    Nsx,Nsy,Nsz] = mesh_plate_3d(pt_mid, dv, shp_id, dim1,dim2,dim3, ...
    R_pul, sig,mur,epr, dlx,dly,dlz, dl_type, p_flag)
    

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
        Nsz = ceil(dim3./dlz);
    case 2  % input the number of the segment
        if length(dlx)==1 && Nc~=1
            Nsx = dlx*ones(Nc,1);
            Nsy = dly*ones(Nc,1);
            Nsz = dlz*ones(Nc,1);
        else
            Nsx = dlx;
            Nsy = dly;
            Nsz = dlz;
        end
end

Nsx_all = Nsx.*(Nsy+1).*(Nsz+1);
Nsy_all = Nsy.*(Nsx+1).*(Nsz+1);
Nsz_all = (Nsy+1).*(Nsx+1).*Nsz;

NL_all = sum(Nsx_all) + sum(Nsy_all) + sum(Nsz_all);

%% 1. mesh the L-cell
pt_mid_L = zeros(NL_all,3);
pt_sta_L = zeros(NL_all,3);
pt_end_L = zeros(NL_all,3);
dv_L    = zeros(NL_all,3);
shpid_L = zeros(NL_all,1);
d1_L    = zeros(NL_all,1);
d2_L    = zeros(NL_all,1);
d3_L     = zeros(NL_all,1);
R_L     = zeros(NL_all,1);
sig_L   = zeros(NL_all,1);
mur_L   = zeros(NL_all,1);
epr_L   = zeros(NL_all,1);

cnt = 0;
for ik = 1:Nc
    dlx_tmp = dim1(ik)./Nsx(ik);
    dly_tmp = dim2(ik)./Nsy(ik);
    dlz_tmp = dim3(ik)./Nsz(ik);

    dlx_all = [dlx_tmp/2; dlx_tmp*ones(Nsx(ik)-1,1); dlx_tmp/2]; 
    dly_all = [dly_tmp/2; dly_tmp*ones(Nsy(ik)-1,1); dly_tmp/2]; 
    dlz_all = [dlz_tmp/2; dlz_tmp*ones(Nsz(ik)-1,1); dlz_tmp/2]; 
    
    xps = pt_mid(ik,1)-dim1(ik)/2 + cumsum(dlx_all)-dlx_all/2;
    yps = pt_mid(ik,2)-dim2(ik)/2 + cumsum(dly_all)-dly_all/2;
    zps = pt_mid(ik,3)-dim3(ik)/2 + cumsum(dlz_all)-dlz_all/2;
    
    xs = pt_mid(ik,1)-dim1(ik)/2 + dlx_tmp*((0:Nsx(ik)-1)'+1/2);
    ys = pt_mid(ik,2)-dim2(ik)/2 + dly_tmp*((0:Nsy(ik)-1)'+1/2);
    zs = pt_mid(ik,3)-dim3(ik)/2 + dlz_tmp*((0:Nsz(ik)-1)'+1/2);
    
    xnod = pt_mid(ik,1)-dim1(ik)/2 + dlx_tmp*((0:Nsx(ik))');
    ynod = pt_mid(ik,2)-dim2(ik)/2 + dly_tmp*((0:Nsy(ik))');
    znod = pt_mid(ik,3)-dim3(ik)/2 + dlz_tmp*((0:Nsz(ik))');
    
    
    [Xxgrd, Yxgrd, Zxgrd] = meshgrid(xs, yps, zps);
    [Xygrd, Yygrd, Zygrd] = meshgrid(xps, ys, zps);
    [Xzgrd, Yzgrd, Zzgrd] = meshgrid(xps, yps, zs);
    [Xnod, Ynod, Znod] = meshgrid(xnod, ynod, znod);
    
    % x-cell
    idx = cnt+(1:Nsx_all(ik));

    pt_mid_L(idx,1) = reshape(Xxgrd,Nsx_all(ik),1);
    pt_mid_L(idx,2) = reshape(Yxgrd,Nsx_all(ik),1);
    pt_mid_L(idx,3) = reshape(Zxgrd,Nsx_all(ik),1);

    % other parameters
    shpid_L(idx) = shp_id(ik);
    d1_L(idx) = dlx_tmp*ones(Nsx_all(ik),1);
    d2_L(idx) = repmat(dly_all,Nsx(ik)*(Nsz(ik)+1),1);
    d3_L(idx) = repmat(dlz_all,Nsx(ik)*(Nsy(ik)+1),1);

    
    R_L(idx) = R_pul(ik);
    dv_L(idx,1:3) = ones(Nsx_all(ik),1)*[1 0 0];
    sig_L(idx) = sig(ik)*ones(Nsx_all(ik),1);
    mur_L(idx) = mur(ik)*ones(Nsx_all(ik),1);
    epr_L(idx) = epr(ik)*ones(Nsx_all(ik),1);
    
    pt_sta_L(idx,1) = pt_mid_L(idx,1)-d1_L(idx)/2;  
    pt_sta_L(idx,2) = reshape(Ynod(:,1:Nsx(ik),:),Nsx_all(ik),1);
    pt_sta_L(idx,3) = reshape(Znod(:,1:Nsx(ik),:),Nsx_all(ik),1);
    
    pt_end_L(idx,1) = pt_mid_L(idx,1)+d1_L(idx)/2;
    pt_end_L(idx,2) = reshape(Ynod(:,2:Nsx(ik)+1,:),Nsx_all(ik),1);
    pt_end_L(idx,3) = reshape(Znod(:,2:Nsx(ik)+1,:),Nsx_all(ik),1);
    
    cnt = cnt+Nsx_all(ik);
    
    % y-cell
    idy = cnt+(1:Nsy_all(ik));

    pt_mid_L(idy,1) = reshape(Xygrd,Nsy_all(ik),1);
    pt_mid_L(idy,2) = reshape(Yygrd,Nsy_all(ik),1);
    pt_mid_L(idy,3) = reshape(Zygrd,Nsy_all(ik),1);

     % other parameters
    shpid_L(idy) = shp_id(ik);
   
    d1_L(idy) = repmat(dlx_all,Nsy(ik)*(Nsz(ik)+1),1);
    d2_L(idy) = dly_tmp*ones(Nsy_all(ik),1);
    d3_L(idy) = repmat(dlz_all,Nsy(ik)*(Nsx(ik)+1),1);
    
    R_L(idy) = R_pul(ik);
    dv_L(idy,1:3) = ones(Nsy_all(ik),1)*[0 1 0];
    sig_L(idy) = sig(ik)*ones(Nsy_all(ik),1);
    mur_L(idy) = mur(ik)*ones(Nsy_all(ik),1);
    epr_L(idy) = epr(ik)*ones(Nsy_all(ik),1);
    
    pt_sta_L(idy,1) = reshape(Xnod(1:Nsy(ik),:,:),Nsy_all(ik),1);
    pt_sta_L(idy,2) = pt_mid_L(idy,2)-d2_L(idy)/2;
    pt_sta_L(idy,3) = reshape(Znod(1:Nsy(ik),:,:),Nsy_all(ik),1);
    
    pt_end_L(idy,1) = reshape(Xnod(2:Nsy(ik)+1,:,:),Nsy_all(ik),1);
    pt_end_L(idy,2) = pt_mid_L(idy,2)+d2_L(idy)/2;
    pt_end_L(idy,3) = reshape(Znod(2:Nsy(ik)+1,:,:),Nsy_all(ik),1);
    
    cnt = cnt+Nsy_all(ik);
    
     % z-cell
    idz = cnt+(1:Nsz_all(ik));

    pt_mid_L(idz,1) = reshape(Xzgrd,Nsz_all(ik),1);
    pt_mid_L(idz,2) = reshape(Yzgrd,Nsz_all(ik),1);
    pt_mid_L(idz,3) = reshape(Zzgrd,Nsz_all(ik),1);

     % other parameters
    shpid_L(idz) = shp_id(ik);
   
    d1_L(idz) = repmat(dlx_all,Nsz(ik)*(Nsy(ik)+1),1);
    d2_L(idz) = repmat(dly_all,Nsz(ik)*(Nsx(ik)+1),1);
    d3_L(idz) = dlz_tmp*ones(Nsz_all(ik),1);
    
    R_L(idz) = R_pul(ik);
    dv_L(idz,1:3) = ones(Nsz_all(ik),1)*[0 0 1];
    sig_L(idz) = sig(ik)*ones(Nsz_all(ik),1);
    mur_L(idz) = mur(ik)*ones(Nsz_all(ik),1);
    epr_L(idz) = epr(ik)*ones(Nsz_all(ik),1);
    
    pt_sta_L(idz,1) = reshape(Xnod(:,:,1:Nsz(ik)),Nsz_all(ik),1);
    pt_sta_L(idz,2) = reshape(Ynod(:,:,1:Nsz(ik)),Nsz_all(ik),1);
    pt_sta_L(idz,3) = pt_mid_L(idz,3)-d3_L(idz)/2;
    
    pt_end_L(idz,1) = reshape(Xnod(:,:,2:Nsz(ik)+1),Nsz_all(ik),1);
    pt_end_L(idz,2) = reshape(Ynod(:,:,2:Nsz(ik)+1),Nsz_all(ik),1);
    pt_end_L(idz,3) = pt_mid_L(idz,3)+d3_L(idz)/2;
    
    cnt = cnt+Nsz_all(ik);
end % g = 1:Nc


%% 2. mesh the P-cell if nessesary
% top - down
% left - right
% front - back
if p_flag>0
    N1p_all = (Nsx+1).*(Nsy+1);
    N2p_all = (Nsy+1).*(Nsz+1);
    N3p_all = (Nsx+1).*(Nsz+1);
  
    NP_all = sum(N1p_all+N2p_all+N3p_all)*2;
    
    pt_P     = zeros(NP_all,3);
    pt_mid_P = zeros(NP_all,3);
    dv_P     = zeros(NP_all,3);
    shpid_P  = zeros(NP_all,1);
    d1_P     = zeros(NP_all,1);
    d2_P     = zeros(NP_all,1);
%     d3_P     = zeros(NP_all,1);

    % generate the initial node for capacitance reduction
    cnt = 0;
    for ik = 1:Nc
        
        dlx_tmp = dim1(ik)./Nsx(ik);
        dly_tmp = dim2(ik)./Nsy(ik);
        dlz_tmp = dim3(ik)./Nsz(ik);
        
        dlx_all = [dlx_tmp/2; dlx_tmp*ones(Nsx(ik)-1,1); dlx_tmp/2];
        dly_all = [dly_tmp/2; dly_tmp*ones(Nsy(ik)-1,1); dly_tmp/2];
        dlz_all = [dlz_tmp/2; dlz_tmp*ones(Nsz(ik)-1,1); dlz_tmp/2]; 

        xps = pt_mid(ik,1)-dim1(ik)/2 + cumsum(dlx_all)-dlx_all/2;
        yps = pt_mid(ik,2)-dim2(ik)/2 + cumsum(dly_all)-dly_all/2;
        zps = pt_mid(ik,3)-dim3(ik)/2 + cumsum(dlz_all)-dlz_all/2;
        
        xnod = pt_mid(ik,1)-dim1(ik)/2 + dlx_tmp*((0:Nsx(ik))');
        ynod = pt_mid(ik,2)-dim2(ik)/2 + dly_tmp*((0:Nsy(ik))');
        znod = pt_mid(ik,3)-dim3(ik)/2 + dlz_tmp*((0:Nsz(ik))');
        
        [X1pgrd, Y1pgrd] = meshgrid(xps, yps);
        [Y2pgrd, Z2pgrd] = meshgrid(yps, zps);
        [X3pgrd, Z3pgrd] = meshgrid(xps, zps);
        [X1nod, Y1nod] = meshgrid(xnod, ynod);
        [Y2nod, Z2nod] = meshgrid(ynod, znod);
        [X3nod, Z3nod] = meshgrid(xnod, znod);
        
        % 1. down surface - x cell
        idp = cnt+(1:N1p_all(ik));

        pt_mid_P(idp,1) = reshape(X1pgrd,N1p_all(ik),1);
        pt_mid_P(idp,2) = reshape(Y1pgrd,N1p_all(ik),1);
        pt_mid_P(idp,3) = pt_mid(ik,3)-dim3(ik)/2;
        
        pt_P(idp,1) = reshape(X1nod,N1p_all(ik),1);
        pt_P(idp,2) = reshape(Y1nod,N1p_all(ik),1);
        pt_P(idp,3) = pt_mid(ik,3)-dim3(ik)/2;
        
        shpid_P(idp) = shp_id(ik);
        d1_P(idp) = repmat(dlx_all,Nsy(ik)+1,1);
        d2_P(idp) = repmat(dly_all,Nsx(ik)+1,1);
%         d3_P(idP) = dim3(ik);
        dv_P(idp,1:3) = ones(N1p_all(ik),1)*[1 0 0];
        
        cnt = cnt+N1p_all(ik);
        
        % 2. up surface - x cell
        idp = cnt+(1:N1p_all(ik));

        pt_mid_P(idp,1) = reshape(X1pgrd,N1p_all(ik),1);
        pt_mid_P(idp,2) = reshape(Y1pgrd,N1p_all(ik),1);
        pt_mid_P(idp,3) = pt_mid(ik,3)+dim3(ik)/2;
        
        pt_P(idp,1) = reshape(X1nod,N1p_all(ik),1);
        pt_P(idp,2) = reshape(Y1nod,N1p_all(ik),1);
        pt_P(idp,3) = pt_mid(ik,3)+dim3(ik)/2;
        
        shpid_P(idp) = shp_id(ik);
        d1_P(idp) = repmat(dlx_all,Nsy(ik)+1,1);
        d2_P(idp) = repmat(dly_all,Nsx(ik)+1,1);
%         d3_P(idP) = dim3(ik);
        dv_P(idp,1:3) = ones(N1p_all(ik),1)*[1 0 0];

        cnt = cnt+N1p_all(ik);
        
         % 3. left surface - y cell
        idp = cnt+(1:N2p_all(ik));

        pt_mid_P(idp,1) = pt_mid(ik,1)-dim1(ik)/2;
        pt_mid_P(idp,2) = reshape(Y2pgrd,N2p_all(ik),1);
        pt_mid_P(idp,3) = reshape(Z2pgrd,N2p_all(ik),1);
        
        pt_P(idp,1) = pt_mid(ik,1)-dim1(ik)/2;
        pt_P(idp,2) = reshape(Y2nod,N2p_all(ik),1);
        pt_P(idp,3) = reshape(Z2nod,N2p_all(ik),1);
        
        shpid_P(idp) = shp_id(ik);
        d1_P(idp) = repmat(dly_all,Nsz(ik)+1,1);
        d2_P(idp) = repmat(dlz_all,Nsy(ik)+1,1);
%         d3_P(idP) = dim3(ik);
        dv_P(idp,1:3) = ones(N2p_all(ik),1)*[0 1 0];

        cnt = cnt+N2p_all(ik);
        
         % 4. right surface - y cell
        idp = cnt+(1:N2p_all(ik));

        pt_mid_P(idp,1) = pt_mid(ik,1)+dim1(ik)/2;
        pt_mid_P(idp,2) = reshape(Y2pgrd,N2p_all(ik),1);
        pt_mid_P(idp,3) = reshape(Z2pgrd,N2p_all(ik),1);
        
        pt_P(idp,1) = pt_mid(ik,1)+dim1(ik)/2;
        pt_P(idp,2) = reshape(Y2nod,N2p_all(ik),1);
        pt_P(idp,3) = reshape(Z2nod,N2p_all(ik),1);

        shpid_P(idp) = shp_id(ik);
        d1_P(idp) = repmat(dly_all,Nsz(ik)+1,1);
        d2_P(idp) = repmat(dlz_all,Nsy(ik)+1,1);
%         d3_P(idP) = dim3(ik);
        dv_P(idp,1:3) = ones(N2p_all(ik),1)*[0 1 0];

        cnt = cnt+N2p_all(ik);
        
        % 5. front surface - z cell
        idp = cnt+(1:N3p_all(ik));

        pt_mid_P(idp,1) = reshape(X3pgrd,N3p_all(ik),1);
        pt_mid_P(idp,2) = pt_mid(ik,2)-dim2(ik)/2;
        pt_mid_P(idp,3) = reshape(Z3pgrd,N3p_all(ik),1);
        
        pt_P(idp,1) = reshape(X3nod,N3p_all(ik),1);
        pt_P(idp,2) = pt_mid(ik,2)+dim2(ik)/2;
        pt_P(idp,3) = reshape(Z3nod,N3p_all(ik),1);
        
        shpid_P(idp) = shp_id(ik);
        d1_P(idp) = repmat(dlx_all,Nsz(ik)+1,1);
        d2_P(idp) = repmat(dlz_all,Nsx(ik)+1,1);
        dv_P(idp,1:3) = ones(N3p_all(ik),1)*[0 0 1];

        cnt = cnt+N3p_all(ik);
        
        % 6. back surface - z cell
        idp = cnt+(1:N3p_all(ik));

        pt_mid_P(idp,1) = reshape(X3pgrd,N3p_all(ik),1);
        pt_mid_P(idp,2) = pt_mid(ik,2)+dim2(ik)/2;
        pt_mid_P(idp,3) = reshape(Z3pgrd,N3p_all(ik),1);
        
        pt_P(idp,1) = reshape(X3nod,N3p_all(ik),1);
        pt_P(idp,2) = pt_mid(ik,2)+dim2(ik)/2;
        pt_P(idp,3) = reshape(Z3nod,N3p_all(ik),1);
        
        shpid_P(idp) = shp_id(ik);
        d1_P(idp) = repmat(dlx_all,Nsz(ik)+1,1);
        d2_P(idp) = repmat(dlz_all,Nsx(ik)+1,1);
%         d3_P(idP) = dim3(ik);
        dv_P(idp,1:3) = ones(N3p_all(ik),1)*[0 0 1];

        cnt = cnt+N3p_all(ik);
        
    end  % g = 1:Nc
else
    pt_mid_P = [];
    pt_P = [];
    dv_P    = [];
    shpid_P = [];
    d1_P    = [];
    d2_P    = [];
    
end
   

    
end



