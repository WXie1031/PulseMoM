function [Ggrid, Cgrid] = para_grid_cir_fix_frq(pt_start,pt_end, dv,re,len, ...
    sig_soil,epr_soil)
%  Function:       para_grid_cir_mutual
%  Description:    Calculate mutual R and C in ground grid using the
%                  model in paper "Lightning Impulse Performance of Grounding
%                  Grids for Substations Considering Soil Ionization"
%  Calls:          
%
%  Input:          pt_start  --  start point of conductors (N*3) (m)
%                  pt_end    --  end point of conductors (N*3) (m)
%                  dv        --  direction vector of conductors (N*3)
%                  re        --  equivalent radius (N*1)
%                  len       --  length of conductors (N*1)
%                  sig_soil  --  conductivity of the soil
%                  ep_soil   --  relative permeability of the soil
%  Output:         Rgm --  R matrix
%                  Cgm --  L matrix
%  Others:
%  Author:         Chen Hongcai
%  Email :         hc.chen@live.com
%  Date:           2016-10-13


% if isempty(pt_start)
%     Ggm = zeros(0,1);
%     Cgm = zeros(0,1);
%     
%     return;
% end

ep0 = 8.85*1e-12;
ERR = 1e-3;

Nc = size(pt_start,1);

pt_start_imag = pt_start;
pt_start_imag(:,3) = -pt_start_imag(:,3);
pt_end_imag = pt_end;
pt_end_imag(:,3) = -pt_end_imag(:,3);

%% 1. calculate the coefficients for mutual Z computation
frq = 1000e3;
w0 = 2*pi*frq;

cof1 = 1./( 4*pi.*(sig_soil+1j*w0.*epr_soil*ep0) );
cof2 = (sig_soil+1j*w0.*(epr_soil-1)*ep0) ./ (sig_soil+1j*w0.*(epr_soil+1)*ep0);


%% 2. calulate the integral for mutual Z parameters
int_mtx = zeros(Nc,Nc);
int_mtx_imag = zeros(Nc,Nc);
for ik = 1:Nc
    % calculate the integral value first
    id_s = ik;
    int_tmp = zeros(ik,1);
    int_tmp_imag = zeros(ik,1);
    
    ps1 = pt_start(ik,:);
    ps2 = pt_end(ik,:);
    dv1 = dv(ik,:);
    r1 = re(ik);
    l1 = len(ik);
    
    pf1 = pt_start(1:ik,:);
    pf2 = pt_end(1:ik,:);
    dv2 = dv(1:ik,:);
    r2 = re(1:ik);
    l2 = len(1:ik);
    
    pf1_imag = pt_start_imag(1:ik,:);
    pf2_imag = pt_end_imag(1:ik,:);
    dv_imag = [dv2(:,1:2) -dv2(:,3)];
    
    dvtmp = repmat(dv1,id_s,1);
    idp = false(id_s,1);
    idv = false(id_s,1);
    ida = false(id_s,1);
    idp(1:id_s) = sum(abs(cross(dvtmp, dv2,2)),2) <= ERR ;  % parallel
    idv(1:id_s) = abs(dot(dvtmp,dv2,2)) <= ERR ;  % perpendicular
    ida(1:id_s) = ~(idp+idv);            % arbitrary angle
    
    % for parallel conductors
    if sum(idp) > 0
        Nftmp = sum(idp);
        dv_sign = ones(Nftmp,1);
        
        ind = sum( abs(dvtmp(idp,1:3) + dv2(idp,1:3)),2  )<1e-3 ;
        dv_sign(ind) = -1;
        int_tmp(idp) = dv_sign.*int_fila_p(ps1,ps2,dv1,r1, pf1(idp,:),pf2(idp,:),dv2(idp,:),r2(idp));
        
        dv_sign = ones(Nftmp,1);
        ind = sum( abs(dvtmp(idp,1:3) + dv_imag(idp,1:3)),2  )<1e-3 ;
        dv_sign(ind) = -1;
        int_tmp_imag(idp) = dv_sign.*int_fila_p(ps1,ps2,dv1,r1, pf1_imag(idp,:),pf2_imag(idp,:),dv_imag(idp,:),r2(idp));
    end
    % for vectical conductors
    if sum(idv)>0
        int_tmp(idv) = int_fila_v_anal(ps1,ps2,l1, pf1(idv,:),pf2(idv,:),l2(idv));
        int_tmp_imag(idv) = int_fila_v_anal(ps1,ps2,l1, pf1_imag(idv,:),pf2_imag(idv,:),l2(idv));
    end
    % for arbitrary conductors
    if sum(ida) > 0
        Nftmp = sum(ida);
        dv_sign = ones(Nftmp,1);
        dv_sign(1:Nftmp) = sign( sum(dvtmp(ida,1:3).*dv2(ida,:),2) );
        int_tmp(ida) = dv_sign.*int_fila_a_anal(ps1,ps2,l1, pf1(ida,:),pf2(ida,:),l2(ida));

        dv_sign = ones(Nftmp,1);
        dv_sign(1:Nftmp) = sign( sum(dvtmp(ida,1:3).*dv_imag(ida,:),2) );
        int_tmp_imag(ida) = dv_sign.*int_fila_a_anal(ps1,ps2,l1, pf1_imag(ida,:),pf2_imag(ida,:),l2(ida));
    end
    
    %Pcol = cal_P_fila(ps1,ps2,dv1,l1,r1, pf1,pf2,dv2,l2,r2);
    int_mtx(1:ik,ik) = int_tmp;
    int_mtx_imag(1:ik,ik) = int_tmp_imag;
end
int_mtx = int_mtx+int_mtx'-diag(diag(int_mtx));
int_mtx_imag = int_mtx_imag+int_mtx_imag'-diag(diag(int_mtx_imag));


%% 3. generate 3D mutual Z matrix

Pgrid = cof1*1./(len*len').* (int_mtx + cof2*int_mtx_imag);

Ygrid = inv(Pgrid);
Ggrid = real(Ygrid);
Cgrid = imag(Ygrid)/w0;

end

