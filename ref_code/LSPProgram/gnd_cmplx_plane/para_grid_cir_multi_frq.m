function Pgrid = para_grid_cir_multi_frq(pt_start,pt_end, dv,re,len, sig_soil,epr_soil, frq)
%  Function:       grid_cir_multi_frq
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

type_solver = 0;

% if isempty(pt_start)
%     Ggm = zeros(0,1);
%     Cgm = zeros(0,1);
%     
%     return;
% end

ep0 = 8.85*1e-12;
ERR = 1e-3;

Nc = size(pt_start,1);
Nfrq = length(frq);

Pgrid = zeros(Nc,Nc,Nfrq);
% Ggm = zeros(Nc,Nc);
% Cgm = zeros(Nc,Nc);

pt_start_imag = pt_start;
pt_start_imag(:,3) = -pt_start_imag(:,3);
pt_end_imag = pt_end;
pt_end_imag(:,3) = -pt_end_imag(:,3);

%% 1. calculate the coefficients for mutual Z computation
w0 = 2*pi*frq;

if type_solver == 2
    % frequency dependent soil
    sigw = sig_soil*(1 + (1.2e-6*sig_soil^(-0.73)).*(frq-100).^0.65);
    sigw = max(sig_soil,sigw);
    eprw = 1.3+7.6e3*frq.^(-0.4);
    eprw = min(1205, eprw); % 1205 is the eprw at 100Hz
    
    cof1 = 1./( 4*pi.*(sigw+1j*w0.*eprw*ep0) );
    cof2 = (sigw+1j*w0.*(eprw-1)*ep0) ./ (sigw+1j*w0.*(eprw+1)*ep0);
else
    cof1 = 1./( 4*pi.*(sig_soil+1j*w0.*epr_soil*ep0) );
    cof2 = (sig_soil+1j*w0.*(epr_soil-1)*ep0) ./ (sig_soil+1j*w0.*(epr_soil+1)*ep0);
end

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
        int_tmp(ida) = dv_sign.*int_fila_a_anal_c(ps1,ps2,l1, pf1(ida,:),pf2(ida,:),l2(ida));

        dv_sign = ones(Nftmp,1);
        dv_sign(1:Nftmp) = sign( sum(dvtmp(ida,1:3).*dv_imag(ida,:),2) );
        int_tmp_imag(ida) = dv_sign.*int_fila_a_anal_c(ps1,ps2,l1, pf1_imag(ida,:),pf2_imag(ida,:),l2(ida));
    end
    
    %Pcol = cal_P_fila(ps1,ps2,dv1,l1,r1, pf1,pf2,dv2,l2,r2);
    int_mtx(1:ik,ik) = int_tmp;
    int_mtx_imag(1:ik,ik) = int_tmp_imag;
end
int_mtx = int_mtx+int_mtx'-diag(diag(int_mtx));
int_mtx_imag = int_mtx_imag+int_mtx_imag'-diag(diag(int_mtx_imag));


%% 3. generate 3D mutual Z matrix
for ig = 1:Nfrq
    Pgrid(:,:,ig) = cof1(ig)*1./(len*len').* (int_mtx + cof2(ig)*int_mtx_imag);
end


%% 4. generate mutual R and C matrix on spicified frequency
% if Nfrq>1
%     
%     % R value under 500Hz is perfered to simulate 8-20us case
%     fi = 500;
%     ind = find(frq == fi, 1);
%     Nf = length(frq);
%     if ~isempty(ind)
%         Ggm = 1./real(Pgm(:,:,ind));
%     elseif frq(Nf)<fi
%         Ggm = 1./real(Pgm(:,:,Nf));
%     else
%         wtmp = 2*pi*fi;
%         
%         c1_tmp = 1./( 4*pi.*(sig_soil+1j*wtmp.*epr_soil*ep0) );
%         c2_tmp = (sig_soil+1j*wtmp.*(epr_soil-1)*ep0) ./ (sig_soil+1j*wtmp.*(epr_soil+1)*ep0);
%         
%         Ptmp = c1_tmp*1./(len*len').* (int_mtx + c2_tmp*int_mtx_imag);
%         Ggm = real(inv(Ptmp));
%     end
%     %Rgm = 1./Rgm;
%     
%     % % L value under 20kHz is perfered to simulate 8-20us case
%     fi = 20e3;
%     ind = find(frq == fi, 1);
%     
%     if ~isempty(ind)
%         Cgm = imag(inv(Pgm(:,:,ind)))./(2*pi*frq(ind));
%     elseif frq(Nf)<fi
%         Cgm = imag(inv(Pgm(:,:,Nf)))./(2*pi*frq(Nf));
%     else
%         wtmp = 2*pi*fi;
%         
%         c1_tmp = 1./( 4*pi.*(sig_soil+1j*wtmp.*epr_soil*ep0) );
%         c2_tmp = (sig_soil+1j*wtmp.*(epr_soil-1)*ep0) ./ (sig_soil+1j*wtmp.*(epr_soil+1)*ep0);
%         
%         Ptmp = c1_tmp*1./(len*len').* (int_mtx + c2_tmp*int_mtx_imag);
%         Cgm = -imag(inv(Ptmp))./(2*pi*fi);
%     end
%     
% else
%     
%     Ggm = 1./real(Pgm(:,:,1));
%     Cgm = imag(inv(Pgm(:,:,1)))./(2*pi*frq);
% 
% end

end

