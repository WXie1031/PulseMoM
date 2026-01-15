function [Pmesh2D] = cal_P_mesh2d_cmplt(p2Dcir,rocir,ricir, eprcir, ...
    p2Drec,wrec,hrec, eprrec,  p2Dagi,wagi,tagi, epragi, ...
    p2Dspt,rospt,rispt, eprspt, lgrp)
%  Function:       cal_P_mesh2d_cmplt
%  Description:    Meshing model is only applicable for aligned conductors.
%
%  Calls:          mesh2d_P_cir
%                  mesh2D_P_box
%                  mesh2D_P_agi
%                  mesh2D_P_spt
%
%  Input:          p2Dcir  --  point offset in 2D view (N*2) (m)
%                  rocir   --  rout (N*1) (m)
%                  ricir   --  rin (N*1) (m)
%
%                  p2Drec  --  point offset in 2D view (N*2) (m)
%                  wrec    --  width (N*1) (m)
%                  hrec    --  height (N*1) (m)
%
%                  p2Dagi  --  point offset in 2D view (N*2) (m)
%                  wagi    --  width (N*1) (m)
%                  tagi    --  thick (N*1) (m)
%
%                  p2Dspt  --  point offset in 2D view (N*2) (m)
%                  rosp    --  rout (N*1) (m)
%                  risp    --  rin (N*1) (m)
%
%                  lgrp    --  length of the conductors (1*1) (m)
%
%  Output:         Pmesh2D    --  P matrix (S/m)
%  Others:
%  Author:         Chen Hongcai
%  Email :         hc.chen@live.com
%  Date:           2018-06-03
%  History:


ep0 = 8.85*1e-12;

%% 1. Generate the Paramenters of Segments
%disp('Meshing the cross section of devices...');

% 1.a circular segments' parameters calculating
Nccir = size(p2Dcir,1);
if Nccir <= 0
    Xsa=zeros(0,1); Ysa=zeros(0,1); NVsa=zeros(0,1); rsa=zeros(0,1); ...
        as1a=zeros(0,1); as2a=zeros(0,1);
else
    [Xsa, Ysa, rsa, as1a, as2a, NVsa] = mesh2d_P_cir(p2Dcir, rocir);
end

Natmp = sum(NVsa,1);
Na = (Natmp(1));


% 1.b reactangle segments' parameters calculating
Ncrec = size(p2Drec,1);
if Ncrec <= 0
    Xsr=zeros(0,1); Ysr=zeros(0,1); Xs1r=zeros(0,1); Xs2r=zeros(0,1);
    NVsr=zeros(0,1); whrs=zeros(0,1);
else
    [Xsr,Ysr, Xs1r,Xs2r, whrs, NVsr] = mesh2d_P_box(p2Drec, wrec,hrec);
end

%Nr = sum(NVsr);
Nrtmp = sum(NVsr);
Nr = Nrtmp(1);

% 1.c Angle iron segments' parameters calculating
Ncagi = size(p2Dagi,1);
if Ncagi <= 0
    Xsl=zeros(0,1); Ysl=zeros(0,1); NVsl=zeros(0,1); wl=zeros(0,1); hl=zeros(0,1);
else
    [Xsl, Ysl, wl, hl, dSl, NVsl] = mesh2d_P_agi(p2Dagi, ...
        wagi, tagi, Rpuagi, f0);
end

Nltmp = sum(NVsl);
Nl = Nltmp(1);
%Nl = sum(NVsl,1);

% 1.d Single tube pipe segments' parameters calculating
Ncspt = size(p2Dspt,1);
if Ncspt <= 0
    Xsp=zeros(0,1); Ysp=zeros(0,1); NVsp=zeros(0,1); rs1p=zeros(0,1); ...
        as1p=zeros(0,1); as2p=zeros(0,1);
else
    p2Doth = [p2Dcir; p2Drec; p2Dagi];
    [Xsp, Ysp, rs1p, as1p, as2p, NVsp] = mesh2d_P_spt(p2Dspt, ...
        rospt,  p2Doth);
end

Nptmp = sum(NVsp,1);
Np = (Nptmp(1));


% 1.e conbine the data
Nall = Na+Nr+Nl+Np;
NVall = cat(1, NVsa, NVsr, NVsl, NVsp);

Xfall = cat(1, Xsa, Xsr, Xsl, Xsp);
Yfall = cat(1, Ysa, Ysr, Ysl, Ysp);


Nc = size(NVall,1);


%% 2. Calculate the Inductance. Mesh in 2D cross section is the same.
%      the only difference is the length of each group. So mesh once and
%      calculate Ngrp times using different length.


fprintf ('Num. of segments is %d.\n\n',Nall);


Pmesh2D = zeros(Nc,Nc);
Plh = zeros(Nall,Nall);

% 2.a caculate the CoP for annulus segments
if Nccir > 0
    cnt1 = 0;
    
    dCa = abs(as1a-as2a).*rsa;
    
    for k = 1 : Nccir
        
        [r0f, a0f] = angle_point2d(p2Dcir(k,:), [Xfall Yfall]);
        
        for g = 1:NVsa(k)
            
            cnt1 = cnt1+1;
            % the distance between the main cable and others
            d0f = sqrt( (Xsa(cnt1)-Xfall).^2 + (Ysa(cnt1)-Yfall).^2);
            
            %ind0 = d0f < cof_eff_area*rocir(k);
            % change to vecters for speeding
            r1 = rsa(cnt1);
            ang1 = as1a(cnt1);
            ang2 = as2a(cnt1);

            Plh(:,cnt1) = 1/(4*pi*ep0)./dCa(k)./lgrp./lgrp .* ...
                int_arc_fila_p3d(ang1,ang2,r1, r0f,a0f, d0f, lgrp, 6);
        end
    end
end

% 2.b caculate the CoP for rectangle segments
if Ncrec > 0
    cnt2 = 0;
    
    Pltmp = zeros(Nr,Nr);
    Pself = zeros(Nr,1);
    % self inductance of the rectangle segment
    Pself(1:Nr) = cop_tape(whrs, lgrp);

    if Nccir>0
        dr_tmp = cat(1,sqrt(dCa),0.8*sqrt(whrs));
    else
        dr_tmp = 0.8*sqrt(whrs);
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
            
            Plh(1:Na+cnt2-1,Na+cnt2) = 1/(4*pi*ep0)./lgrp./lgrp .* ...
                int_fila_p3d(Xo, Yo, Xf, Yf, lgrp, dr);
        end
    end
    Pltmp(1:Nr,1:Nr) = Plh( Na+(1:Nr), Na+(1:Nr) );
    Plh( Na+(1:Nr), Na+(1:Nr) ) = ( Pltmp+Pltmp')+diag( Pself );
    %clear Mltmp Lself
end

% 2.c caculate the CoP for angle iron segments
if Ncagi > 0
    cnt3 = 0;
    
    Pltmp = zeros(Nl,Nl);
    Pself = zeros(Nl,1);
    % self inductance of the rectangle segment
    Pself(1:Nl) = cop_tape(wl, lgrp);
    if Nccir>0
        dr_tmp = cat(1,sqrt(dCa),0.8*sqrt(whrs),0.8*sqrt(dSl));
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
            
            Plh(1:Na+Nr+cnt3-1,Na+Nr+cnt3) = 1/(4*pi*ep0)./lgrp./lgrp .* ...
                int_fila_p3d(Xo,Yo, Xf, Yf, lgrp, dr);
        end
    end
    
    Pltmp(1:Nl,1:Nl) = Plh( Na+Nr+(1:Nl), Na+Nr+(1:Nl) );
    Plh( Na+Nr+(1:Nl), Na+Nr+(1:Nl) ) = ( Pltmp+Pltmp')+diag( Pself );
    
end


% 2.d caculate the CoP for arc segments of single tube tower
if Ncspt > 0
    cnt4 = 0;

    dCp = 2*pi*abs(as1p-as2p);
    
    for k = 1 : Ncspt
        
        [r0f, a0f] = angle_point2d(p2Dspt(k,:), [Xfall Yfall]);
        
        for g = 1:NVsp(k)
            
            cnt4 = cnt4+1;
            % the distance between the main cable and others
            d0f = sqrt( (Xsp(cnt4)-Xfall).^2 + (Ysp(cnt4)-Yfall).^2);

            % change to vecters for speeding
            r1 = rs1p(cnt4);
            ang1 = as1p(cnt4);
            ang2 = as2p(cnt4);

            Plh(:,Na+Nr+Nl+cnt4) = 1/(4*pi*ep0)./dCp(k)./lgrp./lgrp.* ...
                int_arc_fila_p3d(ang1,ang2,r1, r0f,a0f, d0f, lgrp, 8);
        end
    end
    
%     Mltmp(1:Np,1:Np) = Mlh( Na+Nr+Nl+(1:Np), Na+Nr+Nl+(1:Np) );
%     Mlh( Na+Nr+Nl+(1:Np), Na+Nr+Nl+(1:Np) ) = ( Mltmp+Mltmp')+diag( Lself );
end



%% 3. merge the impedance of the segments to one
NVoff = 0;
sel_mtx = zeros(Nc,Nall);
for ik = 1:Nc
    sel_mtx(ik,NVoff+(1:NVall(ik))) = 1;
    NVoff = NVoff+NVall(ik);
end


Pmesh2D(1:Nc,1:Nc) = inv( sel_mtx/((Plh+Plh')/2)*sel_mtx' );
% Pmesh2D(1:Nc,1:Nc) = inv( sel_mtx/Plh*sel_mtx' );

% figure;plot(Xfall,Yfall,'.');axis equal

end



