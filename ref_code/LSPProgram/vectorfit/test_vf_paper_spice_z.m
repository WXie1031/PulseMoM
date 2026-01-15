
% Zi = 


Nfit = 15;


Nc = size(Zi,1);
Nf = length(f0);


VFopts.plot = 0;
VFopts.screen = 0;

VFopts.N = Nfit;
%opts.asymp = 3;
VFopts.poletype = 'linlogcmplx';
VFopts.Niter1 = 12;
VFopts.Niter2 = 6;
%VFopts.weightparam = 1;
%VFopts.compx_ss = 0;
poles = []; %[] initial poles are automatically generated as defined by opts.startpoleflag

RFopts.Niter_in = 5;
RFopts.outputlevel = 0;
RFopts.remove_HFpoles = 0;


% SER :The perturbed model, on pole residue form and on state space form.
% Yfit : (n,n,Ns)3D matrix holding the Y-samples (or S-samples) of the
%         perturbed model (at freq. s)
% SER  Structure holding the rational model, as produced by VFdriveror RPdriver
SER = VFdriver(Zi,s,poles,VFopts);
[SER, Zfit] = RPdriver(SER,s,RFopts);

d0 = SER.D;
e0 = SER.E;

rn = SER.R;
pn = SER.poles;

Rn = zeros(Nc,Nfit);
Xn = zeros(Nc,Nfit);
for ik = 1:Nfit
    if isreal(rn(ik))
        if rn(ik)>0
            Rn(ik) = - rn(ik)/pn(ik);
            Xn(ik) = 1/rn(ik);
        else
            Rn(ik) = - rn(ik)/pn(ik);
            Xn(ik) = - rn(ik)/(pn(ik).^2);
        end
    
    else
        r1 = rn(ik);
        r2 = rn(ik+1);
        p1 = pn(ik);
        p2 = pn(ik+1);
        
        if real(rn(ik))>0
            C = 1/(r1+r2);  % C
            R2 = -1/( (r1*p1+r2*p2)*C^2 );  % R2
            R1 = -1/( 1/(r1/p1+r2/p2) + 1/R2 ); %R1
            L = -R1/C/(r1*p2+r2*p1);   % L
        else
            L = -( r1/p1^2 + r2/p2^2 );    % L
            R2 = L^2/(r1/p1^3+r2/r2^3);
            R1 = 1/( 1/(r1/p1+r2*p2)-1/R2 );
            C = ( r1/(p1^2*p2)+r2/(p1*p2^2) );
        end
        
        Rn(ik) = R1;
        Rn(ik+1) = R2;
        Xn(ik) = L;
        Xn(ik+1) = C;
        
        ik = ik+1;
    end
    
end


















