function [p_sta_L,p_end_L,dv_L,re_L,l_L, shape_L,d1_L,d2_L, R_L, sig_L,mur_L,epr_L, ...
    p_sta_P,p_end_P,pt_P,re_P,dv_P,l_P, shape_P,d1_P,d2_P] ...
    = mesh_line_3d(pt_start,pt_end,dv,re,len, shape,dim1,dim2, R_pul,sig,mur,epr, ...
     dl,dl_type, p_flag)
%  Function:       mesh_line_3d
%  Description:    meshing the 3D wire system. 
%
%  Calls:          
%
%  Input:          pt_start  --  start point of conductors (N*3) (m)
%                  pt_end    --  end point of conductors (N*3) (m)
%                  dv        --  direction vector of conductors (N*3)
%                  re        --  equivalent radius (N*1)
%                  len       --  length of conductors (N*1)
%                  shape     --  flag of the shape of the cross section
%                  dim1      --  dimension 1 of the cross section
%                  dim2      --  dimension 2 of the cross section
%                  R_pul     --  resistance of conductors (N*1) (ohm/m)
%                  sig       --  conducvitivity of conductors (N*1)
%                  mur       --  relative permeability of conductors (N*1)
%                  dl        --  length of the segment
%                  dl_type   --  type of the segment
%                  p_flag    --  if mesh P cell or not
%  Output:         Rmtx --  R matrix
%                  Lmtx --  L matrix
%  Others:
%  Author:         Chen Hongcai
%  Email :         hc.chen@live.com
%  Date:           2014-12-13

% determine dl according the source
% dt of lightning will be considered afterwards

%% add new branches and nodes due to segmenting
Nc = size(pt_start,1);

switch dl_type
    case 1  % input the interval length
        Ns = ceil(len./dl);
    case 2  % input the number of the segment
        if length(dl)==1 && Nc~=1
            Ns = dl*ones(Nc,1);
        else
            Ns = dl;
        end
end

NL_all = sum(Ns);

%% 1. mesh the L-cell
p_sta_L = zeros(NL_all,3);
p_end_L = zeros(NL_all,3);
dv_L    = zeros(NL_all,3);
l_L     = zeros(NL_all,1);
re_L    = zeros(NL_all,1);
shape_L = zeros(NL_all,1);
d1_L    = zeros(NL_all,1);
d2_L    = zeros(NL_all,1);
R_L     = zeros(NL_all,1);
sig_L   = zeros(NL_all,1);
mur_L   = zeros(NL_all,1);
epr_L   = zeros(NL_all,1);

cnt = 1;
for ik = 1:Nc
    if Ns(ik) == 1
        p_sta_L(cnt,:) = pt_start(ik,:);
        p_end_L(cnt,:) = pt_end(ik,:);
        
        dv_L(cnt,:) = dv(ik,:);
        l_L(cnt) = len(ik);
        re_L(cnt) = re(ik);
        
        shape_L(cnt) = shape(ik);
        d1_L(cnt)    = dim1(ik);
        d2_L(cnt)    = dim2(ik);
        R_L(cnt)     = R_pul(ik);
        sig_L(cnt)   = sig(ik);
        mur_L(cnt)   = mur(ik);
        epr_L(cnt)   = epr(ik);
        
        cnt = cnt+1;
    else
        ind = cnt+(0:Ns(ik)-1);
        dltmp = len(ik)./Ns(ik);
        
        lx = dltmp*(dv(ik,1));
        ly = dltmp*(dv(ik,2));
        lz = dltmp*(dv(ik,3));
        
        p_sta_L(ind,1) = pt_start(ik,1) + lx*(0:Ns(ik)-1)';
        p_sta_L(ind,2) = pt_start(ik,2) + ly*(0:Ns(ik)-1)';
        p_sta_L(ind,3) = pt_start(ik,3) + lz*(0:Ns(ik)-1)';
        p_end_L(ind,1) = pt_start(ik,1) + lx*(1:Ns(ik))';
        p_end_L(ind,2) = pt_start(ik,2) + ly*(1:Ns(ik))';
        p_end_L(ind,3) = pt_start(ik,3) + lz*(1:Ns(ik))';
        
        dv_L(ind,1:3) = ones(Ns(ik),1)*dv(ik,1:3);
        l_L(ind) = dltmp;
        re_L(ind) = re(ik);
        
        shape_L(ind) = shape(ik);
        d1_L(ind)    = dim1(ik);
        d2_L(ind)    = dim2(ik);
        R_L(ind)     = R_pul(ik);
        sig_L(ind)   = sig(ik);
        mur_L(ind)   = mur(ik);
        epr_L(ind)   = epr(ik);
        
        cnt = cnt+Ns(ik);
    end
end % g = 1:Nc


%% 2. mesh the P-cell if nessesary
if p_flag>0
    NP_all = sum(Ns)+Nc; % every line add 1 segment
    
    p_sta_P = zeros(NP_all,3);
    p_end_P = zeros(NP_all,3);
    pt_P     = zeros(NP_all,3);
    dv_P    = zeros(NP_all,3);
    re_P    = zeros(NP_all,1);
    l_P     = zeros(NP_all,1);
    shape_P = zeros(NP_all,1);
    d1_P    = zeros(NP_all,1);
    d2_P    = zeros(NP_all,1);

    
    cnt = 1;
    for ik = 1:Nc
        dltmp = len(ik)./Ns(ik);
        lx = dltmp*(dv(ik,1));
        ly = dltmp*(dv(ik,2));
        lz = dltmp*(dv(ik,3));
        
        % first P-segment  dl/2
        p_sta_P(cnt,:) = pt_start(ik,:);
        p_end_P(cnt,1) = pt_start(ik,1) + lx/2;
        p_end_P(cnt,2) = pt_start(ik,2) + ly/2;
        p_end_P(cnt,3) = pt_start(ik,3) + lz/2;
        
        pt_P(cnt,:) = pt_start(ik,:);
        
        l_P(cnt) = dltmp/2;

        if Ns(ik) > 1
            % common segment
            ind = cnt+(1:Ns(ik)-1);
            p_sta_P(ind,1) = p_end_P(cnt,1) + lx*(0:Ns(ik)-2)';
            p_sta_P(ind,2) = p_end_P(cnt,2) + ly*(0:Ns(ik)-2)';
            p_sta_P(ind,3) = p_end_P(cnt,3) + lz*(0:Ns(ik)-2)';
            p_end_P(ind,1) = p_end_P(cnt,1) + lx*(1:Ns(ik)-1)';
            p_end_P(ind,2) = p_end_P(cnt,2) + ly*(1:Ns(ik)-1)';
            p_end_P(ind,3) = p_end_P(cnt,3) + lz*(1:Ns(ik)-1)';
            
            pt_P(ind,:) = (p_sta_P(ind,:)+p_end_P(ind,:))/2;
            
            l_P(ind) = dltmp;
            
        end
        
        % end segment
        p_sta_P(cnt+Ns(ik),1) = pt_end(ik,1) - lx/2;
        p_sta_P(cnt+Ns(ik),2) = pt_end(ik,2) - ly/2;
        p_sta_P(cnt+Ns(ik),3) = pt_end(ik,3) - lz/2;
        p_end_P(cnt+Ns(ik),:) = pt_end(ik,:);

        pt_P(cnt+Ns(ik),:) = pt_end(ik,:);
        
        l_P(cnt+Ns(ik)) = dltmp/2;
        
        % other parameters
        ind = cnt+(0:Ns(ik));
        
        re_P(ind) = re(ik);
        shape_P(ind) = shape(ik);
        d1_P(ind) = dim1(ik);
        d2_P(ind) = dim2(ik);
        dv_P(ind,1:3) = ones(Ns(ik)+1,1)*dv(ik,1:3);
        
        cnt = cnt+Ns(ik)+1;
    end  % g = 1:Nc
else
    p_sta_P = [];
    p_end_P = [];
    pt_P     = [];
    dv_P    = [];
    re_P    = [];
    l_P     = [];
    shape_P = [];
    d1_P    = [];
    d2_P    = [];
end
   

    
end



