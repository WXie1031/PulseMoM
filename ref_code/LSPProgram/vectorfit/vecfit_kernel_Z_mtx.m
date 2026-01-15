function [R03,L03, Rn3,Ln3, Zfit3] = vecfit_kernel_Z_mtx(Zi3, f0, Nfit, vf_mod)
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

Nc = size(Zi3,1);

if length(f0) < 2
    disp('Parameters for Vector Fitting MUST be Multi-Freqency');
    
    R03 = real(Zi3(:,:,5));
    L03 = imag(Zi3(:,:,5))/(2*pi*f0(5));
    if Nc == 1
        Ln3 = zeros(1,Nfit);
        Rn3 = zeros(1,Nfit);
    elseif Nc>1
        Ln3 = zeros(Nc,Nc,Nfit);
        Rn3 = zeros(Nc,Nc,Nfit);
    end
    return;
end


tic

if nargin == 4
    VFopts.asymp = vf_mod;
else
    VFopts.asymp = 3;
end

VFopts.N = Nfit;

VFopts.plot = 1;
VFopts.screen = 0;

% opts.plot.s_pass
%opts.asymp = 3;
VFopts.poletype = 'linlogcmplx';
VFopts.poletype = 'lincmplx';
VFopts.Niter1 = 12;
VFopts.Niter2 = 6;
VFopts.weightparam = 4;
VFopts.passive_DE = 1;
%VFopts.compx_ss = 0;
poles = []; %[] initial poles are automatically generated as defined by opts.startpoleflag

RFopts.Niter_in = 5;
RFopts.plot = VFopts.plot;
RFopts.outputlevel = 0;
RFopts.remove_HFpoles = 0;
% RFopts.weightparam = VFopts.weightparam;

s = 1j*2*pi*f0;

SER = VFdriver(Zi3,s,poles,VFopts);
[SER, Zfit3] = RPdriver(SER,s,RFopts);
vf_v_flag = 1;


if vf_v_flag==1
    R03 = SER.D;
    L03 = SER.E;
    
    if Nc == 1
        Ln3 = zeros(1,VFopts.N);
        Rn3 = zeros(1,VFopts.N);
        for ik = 1:VFopts.N
            Rn3(ik) = SER.R(:,:,ik)/SER.poles(ik);
            Ln3(ik) = -1/SER.poles(ik)*Rn3(ik);
        end
        R03 = R03-sum(Rn3);
        
    elseif Nc >1
        Ln3 = zeros(Nc,Nc,VFopts.N);
        Rn3 = zeros(Nc,Nc,VFopts.N);
        for ik = 1:VFopts.N
            Rn3(:,:,ik) = SER.R(:,:,ik)/SER.poles(ik);
            Ln3(:,:,ik) = -1/SER.poles(ik)*Rn3(:,:,ik);
        end
        
        R03 = R03-sum(Rn3,3);
    end
else
    R03 = real(Zi3(:,:,5));
    L03 = imag(Zi3(:,:,5))/(2*pi*f0(5));
    if Nc == 1
        Ln3 = zeros(1,VFopts.N);
        Rn3 = zeros(1,VFopts.N);
    elseif Nc>1
        Ln3 = zeros(Nc,Nc,VFopts.N);
        Rn3 = zeros(Nc,Nc,VFopts.N);
    end
end



end


