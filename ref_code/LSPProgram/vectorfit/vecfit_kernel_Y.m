function [yd,ye,yrn,ypn, Nfit,Yfit] = vecfit_kernel_Y(Yi, frq, Nfit)
%% Using Vector Fitting Toolkit
% H : (n,n,Ns) 3D matrix holding the H-samples (Yi)
% s : (1,Ns) vector holding the frequency samples, s= jw? [rad/sec].
% poles: (1,N)  vector holding the initial poles (manual specification of
%         initial poles). Use optsfor automated specification.
% SERis a structure with the model on pole-residue form. For a model
%      with nports and N residue matrices, the dimensions are
% SER.poles: (1,N)
% SER.R: (n,n,N)  (residue matrices)
% SER.D: (n,n)
% SER.E: (n,n)

Nc = size(Yi,1);
Nf = length(frq);

if Nf < 2
    disp('Parameters for Vector Fitting MUST be Multi-Freqency');
    
    yd = real(Yi(:,:,5));
    ye = imag(Yi(:,:,5))/(2*pi*frq(5));
    if Nc == 1
        yrn = zeros(1,Nfit);
        ypn = zeros(1,Nfit);
    elseif Nc>1
        yrn = zeros(Nc,Nc,Nfit);
        ypn = zeros(Nc,Nc,Nfit);
    end
    return;
    
end

Nfit_input = Nfit;
vf_v_flag = 1;

tic

if nargin == 4
    VFopts.asymp = vf_mod;
else
    VFopts.asymp = 3;
end


VFopts.plot = 1;
VFopts.screen = 0;

% VFopts.asymp = 3;
VFopts.poletype = 'linlogcmplx';
VFopts.Niter1 = 12;
VFopts.Niter2 = 6;
poles = []; %[] initial poles are automatically generated as defined by opts.startpoleflag

RFopts.Niter_in = 5;
RFopts.outputlevel = 0;
% RFopts.remove_HFpoles = 0;


s = 1j*2*pi*frq;
SER = [];
% SER  Structure holding the rational model, as produced by VFdriveror RPdriver
% Yfit = zeros(Nc,Nc,Nf);
% [~,id_f] = min(abs(frq-1000));
% err_fit = 1;
% err_bdy = 0.4;
% 
% while  err_fit>err_bdy
%     
%     VFopts.N = Nfit;
%     
%     SER = VFdriver(Yi,s,poles,VFopts);
%     
%     % SER :The perturbed model, on pole residue form and on state space form.
%     % Yfit : (n,n,Ns)3D matrix holding the Y-samples (or S-samples) of the
%     %         perturbed model (at freq. s)
%     [SER, Yfit] = RPdriver(SER,s,RFopts);
%     
%     err_fit = max(abs(Yfit(:,:,id_f)-Yi(:,:,id_f))./abs(Yi(:,:,id_f)));
%     
%     if err_fit>err_bdy
%         Nfit = Nfit+1;
%     end
%     
%     if Nfit>10 && Nfit_input>3
%         Nfit = 3;
%         Nfit_input = 3;
%     elseif Nfit>10
%         disp('Vector fitting is failed.');
%         vf_v_flag = 0;
%         break;
%     end
% end


if vf_v_flag==1 && ~isempty(SER)
    
else
    Nfit = Nfit_input;
    VFopts.N = Nfit;
    SER = VFdriver(Yi,s,poles,VFopts);
    [SER, Yfit] = RPdriver(SER,s,RFopts);
end

yd = SER.D;
ye = SER.E;

if Nc == 1
    ypn = zeros(1,VFopts.N);
    yrn = zeros(1,VFopts.N);
    for ik = 1:VFopts.N
        yrn(ik) = SER.R(:,:,ik);
        ypn(ik) = SER.poles(ik);
        %             ypn(ik) = 1/SER.R(ik);
        %             yrn(ik) = -SER.poles(ik)/SER.R(ik);
    end
elseif Nc >1
    ypn = zeros(Nc,Nc,VFopts.N);
    yrn = zeros(Nc,Nc,VFopts.N);
    for ik = 1:VFopts.N
        yrn(:,:,ik) = SER.R(:,:,ik);
        ypn(:,:,ik) = SER.poles(ik);
        %             yrn(:,:,ik) = SER.R(:,:,ik)/SER.poles(ik);
        %             ypn(:,:,ik) = -1/SER.poles(ik)*yrn(:,:,ik);
    end
end


%     yd = real(Yi(:,:,5));
%     ye = imag(Yi(:,:,5))/(2*pi*frq(5));
%     if Nc == 1
%         ypn = zeros(1,VFopts.N);
%         yrn = zeros(1,VFopts.N);
%     elseif Nc>1
%         ypn = zeros(Nc,Nc,VFopts.N);
%         yrn = zeros(Nc,Nc,VFopts.N);
%     end
% end


end
