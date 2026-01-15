function [Rmag2D, Lmag2D]=cal_RL_rec_mag_2d_test(shape, W, T, Nw,Nt, Rdc, mur, frq)
% SOL_M0    main program for solving J, Mx, My in a magnetic plate using
%           METHOD 0 (point-match)
%  W,T       % 铁板的宽度和厚度
% N, M      number of segements along x and y directions for the plate
% updated on July 8 2009
%

mu0 = 4e-7*pi;
cof = 2e-7;


Nf = length(frq);
Nc = length(W);

dS = zeros(Nc,1);
Rmag2D = zeros(Nc,Nf);
Lmag2D = zeros(Nc,Nf);
for ik = 1:Nc
    % shape: 0-rectangle  1-round   2-annular
    if shape(ik) == 1 || shape(ik) == 2
        Nt = Nw;
        
        dW = W(ik)/Nw;
        dT = W(ik)/Nt;
        dS(ik) = dW.*dT;
        
        % mesh
        wsum = -W(ik)/2+(0:Nw)*dW;     % x坐标数组
        tsum = -W(ik)/2+(0:Nt)*dT;     % y坐标数组
    else
        dW = W(ik)/Nw;
        dT = T(ik)/Nt;
        dS(ik) = dW.*dT;
        
        % mesh
        wsum = -W(ik)/2+(0:Nw)*dW;     % x坐标数组
        tsum = -T(ik)/2+(0:Nt)*dT;     % y坐标数组
    end
    
    Ns = Nw*Nt;
    
    Sxy = zeros(Ns,4);
    Oxy = zeros(Ns,2);
    for ig=1:Nt
        idx = (1:Nw)+(ig-1)*Nw;
        
        Sxy(idx,1) = wsum(1:Nw);
        Sxy(idx,3) = wsum(2:Nw+1);
        Sxy(idx,2) = tsum(ig);
        Sxy(idx,4) = tsum(ig+1);
        
        Oxy(idx,1) = 0.5*(wsum(1:Nw)+wsum(2:Nw+1));
        Oxy(idx,2) = 0.5*(tsum(ig)+tsum(ig+1));
    end
    
    r0 = sqrt(sum(Oxy.^2,2));
    
    % shape: 0-rectangle  1-round   2-annular
    if shape == 0
        %             sig = Rdc/(W(ik)*T(ik));
        %             s_dep = sqrt(2./w0.*sig*mu0*mur(ik)) * ones(Nc,1);
        
        %             if 80*s_dep<max(W(ik),T(ik))
        %                 id_r = ((r0<W(ik)/2)|(r0<T(ik)/2)) & (r0>=22*s_dep);
        %                 Sxytmp = Sxy(id_r,:);
        %                 Oxytmp = Oxy(id_r,:);
        %             else
        Sxytmp = Sxy;
        Oxytmp = Oxy;
        %             end
        
    elseif shape == 1
        %             sig = Rdc/(pi*(W(ik)^2)/4);
        %             s_dep = sqrt(2./w0.*sig*mu0*mur(ik)) * ones(Nc,1);
        
        %             if 40*s_dep<W(ik)/2
        %                 id_r = (r0<W(ik)/2) & (r0>=40*s_dep);
        %             else
        id_r = (r0<W(ik)/2);
        %             end
        Sxytmp = Sxy(id_r,:);
        Oxytmp = Oxy(id_r,:);
    elseif shape == 2
        %             sig = Rdc/(pi*(W(ik)^2-T(ik)^2)/4);
        %             s_dep = sqrt(2./w0.*sig*mu0*mur(ik)) * ones(Nc,1);
        
        %             if 40*s_dep>T(ik)/2
        %                 id_r = (r0<W(ik)/2) & (r0>T(ik)/2) & (r0>40*s_dep);
        %             else
        id_r = (r0<W(ik)/2) & (r0>T(ik)/2);
        %             end
        
        Sxytmp = Sxy(id_r,:);
        Oxytmp = Oxy(id_r,:);
    end
    
    Ns = size(Sxytmp,1);
    
    
    for ih = 1:Nf
        w0 = 2*pi*frq(ih);
        
        sig = 1./(Rdc.*W.*T);
        Rskin = resis_bar_ac(W, T, Rdc, sig, 16, 1, frq(ih));
        %Lskin = induct_bar_ac(W, T, sig, 16, 1, frq(ih));
        Rs = Rskin*Ns*ones(Ns,1);
        
        if mur(ik)==1
            Lint = int_z_2d(Sxytmp, Oxytmp);
            
            GI = diag(Rs)+1j*w0*cof*Lint/dS(ik)*1.01;
            Yl = inv(GI);
            
            Zmag = inv(sum(sum(Yl)));
            
            Rmag2D(ik,ih) = real(Zmag);
            Lmag2D(ik,ih) = imag(Zmag)./w0;
                   
        else
            [I31, I21, I23, I22, I33, Lint] = int_z_m_2d(Sxytmp, Oxytmp);
            
            E = eye(Ns,Ns);
            km = mu0*mur/(mur-1);      % km=mu0*mur/(mur-1)/(mu0/2*pi)
            
            G22 =  cof*I22./dS(ik)-(E*km)./dS(ik);
            G33 =  cof*I33./dS(ik)-(E*km)./dS(ik);
            G21 = -cof*I21./dS(ik);
            G31 =  cof*I31./dS(ik);
            G23 =  cof*I23./dS(ik);
            % G32=G23;
            
            G11 =  diag(Rs)+1j*w0*cof*Lint./dS(ik);
            G12 =  1j*w0*cof*I21./dS(ik);
            G13 = -1j*w0*cof*I31./dS(ik);
            
%             Gtmp2 = G23/G22;
%             Gtmp3 = G23/G33;
%             GI = G11 + G12/(Gtmp3*G23-G22)*(G21-G31-Gtmp3*G31) ...
%                 + G13/(Gtmp2*G23-G33)*(G31-Gtmp2*G21);

            
            GI = G11 + ( G12.*(G33.*G21-G23.*G31) + G13.*(G22.*G31-G23.*G21) ) ...
            ./( G23.*G23-G33.*G22 );
    
            Yl = inv(GI);
            
            Zmag = inv(sum(sum(Yl)));
            
            Rmag2D(ik,ih) = real(Zmag);
            Lmag2D(ik,ih) = imag(Zmag)./w0;
            
        end
        
    end
    
    
end


