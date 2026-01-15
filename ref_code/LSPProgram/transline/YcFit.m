%
% Function to fit the characteristic admittance
%
% Input
%
%    Yc    ==> Characteristic admittance
%    f     ==> Vector of frequencies
%    Ns    ==> Number of samples
%    Ncon  ==> Number of conductors
%
% Outputs
%
%    YcPoles     ==> Poles to fit the characteristic admittance
%    YcResidues  ==> Residues of the characteristic admittance fit
%    YcConstant  ==> Constant terms of the characteristic admittance fit
%    YcProportional ==> Proportional term of the characteristic admittance  fit
%
% Call the function
%
%    [YcPoles,YcResidues,YcConstant,YcProportional]=YcFit(Yc,f,Ns,Ncon)
%
%
function [YcPoles,YcResidues,YcConstant,YcProportional]=YcFit(Yc,f,Ns,Ncon)

% Trace of the characteristic admittance
for k = 1:Ns
    Ytrace(k,1) = trace(Yc(:,:,k));
end

Npol = 10;                            % Number of poles
[Ps] = InitialPoles(f,Npol);   % Set the initial poles
s    = j*2*pi*f.';                      % Vector of the variable "s"
Ka   = 2;                                 % 1.-Strictly proper,  2.-Proper,  3.-Improper
for khg=1:20
    [YcPoles]=Poles(Ytrace.',s,Ps,Ns,Ka);  % Fit the trace of Yc, the subroutine calculates only the poles
    Ps=YcPoles;
end

%  Routine to compute the residues and the constant term of Yc by using
% the poles of the trace of Yc
for k = 1:Ncon
    for l = 1:Ncon
        Hs(:,1) = Yc(k,l,:);                                        % k-l term of the characteristic admittance for the frequency range
        [C,D,E]=Residue(Hs.',s,YcPoles,Ns,Ka);    % Function to calculate the k-l term residues and constant
        YcResidues(k,l,:) = C;                                 % k-l term residues
        YcConstant(k,l)   = D;                                % k-l constant term
        YcProportional(k,l) = E;                               % k-l proportional term
    end
end



% Grafics of the aproximated functions
% s    = j*2*pi*f.';
% Npol = length(YcPoles);
% for k=1:Ncon
%     for l=1:Ncon
%         YcU(1:Ns) = Yc(k,l,1:Ns);
%         Fp=0;
%         for kc = 1:Npol
%             Fp = Fp + (YcResidues(k,l,kc)./(s-YcPoles(kc))).';
%         end
%         Fp = Fp + YcConstant(k,l);
% %         figure(1),plot(f,real(YcU),f,real(Fp),'m')
% %         figure(2),plot(f,imag(YcU),f,imag(Fp),'m')
%         figure(3),plot(f,abs(YcU),f,abs(Fp),'m')
%         pause
%     end
% end


