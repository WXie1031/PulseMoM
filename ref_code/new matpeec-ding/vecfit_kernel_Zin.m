function [R0,L0, Rn,Ln, Nfit, Zfit] = vecfit_kernel_Zin(Zin, f0, Nfit, vf_mod)
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

L0 = 0;
Nc = size(Zin,1);

if length(f0) < 2
    disp('Parameters for Vector Fitting MUST be Multi-Freqency');
    
    R0 = real(Zin(:,:,5));
    
    if Nc == 1
        Ln = zeros(1,Nfit);
        Rn = zeros(1,Nfit);
    elseif Nc>1
        Ln = zeros(Nc,Nc,Nfit);
        Rn = zeros(Nc,Nc,Nfit);
    end
    return;
end

Nfit_input = Nfit;
vf_v_flag = 1;

tic

if nargin == 4
    VFopts.asymp = vf_mod;
else
    %VFopts.asymp = 1;  % D=0, E=0(L)
    VFopts.asymp = 2;  % D~=0, E=0(L)
end


VFopts.plot = 0;
VFopts.screen = 0;

VFopts.Niter1 = 12;
VFopts.Niter2 = 6;
%opts.asymp = 1;
%opts.poletype = 'logcmplx';
%opts.Niter1 = 8;
poles = []; %[] initial poles are automatically generated as defined by opts.startpoleflag

RFopts.Niter_in = 5;
RFopts.outputlevel = 0;
RFopts.remove_HFpoles = 0;

s = 1j*2*pi*f0;
R0 = 0;

while R0 <= 0
    
    VFopts.N = Nfit;
    
    SER = VFdriver(Zin,s,poles,VFopts);
    
    % SER :The perturbed model, on pole residue form and on state space form.
    % Yfit : (n,n,Ns)3D matrix holding the Y-samples (or S-samples) of the
    %         perturbed model (at freq. s)
    % SER  Structure holding the rational model, as produced by VFdriveror RPdriver
    
    [SER, Zfit] = RPdriver(SER,s,RFopts);
    
    R0 = SER.D;
    if R0 < 0
        Nfit = Nfit+1;
    end
    
    if Nfit>10 && Nfit_input>3
        Nfit = 3;
        Nfit_input = 3;
    elseif Nfit>10
        disp('Vector fitting is failed.');
        vf_v_flag = 0;
        break;
    end
end

VFopts.N = Nfit;

if vf_v_flag==1
    
    R0 = SER.D;
    
    if Nc == 1
        Ln = zeros(1,VFopts.N);
        Rn = zeros(1,VFopts.N);
        for ik = 1:VFopts.N
            Rn(ik) = SER.R(:,:,ik)/SER.poles(ik);
            Ln(ik) = -1/SER.poles(ik)*Rn(ik);
        end
    elseif Nc >1
        Ln = zeros(Nc,Nc,VFopts.N);
        Rn = zeros(Nc,Nc,VFopts.N);
        for ik = 1:VFopts.N
            Rn(:,:,ik) = SER.R(:,:,ik)/SER.poles(ik);
            Ln(:,:,ik) = -1/SER.poles(ik)*Rn(:,:,ik);
        end
    end
else
    R0 = real(Zin(:,:,1));
    
    if Nc == 1
        Ln = zeros(1,VFopts.N);
        Rn = zeros(1,VFopts.N);
    elseif Nc>1
        Ln = zeros(Nc,Nc,VFopts.N);
        Rn = zeros(Nc,Nc,VFopts.N);
    end
end


end


