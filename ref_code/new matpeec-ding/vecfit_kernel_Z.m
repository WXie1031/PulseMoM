function [R0,L0, Rn,Ln, Nfit, Zfit] = vecfit_kernel_Z(Zi, f0, Nfit, L0min, vf_mod)
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

Nc = size(Zi,1);
Nf = length(f0);
if Nf < 2
    disp('Parameters for Vector Fitting MUST be Multi-Freqency');
    
    R0 = real(Zi(:,:,5));
    L0 = imag(Zi(:,:,5))/(2*pi*f0(5));
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

if nargin == 5
    VFopts.asymp = vf_mod;
else
    VFopts.asymp = 3;
end

% VFopts.asymp = 2;

VFopts.plot = 0;
VFopts.screen = 0;

%opts.asymp = 3;
VFopts.poletype = 'lincmplx';
VFopts.Niter1 = 12;
VFopts.Niter2 = 6;
%VFopts.weightparam = 1;
%VFopts.compx_ss = 0;
poles = []; %[] initial poles are automatically generated as defined by opts.startpoleflag

RFopts.Niter_in = 5;
RFopts.outputlevel = 0;
RFopts.remove_HFpoles = 0;

L0 = 0;
s = 1j*2*pi*f0;
SER = [];
Zfit = zeros(Nc,Nc,Nf);
[~,id_f] = min(abs(f0-100));
err_fit = 1;
err_bdy = 0.4;
while  L0<0.99*L0min || err_fit>err_bdy 
    
    VFopts.N = Nfit;
    
    SER = VFdriver(Zi,s,poles,VFopts);
    
    % SER :The perturbed model, on pole residue form and on state space form.
    % Yfit : (n,n,Ns)3D matrix holding the Y-samples (or S-samples) of the
    %         perturbed model (at freq. s)
    % SER  Structure holding the rational model, as produced by VFdriveror RPdriver
    
    [SER, Zfit] = RPdriver(SER,s,RFopts);
    L0 = SER.E;
    
    err_fit = max(abs(Zfit(:,:,id_f)-Zi(:,:,id_f))./abs(Zi(:,:,id_f)));
    
    %if SER.E<=1e-10
    if SER.E<=0.99*L0min || err_fit>err_bdy
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

if L0min==0
    SER = VFdriver(Zi,s,poles,VFopts);
    [SER, Zfit] = RPdriver(SER,s,RFopts);
    vf_v_flag = 1;
end


if vf_v_flag==1 && ~isempty(SER)
    R0 = SER.D;
    L0 = SER.E;

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
    R0 = real(Zi(:,:,5));
    L0 = imag(Zi(:,:,5))/(2*pi*f0(5));
    if Nc == 1
        Ln = zeros(1,VFopts.N);
        Rn = zeros(1,VFopts.N);
    elseif Nc>1
        Ln = zeros(Nc,Nc,VFopts.N);
        Rn = zeros(Nc,Nc,VFopts.N);
    end
end


end


