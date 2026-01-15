
% Yi =
ex1_Y

Yi = bigY;

Nfit = 8;


Nc = size(Yi,1);
Nf = length(s);

VFopts.plot = 1;
VFopts.screen = 1;

VFopts.N = Nfit;
%opts.asymp = 3;
VFopts.poletype = 'linlogcmplx';
% VFopts.Niter1 = 12;
% VFopts.Niter2 = 6;
%VFopts.weightparam = 1;
%VFopts.compx_ss = 0;
poles = []; %[] initial poles are automatically generated as defined by opts.startpoleflag

% RFopts.Niter_in = 5;
% RFopts.outputlevel = 0;
% RFopts.remove_HFpoles = 0;


% SER :The perturbed model, on pole residue form and on state space form.
% Yfit : (n,n,Ns)3D matrix holding the Y-samples (or S-samples) of the
%         perturbed model (at freq. s)
% SER  Structure holding the rational model, as produced by VFdriveror RPdriver
SER = VFdriver(Yi,s,poles,VFopts);
[SER, Zfit] = RPdriver(SER,s);

d0 = SER.D;
e0 = SER.E;

rn = SER.R;
pn = SER.poles;



%% 1. use r<0 & r>0 combined expression
Rn = zeros(Nc,Nc,Nfit);
Xn = zeros(Nc,Nc,Nfit);

flag_cplx = 0;
for ik = 1:Nc
    for ig = ik:Nc
        
        for ih = 1:Nfit
            
            if flag_cplx == 0
                if isreal(rn(ik,ig,ih))
                    if rn(ik,ig,ih)>0
                        Rn(ik,ig,ih) = - pn(ih)/rn(ik,ig,ih);
                        Xn(ik,ig,ih) = 1/rn(ik,ig,ih);
                    else
                        Rn(ik,ig,ih) = -pn(ih)/rn(ik,ig,ih);
                        Xn(ik,ig,ih) = 1/(pn(ih)*Rn(ik,ig,ih));
                    end
                    
                else
                    r1 = rn(ik,ig,ih);
                    r2 = rn(ik,ig,ih+1);
                    p1 = pn(ih);
                    p2 = pn(ih+1);
                    
                    if real(r1)>0
                        L = 1/(r1+r2);  % L
                        R1 = -(r1*p1+r2*p2)*L^2 ;  % R1
                        R2 = -1/(r1/p1+r2/p2) - R1 ; %R2
                        C = -1/( (p1-p2)^2*L^3*r1*r2 );   % C
                    else
                        C = -( r1/p1^2 + r2/p2^2 );    % L
                        R1 = (r1/p1^3+r2/r2^3)/C^2;
                        L = -r1*r2/((p1*p2)^2)*(1/p1-1/p2)^2/(C^3);
                        R2 = -(1/p1-1/p2)^2/((p1/r1+p2/r2)*C^2);
                    end
                    
                    Rn(ik,ig,ih) = R1;
                    Rn(ik,ig,ih+1) = R2;
                    Xn(ik,ig,ih) = L;
                    Xn(ik,ig,ih+1) = C;
                    
                    flag_cplx = 1;
                end
            else
                flag_cplx = 0;
            end
            
        end
    end
end
 Rn_real = real(Rn);
 Rn_imag = imag(Rn);


%% 2. use r<0 expression
Rm = zeros(Nc,Nc,Nfit);
Xm = zeros(Nc,Nc,Nfit);

for ik = 1:Nc
    for ig = ik:Nc
        for ih = 1:Nfit
            if flag_cplx == 0
                if isreal(rn(ik,ig,ih))
                    Rm(ik,ig,ih) = - pn(ih)/rn(ik,ig,ih);
                    Xm(ik,ig,ih) = 1/rn(ik,ig,ih);
                else
                    r1 = rn(ik,ig,ih);
                    r2 = rn(ik,ig,ih+1);
                    p1 = pn(ih);
                    p2 = pn(ih+1);
                    
                    L = 1/(r1+r2);  % L
                    R1 = -(r1*p1+r2*p2)*L^2 ;  % R1
                    R2 = -1/(r1/p1+r2/p2) - R1 ; %R2
                    C = -1/( (p1-p2)^2*L^3*r1*r2 );   % C
                    
                    Rm(ik,ig,ih) = R1;
                    Rm(ik,ig,ih+1) = R2;
                    Xm(ik,ig,ih) = L;
                    Xm(ik,ig,ih+1) = C;
                    
                    flag_cplx = 1;
                end
            else
                flag_cplx = 0;
            end
        end
    end
end




%% 3. use r>0 expression

Rx = zeros(Nc,Nc,Nfit);
Xx = zeros(Nc,Nc,Nfit);

for ik = 1:Nc
    for ig = ik:Nc
        for ih = 1:Nfit
            if flag_cplx == 0
                if isreal(rn(ik,ig,ih))
                    Rx(ik,ig,ih) = -pn(ih)/rn(ik,ig,ih);
                    Xx(ik,ig,ih) = 1/(pn(ih)*Rn(ik,ig,ih));
                else
                    r1 = rn(ik,ig,ih);
                    r2 = rn(ik,ig,ih+1);
                    p1 = pn(ih);
                    p2 = pn(ih+1);
                    
                    C = -( r1/p1^2 + r2/p2^2 );    % L
                    R1 = (r1/p1^3+r2/r2^3)/C^2;
                    L = -r1*r2/((p1*p2)^2)*(1/p1-1/p2)^2/(C^3);
                    R2 = -(1/p1-1/p2)^2/((p1/r1+p2/r2)*C^2);
                    
                    Rx(ik,ig,ih) = R1;
                    Rx(ik,ig,ih+1) = R2;
                    Xx(ik,ig,ih) = L;
                    Xx(ik,ig,ih+1) = C;
                    
                    flag_cplx = 1;
                end
            else
                flag_cplx = 0;
            end
        end
    end
end











