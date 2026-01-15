% SOL_M0    main program for solving J, Mx, My in a magnetic plate using
%           METHOD 0 (point-match)
%  W,T       % 铁板的宽度和厚度
% N, M      number of segements along x and y directions for the plate
% updated on July 8 2009
%
function [Rmag2D, Lmag2D]=cal_RL_rec_mag_loop_2d(shape, W, T, Nw,Nt, Rdc, mur, frq)


mu0=4e-7*pi;
cof = 2e-7;
dd = 2;

Nf = length(frq);
Nc = length(W);

dS = zeros(Nc,1);
Rmag2D = zeros(Nc,Nf);
Lmag2D = zeros(Nc,Nf);
for ik = 1:Nc
    % shape: 0-rectangle  1-round   2-annular
    if shape(ik) == 1 || shape(ik) == 2
        T = W;
        Nt = Nw;
    end

    Ns = Nw*Nt;
    
    dW = W(ik)/Nw;
    dT = T(ik)/Nt;
    dS(ik) = dW.*dT;
    
    % mesh the rectangle
    wsum = -W(ik)/2+(0:Nw)*dW;                             % x坐标数组
    tsum = -T(ik)/2+(0:Nt)*dT;                           % y坐标数组
    
    Sxy = zeros(Ns,4);
    Oxy1 = zeros(Ns,2);
    for ig=1:Nt
        idx = (1:Nw)+(ig-1)*Nw;
        
        Sxy(idx,1) = wsum(1:Nw);
        Sxy(idx,3) = wsum(2:Nw+1);
        Sxy(idx,2) = tsum(ig);
        Sxy(idx,4) = tsum(ig+1);
        % Z11
        Oxy1(idx,1) = 0.5*(wsum(1:Nw)+wsum(2:Nw+1));
        Oxy1(idx,2) = 0.5*(tsum(ig)+tsum(ig+1));
        
    end
    
    % shape: 0-rectangle  1-round   2-annular
    if shape == 0
        Oxy2 = zeros(Ns,2);
        % Z12 - return conductor is 2m far away
        Oxy2(:,1) = Oxy1(:,1)+dd;
        Oxy2(:,2) = Oxy1(:,2);
    elseif shape == 1 
        r0 = sqrt(sum(Oxy1.^2,2));
        id_r = r0<W/2;
        
        Sxytmp = Sxy(id_r,:);
        Oxytmp = Oxy1(id_r,:);
        
        Ns = size(Sxytmp,1);
        Oxy2 = zeros(Ns,2);
        
        Sxy = Sxytmp;
        Oxy1 = Oxytmp;

        % Z12 - return conductor is 2m far away
        Oxy2(:,1) = Oxytmp(:,1)+dd;
        Oxy2(:,2) = Oxytmp(:,2);
    elseif shape == 2
        r0 = sqrt(sum(Oxy1.^2,2));
        id_r = (r0<W/2) && (r0>T/2);
        
        Sxytmp = Sxy(id_r,:);
        Oxytmp = Oxy1(id_r,:);
        
        Ns = size(Sxytmp,1);
        Oxy2 = zeros(Ns,2);
        
        Sxy = Sxytmp;
        Oxy1 = Oxytmp;

        % Z12 - return conductor is 2m far away
        Oxy2(:,1) = Oxytmp(:,1)+dd;
        Oxy2(:,2) = Oxytmp(:,2);
    end
    
    Rs = Rdc(ik)*Ns*ones(Ns,1);
        
    if mur(ik)==1
%         Lint = int_z_2d(Sxy, Oxy1);
%         Mint = int_z_2d(Sxy, Oxy2);
        
        T11 = int_z_2d(Sxy, [Oxy1; Oxy2]);
        I11 = [ T11, [T11(Ns+(1:Ns),:)'; T11(1:Ns,:)] ];
         % I11 - L integral
        for ih = 1:Nf
            w0 = 2*pi*frq(ih);
            
            GI = diag([Rs; Rs])+1j*w0*cof*I11/dS(ik);
            
%             Zmtx = [GI  1j*w0*cof*Mint'/dS(ik); ...
%                 1j*w0*cof*Mint/dS(ik)  GI ];
            Yl = inv(GI);
            
            Yl_tmp = zeros(2,Ns*2);
            Yc = zeros(2,2);
            for k = 1 : 2
                Yl_tmp(k,:) = sum( Yl((k-1)*Ns+(1:Ns),:), 1 );
                for g = 1 : 2
                    Yc(k,g) = sum( Yl_tmp(k, (g-1)*Ns+(1:Ns)), 2);
                end
            end
            
            Z2x2 = inv(Yc);
            
            Rmag2D(ik,ih) = real(Z2x2(1,1));
            Lmag2D(ik,ih) = imag(Z2x2(1,1)-Z2x2(1,2))./w0;
        end
        
    else
        [I31, I21, I23, I22, I33, I11] = int_z_m_2d(Sxy, Oxy1);
        Mint = int_z_2d(Sxy, Oxy2);
        
%         [T31, T21, T23, T22, T11] = int_z_m_2d(Sxy, [Oxy1; Oxy2]);
         
%         I31 = [ mur*T31(1:Ns,:),  T31(Ns+(1:Ns),:)'; ...
%                 T31(Ns+(1:Ns),:), mur*T31(1:Ns,:) ];
%         I21 = [ mur*T21(1:Ns,:),  T21(Ns+(1:Ns),:)';...
%                 T21(Ns+(1:Ns),:), mur*T21(1:Ns,:) ];
%         I23 = [ mur*T23(1:Ns,:),  T23(Ns+(1:Ns),:)'; ...
%                 T23(Ns+(1:Ns),:), mur*T23(1:Ns,:) ];
%         I22 = [ mur*T22(1:Ns,:)   T22(Ns+(1:Ns),:)'; ...
%                 T22(Ns+(1:Ns),:), mur*T22(1:Ns,:) ];
%         I11 = [ mur*T11(1:Ns,:),  T11(Ns+(1:Ns),:)'; ...
%                 T11(Ns+(1:Ns),:), mur*T11(1:Ns,:) ];
%         E = eye(Ns*2,Ns*2);

        % I11 - L integral
        E = eye(Ns,Ns);
        km = mu0*mur/(mur-1);      % km=mu0*mur/(mur-1)/(mu0/2*pi)  
        
        G22 =  cof*I22./dS(ik)-(E*km)./dS(ik);
        G33 =  cof*I33./dS(ik)-(E*km)./dS(ik);
        G21 = -cof*I21./dS(ik);
        G31 =  cof*I31./dS(ik);
        G23 =  cof*I23./dS(ik);
        % G32=G23;
        
        for ih = 1:Nf
            w0 = 2*pi*frq(ih);
            
            G11 =  diag([Rs]) + 1j*w0*cof*I11/dS(ik);
            G12 =  1j*w0*cof*I21/dS(ik);
            G13 = -1j*w0*cof*I31/dS(ik);
            
            Gtmp2 = G23/G22; 
            Gtmp3 = G23/G33; 
            GI = G11 + G12/(Gtmp3*G23-G22)*(G21-G31-Gtmp3*G31) ...
                 + G13/(Gtmp2*G23-G33)*(G31-Gtmp2*G21);
             
            Zmtx = [GI    1j*w0*cof*Mint'/dS(ik); ...
                    1j*w0*cof*Mint/dS(ik)    GI ];
            Yl = inv(Zmtx);

            Yl_tmp = zeros(2,Ns*2);
            Yc = zeros(2,2);
            for k = 1 : 2
                Yl_tmp(k,:) = sum( Yl((k-1)*Ns+(1:Ns),:), 1 );
                for g = 1 : 2
                    Yc(k,g) = sum( Yl_tmp(k, (g-1)*Ns+(1:Ns)), 2);
                end
            end
            
            Z2x2 = inv(Yc);
            
            Rmag2D(ik,ih) = real(Z2x2(1,1));
            Lmag2D(ik,ih) = imag(Z2x2(1,1)-Z2x2(1,2))./w0;
        end
    end
    
end


end


