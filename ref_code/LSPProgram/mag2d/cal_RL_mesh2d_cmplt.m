function [Rmesh2D, Lmesh2D, Ce2D] = cal_RL_mesh2d_cmplt( ...
    p2Dcir, rocir, ricir, Rpucir, murcir, eprcir, ...
    p2Drec, wrec, hrec, Rpurec, murrec, eprrec, ...
    p2Dagi, wagi, tagi, Rpuagi, muragi, epragi, ...
    p2Dspt, rospt, rispt, Rpuspt, murspt, eprspt, lgrp, f0)
%  Function:       para_group_mesh2D
%  Description:    Meshing model is only applicable for aligned conductors.
%                  This version of program can simulate from DC to 2MHz.
%                  Calculation effiency is low so that do not attempt to
%                  simulate more than conductor     s.
%
%  Calls:          mesh2d_L_cir
%                  mesh2D_L_box
%                  mesh2D_L_agi
%                  mesh2D_L_spt
%
%  Input:          p2Dcir  --  point offset in 2D view (N*2) (m)
%                  rocir   --  rout (N*1) (m)
%                  ricir   --  rin (N*1) (m)
%                  Rpucir  --  resistance of conductors (N*1) (ohm/m)
%                  Sicir   --  conductivity of conductors (N*1) (Sig/m)
%
%                  p2Drec  --  point offset in 2D view (N*2) (m)
%                  wrec    --  width (N*1) (m)
%                  hrec    --  height (N*1) (m)
%                  Rpurec  --  resistance of conductors (N*1) (ohm/m)
%                  Sirec   --  conductivity of conductors (N*1) (Sig/m)
%
%                  p2Dagi  --  point offset in 2D view (N*2) (m)
%                  wagi    --  width (N*1) (m)
%                  tagi    --  thick (N*1) (m)
%                  Rpuagi  --  resistance of conductors (N*1) (ohm/m)
%                  Siagi   --  conductivity of conductors (N*1) (Sig/m)
%
%                  p2Dspt  --  point offset in 2D view (N*2) (m)
%                  rosp    --  rout (N*1) (m)
%                  risp    --  rin (N*1) (m)
%                  Rpusp   --  resistance of conductors (N*1) (ohm/m)
%                  Sisp    --  conductivity of conductors (N*1) (Sig/m)
%
%                  lgrp    --  length of the conductors (1*1) (m)
%                  f0      --  frequency
%
%  Output:         Rmtx    --  R matrix per unit (ohm/m)
%                  Lmtx    --  L matrix per unit (H/m)
%  Others:
%  Author:         Chen Hongcai
%  Email :         hc.chen@live.com
%  Date:           2014-4-18
%  Update:


mu0 = 4*pi*1e-7;

%% 1. Generate the Paramenters of Segments
%disp('Meshing the cross section of devices...');

% 1.a circular segments' parameters calculating
Nccir = size(p2Dcir,1);
if Nccir <= 0
    Xsa=zeros(0,1); Ysa=zeros(0,1); dSa=zeros(0,1); Rpsa=zeros(0,1); ...
        NVsa=zeros(0,1); rs1a=zeros(0,1); rs2a=zeros(0,1); as1a=zeros(0,1); as2a=zeros(0,1);
else
    fmesh = (max(murcir))*f0;

    [Xsa, Ysa, rs1a, rs2a, as1a, as2a, dSa, Rpsa, NVsa] = mesh2d_L_cir(p2Dcir, ...
        rocir, ricir, Rpucir, murcir, fmesh);
end

Natmp = sum(NVsa,1);
Na = (Natmp(1));
MagID = [];
for i = 1: Nccir
    MagIDtemp = ones(NVsa(i),1)*murcir(i);
    MagID = [MagID;MagIDtemp];
end
idcir_mag = find(MagID>1);
Ncir_mag = size(idcir_mag,1);


% 1.b reactangle segments' parameters calculating
Ncrec = size(p2Drec,1);
if Ncrec <= 0
    Xsr=zeros(0,1); Ysr=zeros(0,1); dSr=zeros(0,1); Rpsr = zeros(0,1); ...
        NVsr=zeros(0,1); ws=zeros(0,1); hs=zeros(0,1);
else
    [Xsr,Ysr, ws,hs, dSr, Rpsr, NVsr] = mesh2d_L_box(p2Drec, wrec,hrec, ...
        Rpurec, f0);
    % Rpsr = Rpsr.*sqrt(murrec);
end

%Nr = sum(NVsr);
Nrtmp = sum(NVsr);
Nr = Nrtmp(1);
MagID = [];
for i = 1: Ncrec
    MagIDtemp = ones(NVsr(i),1)*murrec(i);
    MagID = [MagID;MagIDtemp];
end
idrec_mag = find(MagID>1);
Nrec_mag = size(idrec_mag,1);


% 1.c Angle iron segments' parameters calculating
Ncagi = size(p2Dagi,1);
if Ncagi <= 0
    Xsl=zeros(0,1); Ysl=zeros(0,1); dSl=zeros(0,1); Rpsl=zeros(0,1); ...
        NVsl=zeros(0,1); wl=zeros(0,1); hl=zeros(0,1);
else
    [Xsl, Ysl, wl, hl, dSl, Rpsl, NVsl] = mesh2d_L_agi(p2Dagi, ...
        wagi, tagi, Rpuagi, f0);
end

Nltmp = sum(NVsl);
Nl = Nltmp(1);
MagID = [];
for i = 1: Ncagi
    MagIDtemp = ones(NVsl(i),1)*muragi(i);
    MagID = [MagID;MagIDtemp];
end
idagi_mag = find(MagID>1);
Nagi_mag = size(idagi_mag,1);


% 1.d Single tube pipe segments' parameters calculating
Ncspt = size(p2Dspt,1);
if Ncspt <= 0
    Xsp=zeros(0,1); Ysp=zeros(0,1); dSp=zeros(0,1); Rpsp=zeros(0,1); ...
        NVsp=zeros(0,1); rs1p=zeros(0,1); rs2p=zeros(0,1); as1p=zeros(0,1); as2p=zeros(0,1);
else
    p2Doth = [p2Dcir; p2Drec; p2Dagi];
%     [Xsp, Ysp, rs1p, rs2p, as1p, as2p, dSp, Rpsp, NVsp] = mesh2d_L_spt(p2Dspt, ...
%         rospt, rispt, Rpuspt, p2Doth, f0);
    [Xsp, Ysp, rs1p, rs2p, as1p, as2p, dSp, Rpsp, NVsp] = mesh2d_L_spt_gauss(p2Dspt, ...
        rospt, rispt, Rpuspt, p2Doth, f0);
end

Nptmp = sum(NVsp,1);
Np = (Nptmp(1));
MagID = [];
for i = 1: Ncspt
    MagIDtemp = ones(NVsp(i),1)*murspt(i);
    MagID = [MagID;MagIDtemp];
end
idspt_mag = find(MagID>1);
Nspt_mag = size(idspt_mag,1);


% 1.e conbine the data
Nall = Na+Nr+Nl+Np;
NVall = cat(1, NVsa, NVsr, NVsl, NVsp);

Xfall = cat(1, Xsa, Xsr, Xsl, Xsp);
Yfall = cat(1, Ysa, Ysr, Ysl, Ysp);
%dS = cat(1, dSa, dSr, dSl);
Rps = cat(1, Rpsa, Rpsr, Rpsl, Rpsp);

Nc = size(NVall,1);


%% 2. Calculate the Inductance. Mesh in 2D cross section is the same.
%      the only difference is the length of each group. So mesh once and
%      calculate Ngrp times using different length.

if f0>1e6
    fprintf ('Calculation frequency is %.4f MHz.\n',f0/1e6);
elseif f0>1e3
    fprintf ('Calculation frequency is %.4f kHz.\n',f0/1e3);
else
    fprintf ('Calculation frequency is %.2f Hz.\n',f0);
end
fprintf ('Num. of segments is %d.\n\n',Nall);


Lmesh2D = zeros(Nc,Nc);
Rmesh2D = zeros(Nc,Nc);
Ce2D = zeros(Nc,Nc);

Mlh = zeros(Nall,Nall);
Dx = zeros(Nall,Ncir_mag);
Dy = zeros(Nall,Ncir_mag);
Txx = zeros(Ncir_mag,Ncir_mag);
Txy = zeros(Ncir_mag,Ncir_mag);
Tyy = zeros(Ncir_mag,Ncir_mag);
Ce_tmp = zeros(Nall,Nall);

% 2.a caculate the inductances for annulus segments
if Nccir > 0

    cnt1 = 0;
    
    for k = 1 : Nccir
        
        [r0f, a0f] = angle_point2d(p2Dcir(k,:), [Xfall Yfall]);
        
        for g = 1:NVsa(k)
            
            cnt1 = cnt1+1;
            % the distance between the main cable and others
            d0f = sqrt( (Xsa(cnt1)-Xfall).^2 + (Ysa(cnt1)-Yfall).^2);
            
            %ind0 = d0f < cof_eff_area*rocir(k);
            % change to vecters for speeding
            r1 = rs1a(cnt1);
            r2 = rs2a(cnt1);
            ang1 = as1a(cnt1);
            ang2 = as2a(cnt1);
            
%             Mlh(ind0,cnt0) = mu0/(4*pi)./dSa(cnt0).* ...
%                 int_anl_fila_p3d(ang1,ang2,r1,r2, r0f(ind0),a0f(ind0), d0f(ind0), lgrp, 6);
            
            Mtmp = mu0/(4*pi)./dSa(cnt1).* ...
                int_anl_fila_p3d(ang1,ang2,r1,r2, r0f,a0f, d0f, lgrp, 6);
            Mlh(:,cnt1) = Mtmp;
            
% mutual inductance caused by magnetic current   
            if find(cnt1==idcir_mag)
                id_oth = setdiff(idcir_mag, cnt1);
                
                [DxzTmp, DyzTmp, TxxTmp, TxyTmp, TyyTmp] = int_anl_mag_p3d( ...
                    ang1, ang2, r1, r2, r0f(idcir_mag), a0f(idcir_mag), lgrp, 5, 6);
                Dx(cnt1,idcir_mag) = mu0/(4*pi)./dSa(cnt1) .* DxzTmp;
                Dy(cnt1,idcir_mag) = mu0/(4*pi)./dSa(cnt1) .* DyzTmp;
                Txx(cnt1,idcir_mag) = mu0/(4*pi)./dSa(cnt1) .* TxxTmp;
                Tyy(cnt1,idcir_mag) = mu0/(4*pi)./dSa(cnt1) .* TyyTmp;
                Txy(cnt1,idcir_mag) = mu0/(4*pi)./dSa(cnt1) .* TxyTmp;
                
                
%                 [DxzTmp, DyzTmp, TxxTmp, TxyTmp, TyyTmp] = int_anl_mag_p3d( ...
%                     ang1, ang2, r1, r2, r0f(cnt1), a0f(cnt1), lgrp, 5, 6);
%                 Dx(cnt1,cnt1) = mu0/(4*pi)./dSa(cnt1) .* DxzTmp;
%                 Dy(cnt1,cnt1) = mu0/(4*pi)./dSa(cnt1) .* DyzTmp;
%                 Txx(cnt1,cnt1) = mu0/(4*pi)./dSa(cnt1) .* TxxTmp;
%                 Tyy(cnt1,cnt1) = mu0/(4*pi)./dSa(cnt1) .* TyyTmp;
%                 Txy(cnt1,cnt1) = mu0/(4*pi)./dSa(cnt1) .* TxyTmp;
%                 
%                 [DxzTmp, DyzTmp, TxxTmp, TxyTmp, TyyTmp] = int_anl_mag_p3d_integral2(...
%                      ang1, ang2, r1, r2, r0f(id_oth), a0f(id_oth), lgrp);
%                 
%                 Dx(cnt1,id_oth) = mu0/(4*pi)./dSa(cnt1) .* DxzTmp;
%                 Dy(cnt1,id_oth) = mu0/(4*pi)./dSa(cnt1) .* DyzTmp;
%                 Txx(cnt1,id_oth) = mu0/(4*pi)./dSa(cnt1) .* TxxTmp;
%                 Tyy(cnt1,id_oth) = mu0/(4*pi)./dSa(cnt1) .* TyyTmp;
%                 Txy(cnt1,id_oth) = mu0/(4*pi)./dSa(cnt1) .* TxyTmp;
                
                Txx(cnt1,cnt1) = Txx(cnt1,cnt1) - (mu0*murcir(k)/(murcir(k)-1))./dSa(cnt1);
                Tyy(cnt1,cnt1) = Tyy(cnt1,cnt1) - (mu0*murcir(k)/(murcir(k)-1))./dSa(cnt1);
            end
           
        end
    end
%     Dx = Dx(:,idcir_mag);
%     Dy = Dy(:,idcir_mag);
%     Txx = Txx(idcir_mag,idcir_mag);
%     Tyy = Tyy(idcir_mag,idcir_mag);
%     Txy = Txy(idcir_mag,idcir_mag);
    id_epr = find(eprcir>1); 
    Ce_tmp(id_epr,id_epr) = diag(cap_die(dSa(id_epr), eprcir(id_epr), lgrp));
    
end

% 2.b caculate the inductances for rectangle segments
if Ncrec > 0
    cnt2 = 0;
    
    Mltmp = zeros(Nr,Nr);
    Lself = zeros(Nr,1);
    % self inductance of the rectangle segment
    Lself(1:Nr) = induct_bar_Grover(ws, hs, lgrp);
    
    if Nccir>0
        dr_tmp = cat(1,sqrt(dSa),0.8*sqrt(dSr));
    else
        dr_tmp = 0.8*sqrt(dSr);
    end
    
    for k = 1:Ncrec
        for g = 1:NVsr(k)
            cnt2 = cnt2+1;
            %for k = 1 : Nr
            %ind = 1:Na+k-1;
            
            % caculate the mutual inductance
            dr = max(dr_tmp(1:Na+cnt2-1),dr_tmp(Na+cnt2));
            Xo = Xsr(cnt2);
            Yo = Ysr(cnt2);
            Xf = Xfall(1:Na+cnt2-1);
            Yf = Yfall(1:Na+cnt2-1);
            
            % the distance between the main cable and others
%             d1f = sqrt( (Xo-Xf).^2 + (Yo-Yf).^2);
%             ind1 = d1f < cof_eff_area * sqrt(wrec(k)*hrec(k));
            
            Mlh(1:Na+cnt2-1,Na+cnt2) = mu0/(4*pi)*int_fila_p3d(Xo, Yo, Xf, Yf, lgrp, dr);
            %Mlh(ind1,Na+cnt1) = mu0/(4*pi)*int_fila_p3d(Xo, Yo, Xf(ind1), Yf(ind1), lgrp, dr);
            %             + lgrp*1/4*murrec );
        end
    end
    Mltmp(1:Nr,1:Nr) = Mlh( Na+(1:Nr), Na+(1:Nr) );
    Mlh( Na+(1:Nr), Na+(1:Nr) ) = ( Mltmp+Mltmp')+diag( Lself );
    
    id_epr = find(eprrec>1); 
    Ce_tmp(id_epr+Na,id_epr+Na) = diag(cap_die(dSr(id_epr), eprrec(id_epr), lgrp));

end

% 2.c caculate the inductances for angle iron segments
if Ncagi > 0
    cnt3 = 0;
    
    Mltmp = zeros(Nl,Nl);
    Lself = zeros(Nl,1);
    % self inductance of the rectangle segment
    Lself(1:Nl) = induct_bar_Grover(wl, hl, lgrp);
    if Nccir>0
        dr_tmp = cat(1,sqrt(dSa),0.8*sqrt(dSr),0.8*sqrt(dSl));
    else
        dr_tmp = 0.8*sqrt(dSl);
    end
    
    for k = 1 : Ncagi
        for g = 1:NVsl(k)
            
            cnt3 = cnt3+1;
            %ind = 1:Na+Nr+k-1;
            % caculate the mutual inductance
            dr = max(dr_tmp(1:Na+Nr+cnt3-1),dr_tmp(Na+Nr+cnt3));
            
            Xo = Xsl(cnt3);
            Yo = Ysl(cnt3);
            Xf = Xfall(1:Na+Nr+cnt3-1);
            Yf = Yfall(1:Na+Nr+cnt3-1);
            
            % the distance between the main cable and others
%             d2f = sqrt( (Xo-Xf).^2 + (Yo-Yf).^2);
%             ind2 = d2f < cof_eff_area * sqrt(2*abs(wagi(k)*tagi(k))-tagi(k)^2);
            
            Mlh(1:Na+Nr+cnt3-1,Na+Nr+cnt3) = mu0/(4*pi)*int_fila_p3d(Xo,Yo, Xf, Yf, lgrp, dr);
            %Mlh(ind2,Na+Nr+cnt2) = mu0/(4*pi)*int_fila_p3d(Xo,Yo, Xf(ind2), Yf(ind2), lgrp, dr(ind2));
        end
    end
    
    Mltmp(1:Nl,1:Nl) = Mlh( Na+Nr+(1:Nl), Na+Nr+(1:Nl) );
    Mlh( Na+Nr+(1:Nl), Na+Nr+(1:Nl) ) = ( Mltmp+Mltmp')+diag( Lself );
    
    id_epr = find(epragi>1); 
    Ce_tmp(id_epr+Na+Nr,id_epr+Na+Nr) = diag(cap_die(dSl(id_epr), epragi(id_epr), lgrp));
end


% 2.d caculate the inductances for annulus segments of single tube tower
if Ncspt > 0
    cnt4 = 0;
    
    Mltmp = zeros(Np,Np);
    for k = 1 : Ncspt
        
        [r0f, a0f] = angle_point2d(p2Dspt(k,:), [Xfall Yfall]);
        
        for g = 1:NVsp(k)
            
            cnt4 = cnt4+1;
            % the distance between the main cable and others
            d0f = sqrt( (Xsp(cnt4)-Xfall).^2 + (Ysp(cnt4)-Yfall).^2);

            % change to vecters for speeding
            r1 = rs1p(cnt4);
            r2 = rs2p(cnt4);
            ang1 = as1p(cnt4);
            ang2 = as2p(cnt4);

%             Mlh(1:Na+Nr+Nl+cnt4,Na+Nr+Nl+cnt4) = mu0/(4*pi)./dSp(cnt4).* ...
%                 int_anl_fila_p3d(ang1,ang2,r1,r2, r0f,a0f, d0f, lgrp, 8);
            Mlh(:,Na+Nr+Nl+cnt4) = mu0/(4*pi)./dSp(cnt4).* ...
                int_anl_fila_p3d(ang1,ang2,r1,r2, r0f,a0f, d0f, lgrp, 8);
        end
    end
    
%     Mltmp(1:Np,1:Np) = Mlh( Na+Nr+Nl+(1:Np), Na+Nr+Nl+(1:Np) );
%     Mlh( Na+Nr+Nl+(1:Np), Na+Nr+Nl+(1:Np) ) = ( Mltmp+Mltmp')+diag( Lself );

    id_epr = find(eprspt>1);
    Ce_tmp(id_epr+Na+Nr+Nl,id_epr+Na+Nr+Nl) = diag(cap_die(dSp(id_epr), eprspt(id_epr), lgrp));
end



%% 3. merge the impedance of the segments to one
w0 = 2*pi*f0;

NVoff = 0;
sel_mtx = zeros(Nc,Nall);
for ik = 1:Nc
    sel_mtx(ik,NVoff+(1:NVall(ik))) = 1;
    NVoff = NVoff+NVall(ik);
end

% symmetry the inductance
Zl = ( diag(Rps)*lgrp + 1j*w0*((Mlh+Mlh')/2) );

% Txx =  cof*I22./dS(ik)-(E*km)./dS(ik);
%             Tyy =  cof*I33./dS(ik)-(E*km)./dS(ik);
%             Dxz = -cof*I21./dS(ik);
%             Dyz =  cof*I31./dS(ik);
%             Txy =  cof*I23./dS(ik);
%             % G32=G23;
%             G11 =  diag(Rs)+1j*w0*cof*Lint./dS(ik);
            if ~isempty(find(murcir>1, 1))      
                
%                 Txy=Txy-diag(diag(Txy));
%                 Txx=-Txy;
                Qx = 1j*w0*Dx;
                Qy = 1j*w0*Dy;
                Gt1 = Txy/Tyy;
                Gt2 = Txy/Txx;
%                 Zl = Zl + ( Qx.*(Tyy.*Dx-Txy.*Dy) + Qy.*(Txx.*Dy-Txy.*Dx) );
%                 Zl = Zl + Qx/(Gt1*Txy-Txx)*(Dx-Gt1*Dy) + Qy/(Gt2*Txy-Tyy)*(Dy-Gt2*Dx);
                Mmag = Qx/(Gt1*Txy-Txx)*(Dx-Gt1*Dy)...
                    + Qy/(Gt2*Txy-Tyy)*(Dy-Gt2*Dx);
                Zl = Zl + Mmag;
            end
%tic
Yc = sel_mtx/Zl*sel_mtx';
%toc
%disp(['Cable group calculation at ',num2str(f0),' Hz -- inversr method']);


% tic
% %
% tol = 1e-10*w0;
% maxit = 50;
% YG = zeros(Nall,Nall);
% [L,U] = lu(Zl);
%
% for ik = 1:Nall
%     b = zeros(Nall,1);
%     b(ik) = 1;
%
% %     [x0] = pcg(Zl,b,tol,maxit);
%     [x0,fl0,rr0,it0,rv0] = gmres(Zl,b,10,tol,maxit,P);
%     %[x0] = tfqmr(A,b,tol,maxit,P);
%     %[x0,relres,iter,resvec] = mpgmres(A,b,P,'trunc',tol,maxit);
%
% %     Iseg = zeros(Nall,1);
% %     Iseg(1:Nall) = x0;
%
%     YG(:,ik) = x0;
% end

% YcG =  sel_mtx*YG*sel_mtx';
% toc
% disp(['Cable group calculation at ',num2str(f0),' Hz -- eigenvalue method']);


Zmtx = complex(zeros(Nc,Nc));
Zmtx(1:Nc,1:Nc) = inv(Yc(1:Nc,1:Nc));

Rmesh2D(1:Nc,1:Nc) = real(Zmtx(1:Nc,1:Nc));
Lmesh2D(1:Nc,1:Nc) = imag(Zmtx(1:Nc,1:Nc))./w0;

% figure;plot(Xfall,Yfall,'.');axis equal
% 
% Yl = inv(Zl);
% figure;surf(Xfall,Yfall,abs(Yl));axis equal


end



