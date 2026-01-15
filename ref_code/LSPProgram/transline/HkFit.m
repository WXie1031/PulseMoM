function [HkPoles,HkResidues,HkConstant, HkProportional,md] = ...
    HkFit(Hm,Vm,ZL,YL,f,Ns,lenght,Ncon)

% Minimum phase of each mode
md = ModeDelay(Hm.',f,lenght,Vm.',Ns,Ncon);

% Computing Idempotents
for k=1:Ns
    % Function to calculate Idempotents of Y*Z
    [Hk] = HmIdem(ZL(:,:,k),YL(:,:,k),lenght,f(k), md,Hm(k,:));
    HkIdem(:,:,:,k) = Hk; % Idempotents
end
for m = 1:3
    for k=1:Ns
        TraceHk(m,k) = trace(HkIdem(:,:,m,k));
    end
end

s = 1i*2*pi*f.'; % Vector of the variable "s"
Ka =1;%1.-Strictly proper, 2.-Proper, 3.-Improper
Npol = 5; % Number of poles
[Ps] = InitialPoles(f,Npol); % Set the initial poles
for m = 1:3
    Hk = TraceHk(m,:);
    for khg=1:10
        [HkPol]=Poles(Hk,s,Ps,Ns,Ka);
        Ps=HkPol;
    end
    HkPoles(m,:)=Ps;
end

% Residues for Idempotent matrices of
% Hm from the poles of each trace.
for m = 1:3
    for k = 1:Ncon
        for l = 1:Ncon
            Hs(:,1) = HkIdem(k,l,m,:); % k-l term
            [C,D,E]=Residue(Hs.',s,HkPoles(m,:),Ns,Ka);
            HkResidues(k,l,m,:) = C; % k-l-m term
            HkConstant(k,l,m) = D; % k-l-m constant
            HkProportional(k,l,m) = E; % k-l-m prop
        end
    end
end


end

