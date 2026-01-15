function [Rmesh2D, Lmesh2D] = cal_RL_mesh2d_complete_gmd(p2Dcir, rocir, ricir, Rpucir, murcir, ...
    p2Drec, wrec, hrec, Rpurec, murrec, ...
    p2Dagi, wagi, tagi, Rpuagi, muragi, lgrp, f0)
%  Function:       para_group_mesh2D
%  Description:    Meshing model is only applicable for aligned conductors.
%                  This version of program can simulate from DC to 2MHz.
%                  Calculation effiency is low so that do not attempt to
%                  simulate more than conductors.
%
%  Calls:          mesh2D_circular
%                  mesh2D_box
%                  mesh2D_angle_steel
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
%                  lgrp    --  length of the conductors (1*1) (m)
%                  f0      --  frequency
%
%  Output:         Rmtx    --  R matrix per unit (ohm/m)
%                  Lmtx    --  L matrix per unit (H/m)
%  Others:
%  Author:         Chen Hongcai
%  Email :         hc.chen@live.com
%  Date:           2014-4-18
%  History:

mu0 = 4*pi*1e-7;

%% 1. Generate the Paramenters of Segments
disp('Mesh the cross section of devices.');

% 1.a circular segments' parameters calculating
Nccir = size(p2Dcir,1);

if Nccir <= 0
    Xsa=zeros(0,1); Ysa=zeros(0,1); dSa=zeros(0,1); Rpsa=zeros(0,1); ...
        NVsa=zeros(0,1); rs1=zeros(0,1); rs2=zeros(0,1); as1=zeros(0,1); as2=zeros(0,1);
else
    [Xsa, Ysa, rs1, rs2, as1, as2, dSa, Rpsa, NVsa] = mesh2d_circular(p2Dcir, ...
        rocir, ricir, Rpucir, f0);
end

Natmp = sum(NVsa,1);
Na = (Natmp(1));


% 1.b reactangle segments' parameters calculating
Ncrec = size(p2Drec,1);
if Ncrec <= 0
    Xsr=zeros(0,1); Ysr=zeros(0,1); dSr=zeros(0,1); Rpsr = zeros(0,1); ...
        NVsr=zeros(0,1); ws=zeros(0,1); hs=zeros(0,1);
else
    [Xsr, Ysr, ws, hs, dSr, Rpsr, NVsr] = mesh2d_box(p2Drec, wrec, hrec, ...
        Rpurec, f0);
end

%Nr = sum(NVsr);
Nrtmp = sum(NVsr);
Nr = Nrtmp(1);

% 1.c Angle iron segments' parameters calculating
Ncagi = size(p2Dagi,1);
if Ncagi <= 0
    Xsl=zeros(0,1); Ysl=zeros(0,1); dSl=zeros(0,1); Rpsl=zeros(0,1); ...
        NVsl=zeros(0,1); wl=zeros(0,1); hl=zeros(0,1);
else
    [Xsl, Ysl, wl, hl, dSl, Rpsl, NVsl] = mesh2d_angle_steel(p2Dagi, ...
        wagi, tagi, Rpuagi, f0);
end

Nltmp = sum(NVsl);
Nl = Nltmp(1);
%Nl = sum(NVsl,1);

% 1.d conbine the data
Nall = Na+Nr+Nl;
NVall = cat(1, NVsa, NVsr, NVsl);

Xfall = cat(1, Xsa, Xsr, Xsl);
Yfall = cat(1, Ysa, Ysr, Ysl);
%dS = cat(1, dSa, dSr, dSl);
Rps = cat(1, Rpsa, Rpsr, Rpsl);

Nc = size(NVall,1);

disp('Meshing is finished.');
%% 2. Calculate the Inductance. Mesh in 2D cross section is the same.
%      the only difference is the length of each group. So mesh once and
%      calculate Ngrp times using different length.
disp('Calculation segments parameter matrix.');
disp('Num. of segments is ');
disp(Nall);


Lmesh2D = zeros(Nc,Nc);
Rmesh2D = zeros(Nc,Nc);

Mlh = zeros(Nall,Nall);

% 2.a caculate the inductances for annulus segments
if Nccir > 0
    cnt = 0;
    
    for k = 1 : Nccir
        
        [r0f, a0f] = angle_point2d(p2Dcir(k,:), [Xfall Yfall]);
        
        for g = 1:NVsa(k)
            
            cnt = cnt+1;
            % the distance between the main cable and others
            d0f = sqrt( (Xsa(cnt)-Xfall).^2 + (Ysa(cnt)-Yfall).^2);
            % change to vecters for speeding
            r1 = rs1(cnt);
            r2 = rs2(cnt);
            ang1 = as1(cnt);
            ang2 = as2(cnt);
            
            Mlh(:,cnt) = mu0/(4*pi)./dSa(cnt).* ...
                int_anl_fila_p3d(ang1, ang2, r1, r2, r0f, a0f, d0f, lgrp, 6);
        end
    end
end

% 2.b caculate the inductances for rectangle segments
if Ncrec > 0
    Mltmp = zeros(Nr,Nr);
    Lself = zeros(Nr,1);
    % self inductance of the rectangle segment
    Lself(1:Nr) = induct_bar_Grover(ws, hs, lgrp);

    Drec = zeros(Na+Nr,1);
    
    for k = 1 : Nr
        %ind = 1:Na+k-1;
        % caculate the mutual inductance
        Xo = Xsr(k);
        Yo = Ysr(k);
        Wo = ws(k);
        To = hs(k);
        
        Xf = Xsr(1:k-1);
        Yf = Ysr(1:k-1);
        Wf = ws(1:k-1);
        Tf = hs(1:k-1);

        %Mlh(1:Na+k-1,Na+k) = mu0/(4*pi)*int_line_p2d(Xo, Yo, Xf, Yf, lgrp, dr);
        Drec(1:Na) = sqrt((Xo-Xsa).^2+(Yo-Ysa).^2);
        Drec(Na+1:Na+k-1) = gmd_rec(Xo,Yo,Wo,To, Xf,Yf,Wf,Tf);
        
        Mlh(1:Na+k-1,Na+k) = induct_gmd(Drec(1:Na+k-1), lgrp);
    end
    Mltmp(1:Nr,1:Nr) = Mlh( Na+(1:Nr), Na+(1:Nr) );
    Mlh( Na+(1:Nr), Na+(1:Nr) ) = ( Mltmp+Mltmp')+diag( Lself );
    %clear Mltmp Lself
end

% 2.c caculate the inductances for angle iron segments
if Ncagi > 0
    Mltmp = zeros(Nl,Nl);
    Lself = zeros(Nl,1);
    % self inductance of the rectangle segment
    Lself(1:Nl) = induct_bar_Grover(wl, hl, lgrp);
    Dagi = zeros(Nall,1);
    
    for k = 1 : Nl
        %ind = 1:Na+Nr+k-1;
        % caculate the mutual inductance
        Xo = Xsl(k);
        Yo = Ysl(k);
        Wo = wl(k);
        To = hl(k);
        
        Xf = Xsl(1:k-1);
        Yf = Ysl(1:k-1);
        Wf = wl(1:k-1);
        Tf = hl(1:k-1);
        
        Dagi(1:Na+Nr) = sqrt((Xo-[Xsa;Xsr]).^2+(Yo-[Ysa;Ysr]).^2);
        Dagi(Na+Nr+1:Na+Nr+k-1) = gmd_rec(Xo,Yo,Wo,To, Xf,Yf,Wf,Tf);
        
        Mlh(1:Na+Nr+k-1,Na+Nr+k) = induct_gmd(Dagi(1:Na+Nr+k-1), lgrp);
        
        %Mlh(1:Na+Nr+k-1,Na+Nr+k) = mu0/(4*pi)*int_line_p2d(Xo, Yo, Xf, Yf, lgrp, dr);
    end
    Mltmp(1:Nl,1:Nl) = Mlh( Na+Nr+(1:Nl), Na+Nr+(1:Nl) );
    Mlh( Na+Nr+(1:Nl), Na+Nr+(1:Nl) ) = ( Mltmp+Mltmp')+diag( Lself );
    
end


%% 3. merge the impedance of the segments to one
% symmetry the inductance
%Ml = (Mlh+Mlh')/2;

w0 = 2*pi*f0;

Yl = inv( diag(Rps)*lgrp + 1j*w0*((Mlh+Mlh')/2) );

Yl_tmp = zeros(Nc,Nall);
Yc = zeros(Nc,Nc);

for k = 1 : Nc
    num_y = sum( NVall(1:k) )-NVall(k);
    Yl_tmp(k,:) = sum( Yl( num_y+1:num_y+NVall(k),:),1 );
    for g = 1 : Nc
        num_x = sum( NVall(1:g) )-NVall(g);
        Yc(k,g) = sum( Yl_tmp(k, num_x+1:num_x+NVall(g) ),2);
    end
end


Zmtx = complex(zeros(Nc,Nc));
Zmtx(1:Nc,1:Nc) = inv(Yc(1:Nc,1:Nc));

Rmesh2D(1:Nc,1:Nc) = real(Zmtx(1:Nc,1:Nc));
Lmesh2D(1:Nc,1:Nc) = imag(Zmtx(1:Nc,1:Nc))./w0;

%figure;plot(Xfall,Yfall,'.');axis equal

end



