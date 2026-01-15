%
% Function to fit the Idempotent metrices of Y*Z
%
% Input
%
%    Hm     ==> Modal Hm
%    Vm     ==> Modal velocities
%    ZL     ==> Total impedance
%    YL     ==> Total admittance
%    f      ==> Vector of frequencies
%    Ns     ==> Number of samples
%    lenght ==> Line lenght
%    Ncon   ==> Number of conductors
%
% Outputs
%
%    HkPoles     ==> Poles to fit the Idempotent matrices of Y*Z
%    HkResidues  ==> Residues of the Idempotent matrices of Y*Z
%    HkConstant  ==> Constant of the Idempotent matrices of Y*Z
%    HkProportional ==> Proportional of the Idempotent matrices of Y*Z
%    md           ==> Mode delay
%
% Call the function
%
%    [HkPoles,HkResidues,HkConstant,HkProportional,md]=HkFit(Hm,Vm,ZL,YL,f,Ns,lenght,Ncon)
%
%
function [HkPoles,HkResidues,HkConstant,HkProportional,md]=HkFit(Hm,Vm,ZL,YL,f,Ns,lenght,Ncon);

% Minimum phase delay by each mode
mda = ModeDelay(Hm.',f,lenght,Vm.',Ns,Ncon);
md = mda.';

% Routine to compute the Idempotents for the full frequency
for k=1:Ns
   % Function to calculate the Idempotents of Y*Z for one frequency
   [Hk] = HmIdem(ZL(:,:,k),YL(:,:,k),lenght,f(k),md,Hm(k,:));
   HkIdem(:,:,k,:) = Hk; % Idempotents for the fist frequency
end

Ng = 3;

for m = 1:Ng
    for k=1:Ns
        TraceHk(k,m) = trace(HkIdem(:,:,k,m));   % First Idempotent for k-frequency
    end
end

s     = j*2*pi*f.';                          % Vector of the variable "s"
Ka    = 1;                                      % 1.-Strictly proper,  2.-Proper,  3.-Improper
Npol = 10;                                   % Number of poles

for m = 1:Ng
    Hk = TraceHk(:,m).';
    [Ps] = InitialPoles(f,Npol);          % Set the initial poles
    for khg=1:10
           [HkPol]=Poles(Hk,s,Ps,Ns,Ka); % Fit the trace of Hk1, means, the trace of the first Idempotent
           Ps=HkPol;
    end
    HkPoles(:,m)=Ps;
end

%  Routine to compute the residues and the constant term of Idempotent matrices of
% Hm by using the poles of the trace of each one
for m = 1:Ng
    for k = 1:Ncon
        for l = 1:Ncon
            Hs(:,1) = HkIdem(k,l,:,m);                % k-l term of the first Idempotent for the frequency range
            [C,D,E]=Residue(Hs.',s,HkPoles(:,m),Ns,Ka);   % Function to calculate the k-l term residues and constant
            HkResidues(k,l,:,m)   = C;                  % k-l-m term residues
            HkConstant(k,l,m)   = D;                    % k-l-m constant term
            HkProportional(k,l,m) = E;              % k-l-m proportional term
        end
    end
end



% s    = j*2*pi*f.';
% Npol = length(HkPoles);
% for m=1:3
%     for k=1:Ncon
%         for l=1:Ncon
%              HmD(1:Ns) = HkIdem(k,l,m,1:Ns);
%              HmU = (HmD.').*exp(s.*md(m));
%              Fm=0;
%              for kc = 1:Npol
%                 Fm = Fm + (HkResidues(k,l,m,kc)./(s-HkPoles(m,kc))).';
%              end
%             %figure(1),semilogx(f,abs(HmU),f,abs(Fm),'m')
%             %figure(2),plot(f,imag(HmU),f,imag(Fm),'m')
%             figure(3),semilogx(f,abs(HmU),'b',f,abs(Fm),'m')
%             pause
%         end
%     end
% end


