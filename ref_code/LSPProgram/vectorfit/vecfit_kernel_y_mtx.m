function [yd3,ye3, yrn3,ypn3, Yfit3] = vecfit_kernel_y_mtx(Yi3, f0, Nfit, vf_mod)
%% Using Vector Fitting Toolkit
% H : (n,n,Ns) 3D matrix holding the H-samples (Ycab)
% s : (1,Ns) vector holding the frequency samples, s= jw? [rad/sec].
% poles: (1,N)  vector holding the initial poles (manual specification of
%         initial poles). Use optsfor automated specification.
% SERis a structure with the model on pole-residue form. For a model
%      with nports and N residue matrices, the dimensions are
% SER.poles: (1,N)
% SER.R: (n,n,N)  (residue matrices)
% SER.D: (n,n)
% SER.E: (n,n)

Nc = size(Yi3,1);

if length(f0) < 2
    disp('Parameters for Vector Fitting MUST be Multi-Freqency');
    
    yd3 = real(Yi3(:,:,5));
    ye3 = imag(Yi3(:,:,5))/(2*pi*f0(5));
    if Nc == 1
        ypn3 = zeros(1,Nfit);
        yrn3 = zeros(1,Nfit);
    elseif Nc>1
        ypn3 = zeros(Nc,Nc,Nfit);
        yrn3 = zeros(Nc,Nc,Nfit);
    end
    return;
end


tic

if nargin == 4
    VFopts.asymp = vf_mod;
else
    VFopts.asymp = 2;
end
VFopts.asymp = 3;

s = 1j*2*pi*f0;

VFopts.plot = 1;
VFopts.screen = 0;

VFopts.N = Nfit;
VFopts.screen = 0;

%opts.asymp = 3;
VFopts.poletype = 'logcmplx';
VFopts.Niter1 = 12;
VFopts.Niter2 = 10;
VFopts.weightparam = 2;
% =1 --> Same weight for all elements: weight(i,j,k)=1
% =2 --> weight(i,j,k)=1./abs(H(i,j,k)) inverse weighting
% =3 --> weight(i,j,k)=1./sqrt(abs(H(i,j,k)))
% =4 --> weight(k)=1/norm(H(:,:,k)) inverse weighting
% =5 --> weight(k)=1/sqrt(norm(H(:,:,k)))

%VFopts.asymp=2;      %Fitting includes D   
%VFopts.passive_DE = 1;
%VFopts.compx_ss = 0;
poles = [];
%[] initial poles are automatically generated as defined by opts.startpoleflag



% SER :The perturbed model, on pole residue form and on state space form.
% Yfit : (n,n,Ns)3D matrix holding the Y-samples (or S-samples) of the
%         perturbed model (at freq. s)
RFopts.Niter_in = 5;
RFopts.outputlevel = 0;
% RFopts.remove_HFpoles = 1;
% RFopts.weightparam = VFopts.weightparam;

SER = VFdriver(Yi3,s,poles,VFopts);
[SER, Yfit3] = RPdriver(SER,s,RFopts);

yd3 = SER.D;
ye3 = SER.E;

if Nc == 1
    yrn3 = zeros(1,VFopts.N);
    ypn3 = zeros(1,VFopts.N);
    for ik = 1:VFopts.N
        yrn3(ik) = SER.R(:,:,ik);
        ypn3(ik) = SER.poles(ik);
        
        if real(yrn3(ik))<0
            yd3 = yd3-yrn3(ik)/ypn3(ik);
        end
    end
    
elseif Nc >1
    yrn3 = zeros(Nc,Nc,VFopts.N);
    ypn3 = zeros(Nc,Nc,VFopts.N);
    
    
    for ik = 1:VFopts.N
        yrn3(:,:,ik) = SER.R(:,:,ik);
        ypn3(:,:,ik) = SER.poles(ik);
        
%         rtmp = rfit3(:,:,ik);
%         ptmp = pfit3(:,:,ik);
%         ind = real(rtmp)>0;
%         
%         dfit3(ind) = dfit3(ind)+rtmp(ind)./ptmp(ind);
    end
end


end


