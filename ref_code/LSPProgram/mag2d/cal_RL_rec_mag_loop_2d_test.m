
% uniform meshing cannot work, thus, this version is still under test

function [Rmag2D, Lmag2D]=cal_RL_rec_mag_loop_2d_test(W, T, Rdc, mur, frq)

% mu0/(2*pi) = 2e-7  this coefficient is cancelled out
mu0=4e-7*pi;
cof = 2e-7;

dd = 2; % distance between source and return conductor

Nf = length(frq);
Nc = length(W);

Rmag2D = zeros(Nc,Nf);
Lmag2D = zeros(Nc,Nf);

pt2D = zeros(1,2);
for ik = 1:Nc

    if mur(ik)==1

        for ih = 1:Nf
            w0 = 2*pi*frq(ih);
            
            [Xs1, Ys1, ws1, hs1, dS1, Rs1, Ns] = mesh2d_box_mag(pt2D, W(ik), T(ik), ...
                Rdc(ik), mur(ik), frq(ih));
            
            dS = zeros(Ns*2,Ns*2);
            for ix = 1:Ns
                %dS(ix+1:Ns*2,ix) = hstmp(ix+1:Ns*2)*ws1(ix);
                dS(ix+1:Ns*2,ix) = dS1(ix);
            end
            
            dS(Ns+1:Ns*2,Ns+1:Ns*2) = dS(1:Ns,1:Ns);
            dS = (dS+dS') + diag([dS1;dS1]);
            
            Sxy1 = zeros(Ns,4);
            
            Sxy1(:,1) = Xs1-ws1/2;
            Sxy1(:,3) = Xs1+ws1/2;
            Sxy1(:,2) = Ys1-hs1/2;
            Sxy1(:,4) = Ys1+hs1/2;
            
            Oxy1 = [Xs1,    Ys1];
            Oxy2 = [Xs1+dd, Ys1];
            
            T11 = int_z_2d(Sxy1, [Oxy1; Oxy2]);
            I11 = [ T11, [T11(Ns+(1:Ns),:)'; T11(1:Ns,:)] ];

            sel_mtx = tril(ones(Ns*2,Ns*2));
            I11 = I11.*sel_mtx;
            I11 = I11+I11'-diag(diag(I11));
            
            GI = diag([Rs1;Rs1])+1j*w0*cof*I11./dS;

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
        
        km = mu0*mur(ik)/(mur(ik)-1);      % km=mu0*mur/(mur-1)/(mu0/2*pi)
        
        for ih = 1:Nf
            
            w0 = 2*pi*frq(ih);
            
            [Xs1, Ys1, ws1, hs1, dS1, Rs1, Ns] = mesh2d_box_mag(pt2D, W(ik), T(ik), ...
                Rdc(ik), mur(ik), frq(ih));

            dS = zeros(Ns*2,Ns*2);
            for ix = 1:Ns
                dS(1:Ns*2,ix) = [dS1;dS1];
                dS(1:Ns*2,ix+Ns) = dS(1:Ns*2,ix);
            end

            Sxy1 = zeros(Ns,4);
            
            Sxy1(:,1) = Xs1-ws1/2;
            Sxy1(:,3) = Xs1+ws1/2;
            Sxy1(:,2) = Ys1-hs1/2;
            Sxy1(:,4) = Ys1+hs1/2;
            
            Sxy2 = Sxy1;
            Sxy2(:,1) =Sxy2(:,1)+dd;
            Sxy2(:,3) =Sxy2(:,3)+dd;
            
            Oxy1 = [Xs1,    Ys1];
            Oxy2 = [Xs1+dd, Ys1];
            
            [I31, I21, I23, I22, I33, I11] = int_z_m_2d([Sxy1;Sxy2], [Oxy1; Oxy2]);
            
            sel_mtx = triu(ones(Ns*2,Ns*2));
            dS = dS.*sel_mtx;
            dS = dS+dS'-diag(diag(dS));
            I31 = I31.*sel_mtx;
            I31 = I31+I31'-diag(diag(I31));
            I21 = I21.*sel_mtx;
            I21 = I21+I21'-diag(diag(I21));
            I23 = I23.*sel_mtx;
            I23 = I23+I23'-diag(diag(I23));
            I22 = I22.*sel_mtx;
            I22 = I22+I22'-diag(diag(I22));
            I33 = I33.*sel_mtx;
            I33 = I33+I33'-diag(diag(I33));
            I11 = I11.*sel_mtx;
            I11 = I11+I11'-diag(diag(I11));
            
            
            G22 =  cof*I22./dS - km./dS;
            G33 =  cof*I33./dS - km./dS;
            G21 = -cof*I21./dS;
            G31 =  cof*I31./dS;
            G23 =  cof*I23./dS;
            % G32=G23;
            
            G11 =  diag([Rs1;Rs1]) + 1j*w0*cof*I11./dS;
            G12 =  1j*w0*cof*I21./dS;
            G13 = -1j*w0*cof*I31./dS;

            Gtmp2 = G23/G22;
            Gtmp3 = G23/G33;
            GI = G11 + G12/(Gtmp3*G23-G22)*(G21-G31-Gtmp3*G31) ...
                + G13/(Gtmp2*G23-G33)*(G31-Gtmp2*G21);
            %             Zmtx = [GI    1j*w0*cof_mur*Mint'/dS(ik); ...
            %                     1j*w0*cof_mur*Mint/dS(ik)    GI ];
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
    end
    
end


end


