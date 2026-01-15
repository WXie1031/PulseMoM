function [Rmtx, Lmtx] = cal_RL_mesh2d_method_cmp(p2Dcir, rocir, ricir, Rpucir, ...
    p2Drec, wrec, hrec, Rpurec,  ...
    p2Dagi, wagi, tagi, Rpuagi, lgrp, f0, pre_mtx)
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
    [Xsa, Ysa, rs1, rs2, as1, as2, dSa, Rpsa, NVsa] = mesh2d_L_cir(p2Dcir, ...
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
    [Xsr, Ysr, ws, hs, dSr, Rpsr, NVsr] = mesh2d_L_box(p2Drec, wrec, hrec, ...
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
    [Xsl, Ysl, wl, hl, dSl, Rpsl, NVsl] = mesh2d_L_agi(p2Dagi, ...
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
dS = cat(1, dSa, dSr, dSl);
Rps = cat(1, Rpsa, Rpsr, Rpsl);

Nc = size(NVall,1);

disp('Meshing is finished.');
%% 2. Calculate the Inductance. Mesh in 2D cross section is the same.
%      the only difference is the length of each group. So mesh once and
%      calculate Ngrp times using different length.
disp('Calculation segments parameter matrix.');
disp('Num. of segments is ');
disp(Nall);

%Ngrp = size(lgrp,1);
Lmtx = zeros(Nc,Nc);
Rmtx = zeros(Nc,Nc);


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
            
            
            Mlh(:,cnt) = mu0/(4*pi)./dS(cnt).* ...
                int_anl_fila_p3d(ang1, ang2, r1, r2, r0f, a0f, d0f, lgrp, 6);
        end
    end
end

% 2.b caculate the inductances for rectangle segments
if Ncrec > 0
    Mltmp = zeros(Nr,Nr);
    Lself = zeros(Nr,1);
    % self inductance of the rectangle segment
    Lself(1:Nr) = selfL_bar_Grover(ws, hs, lgrp);
    
    if Nccir>0
        dr_tmp = cat(1,sqrt(dSa),0.8*sqrt(dSr));
    else
        dr_tmp = 0.8*sqrt(dSr);
    end
    
    for k = 1 : Nr
        %ind = 1:Na+k-1;
        dr = max(dr_tmp(1:Na+k-1),dr_tmp(Na+k));
        % caculate the mutual inductance
        % INDUCT-(mH) u0/(4*pi) = 1e-7 (filament model)
        Xo = Xsr(k);
        Yo = Ysr(k);
        Xf = Xfall(1:Na+k-1);
        Yf = Yfall(1:Na+k-1);
        Mlh(1:Na+k-1,Na+k) = mu0/(4*pi)*int_fila_p3d(Xo, Yo, Xf, Yf, lgrp, dr);
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
    
    if Nccir>0
        dr_tmp = cat(1,sqrt(dSa),0.8*sqrt(dSr),0.8*sqrt(dSl));
    else
        dr_tmp = 0.8*sqrt(dSl);
    end
    
    for k = 1 : Nl
        %ind = 1:Na+Nr+k-1;
        dr = max(dr_tmp(1:Na+Nr+k-1),dr_tmp(Na+Nr+k));
        % caculate the mutual inductance
        % INDUCT-(mH) u0/(4*pi) = 1e-7 (filament model)
        Xo = Xsl(k);
        Yo = Ysl(k);
        Xf = Xfall(1:Na+Nr+k-1);
        Yf = Yfall(1:Na+Nr+k-1);
        Mlh(1:Na+Nr+k-1,Na+Nr+k) = mu0/(4*pi)*int_fila_p3d(Xo, Yo, Xf, Yf, lgrp, dr);
    end
    Mltmp(1:Nl,1:Nl) = Mlh( Na+Nr+(1:Nl), Na+Nr+(1:Nl) );
    Mlh( Na+Nr+(1:Nl), Na+Nr+(1:Nl) ) = ( Mltmp+Mltmp')+diag( Lself );
    
end



%% 3. merge the impedance of the segments to one

% symmetry the inductance
%Ml = (Mlh+Mlh')/2;


w0 = 2*pi*f0;

tic

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

disp('Inv time')

toc

tic
YlGuass = (diag(Rps)*lgrp + 1j*w0*((Mlh+Mlh')/2)) \ eye(Nall);

Yl_tmp = zeros(Nc,Nall);
YcGuass = zeros(Nc,Nc);

for k = 1 : Nc
    num_y = sum( NVall(1:k) )-NVall(k);
    Yl_tmp(k,:) = sum( Yl( num_y+1:num_y+NVall(k),:),1 );
    for g = 1 : Nc
        num_x = sum( NVall(1:g) )-NVall(g);
        YcGuass(k,g) = sum( Yl_tmp(k, num_x+1:num_x+NVall(g) ),2);
    end
end

disp('Guass time')

toc

disp('Test');

%% gmers
tic
A = diag(Rps)*lgrp + 1j*w0*((Mlh+Mlh')/2);

P = 1./(diag(Rps)*lgrp + 1j*w0*diag((diag(Mlh+Mlh')/2)));
[L,U] = ilu(sparse(A),struct('type','ilutp','droptol',1e-6));


tol = 1e-3;
maxit = 20;

YG = zeros(Nall,Nall);

for ik = 1:Nall
    
    b = zeros(Nall,1);
    b(ik) = 1;

%     [x0,fl0,rr0,it0,rv0] = gmres(A,b,10,tol,maxit,P);
    [x0,fl0,rr0,it0,rv0] = gmres(A,b,10,tol,maxit, L,U);
    %[x0] = tfqmr(A,b,tol,maxit,P);
    %[x0,relres,iter,resvec] = mpgmres(A,b,P,'trunc',tol,maxit);
    
    Iseg = zeros(Nall,1);
    Iseg(1:Nall) = P*x0;
    
    YG(:,ik) = Iseg;

end

Yl_tmp = zeros(Nc,Nall);
YcG = zeros(Nc,Nc);

for k = 1 : Nc
    num_y = sum( NVall(1:k) )-NVall(k);
    Yl_tmp(k,:) = sum( Yl( num_y+1:num_y+NVall(k),:),1 );
    for g = 1 : Nc
        num_x = sum( NVall(1:g) )-NVall(g);
        YcG(k,g) = sum( Yl_tmp(k, num_x+1:num_x+NVall(g) ),2);
    end
end

disp('GMRES time')
toc


tic

A = diag(Rps)*lgrp + 1j*w0*((Mlh+Mlh')/2);

YG = zeros(Nall,Nall);
YG = A\eye(Nall);


Yl_tmp = zeros(Nc,Nall);
YcG = zeros(Nc,Nc);

for k = 1 : Nc
    num_y = sum( NVall(1:k) )-NVall(k);
    Yl_tmp(k,:) = sum( Yl( num_y+1:num_y+NVall(k),:),1 );
    for g = 1 : Nc
        num_x = sum( NVall(1:g) )-NVall(g);
        YcG(k,g) = sum( Yl_tmp(k, num_x+1:num_x+NVall(g) ),2);
    end
end

disp('\ time')
toc



tic 

[V_L,D_L] = eig(1j*w0*((Mlh+Mlh')/2));
diag(Rps)*lgrp

ZmtxG = inv(YcGuass);

Zmtx = complex(zeros(Nc,Nc));
Zmtx(1:Nc,1:Nc) = inv(Yc(1:Nc,1:Nc));

RmtxG(1:Nc,1:Nc) = real(ZmtxG(1:Nc,1:Nc));
LmtxG(1:Nc,1:Nc) = imag(ZmtxG(1:Nc,1:Nc))./w0;

Rmtx(1:Nc,1:Nc) = real(Zmtx(1:Nc,1:Nc));
Lmtx(1:Nc,1:Nc) = imag(Zmtx(1:Nc,1:Nc))./w0;

disp('...')

disp('...')



end



