% Parameer for the simulation
Ns = menu('CHOOSE THE TIME STEP','64 micro-seconds','32 microseconds',...
    '16 microseconds','8 microseconds',...
    '4 microseconds','2 microseconds',...
    '1 microseconds','0.5 microseconds',...
    '0.3 microseconds','0.1 microseconds');

if Ns == 1
    Dt   = 64e-6;
elseif Ns == 2
    Dt   = 32e-6;
elseif Ns == 3
    Dt   = 16e-6;
elseif Ns == 4
    Dt   =  8e-6;
elseif Ns == 5
    Dt   = 4e-6;
elseif Ns == 6;
    Dt   = 2e-6;
elseif Ns == 7
    Dt   = 1e-6;
elseif Ns == 8
    Dt   = 5e-7;
elseif Ns == 9
    Dt   = 3e-7;
elseif Ns ==10
    Dt   = 1e-7;
end

Ts   = 0.016;                % Observation time
Nt   = fix(Ts/Dt);                       % Number of time steps
t1   = fix(md./Dt);                    % Time delay of H expresed in samples
t0   = fix(max(md)./Dt);           % Maximum time delay expressed in samples
t    = (0:Dt:(Nt+t1)*Dt-Dt);     % Vector of time
Ks = menu('CHOOSE THE TIME STEP','unit step','sinoidal');
if Ks == 1
    Isr  = ones(Ncon,Nt+t0);               % Direct current source
    %Isr(2:3,:) =  Isr(2:3,:)*0;             % 2nd and 3th source are zero
elseif Ks ==2
    Isr(1,:) = sin(337*t+2*pi/3);
    Isr(2,:) = sin(337*t);
    Isr(3,:) = sin(337*t-2*pi/3);
end
NpYc = length(YcPoles);        % Number of poles of Yc
NpH  = length(HkPoles);      % Number of poles of the first Idempotent matrix
Ng = 3;                                  %Numero de grupos

% Initialize the states for both nodes
ZA  = zeros(Ncon,NpYc);  % Keep the state variables
ZB  = zeros(Ncon,NpYc);  % Keep the state variables
YA  = zeros(Ncon,NpH,Ng);   % Keep the state variables
YB  = zeros(Ncon,NpH,Ng);   % Keep the state variables
IfarA = zeros(Ncon,t0+3);     % Current in the remote node
IfarB = zeros(Ncon,t0+3);     % Current in the remote node
VO  = zeros(Ncon,1);       % Voltage in node A
Vi  = zeros(Ncon,Nt+t0);       % Voltage in node A
VL  = zeros(Ncon,1);       % Voltage in node B
Vf  = zeros(Ncon,Nt+t0);       % Voltage in node B
IO  = zeros(Ncon,1);       % Current in node A
Ii  = zeros(Ncon,Nt+t0);       % Current in node A
IL  = zeros(Ncon,1);       % Current in node B
If  = zeros(Ncon,Nt+t0);       % Current in node B
Iri = zeros(Ncon,Nt+t0);       % Current in Y source
Irf = zeros(Ncon,Nt+t0);       % Current in Y charge

IfarAint = zeros(Ncon,Ng);     % Current in the remote node
IfarBint = zeros(Ncon,Ng);     % Current in the remote node

% Constants for the state ZA and ZB
Ai(:,1) = (1+(Dt/2)*YcPoles)./(1-(Dt/2)*YcPoles);
Au(:,1) = ((Dt/2)./(1-(Dt/2)*YcPoles));
Bi(:,1) = (Ai+1).*Au;
Gy = zeros(Ncon,Ncon);
for nm = 1:NpYc
    Di(:,:,nm)    = YcResidues(:,:,nm)*Bi(nm);
    Gy  = Gy + YcResidues(:,:,nm)*Au(nm);
end

% Constants for the states YA and YB
for k = 1:Ng
    K1(:,k) = (1+(Dt/2)*HkPoles(:,k))./(1-(Dt/2)*HkPoles(:,k));
    Ka(:,k) = (((Dt/2))./(1-(Dt/2)*HkPoles(:,k)));
    Ku(:,k) = (K1(:,k)+1).*Ka(:,k);
end

for k = 1:Ng
    for nm = 1:NpH
        K2(:,:,nm,k) =  HkResidues(:,:,nm,k).*Ka(nm,k);
        K3(:,:,nm,k) = HkResidues(:,:,nm,k).*Ku(nm,k);
    end
end

Gy  = Gy + YcConstant;                 % Admitance of the Ish
Yi  = diag(eye(3)*[1/600; 1/600; 1/600]);    % Admitance of the source, connected in node A
Gys = inv(Gy + Yi);                         % Impedance to calculate VO
Yr  =diag(eye(3)*[1/1e6; 1/1e6; 1/1e6]);     % Admittance of the charge, connected in node B
Gyr = inv(Gy + Yr);                         % Impedance to calculate VL

% % Contants terms to perform the interpolation
tm =md - t1*Dt;           % Time to perform the interpolation
% % Linear interpolation constants
c1 = tm/Dt;
c2 = 1-c1;
c3 = ones(Ng,1);

h1 = t1+1;    % Poiter for the intrpolation and the buffer
h2 = t1+2;    % Poiter for the intrpolation and the buffer
h3 = t1+3;    % Poiter for the intrpolation and the buffer

tic

for k = t0+2:Nt+t0-3
    
    IfarA(:,1) = IL + Gy*VL + sum(ZB(:,:),2);
    IfarB(:,1) = IO + Gy*VO + sum(ZA(:,:),2);
    
    % Con interpolación lineal
    for m = 1:Ng
        IfarAint(:,m) = c2(m)*IfarA(:,t1(m)) + c3(m)*IfarA(:,h1(m)) + c1(m)*IfarA(:,h2(m));
        IfarBint(:,m) = c2(m)*IfarB(:,t1(m)) + c3(m)*IfarB(:,h1(m)) + c1(m)*IfarB(:,h2(m));
    end
    
    IfarA(:,2:h3) = IfarA(:,1:h2);
    IfarB(:,2:h3) = IfarB(:,1:h2);
    
    for m = 1:NpYc
        ZA(:,m) = Ai(m)*ZA(:,m) + Di(:,:,m)*VO;
        ZB(:,m) = Ai(m)*ZB(:,m) + Di(:,:,m)*VL;
    end
    
    for l = 1:Ng
        for m = 1:NpH
            YA(:,m,l) = K1(m,l)*YA(:,m,l) + K2(:,:,m,l)*IfarAint(:,l);
            YB(:,m,l) = K1(m,l)*YB(:,m,l) + K2(:,:,m,l)*IfarBint(:,l);
        end
    end
    
    HistO = - sum(ZA(:,:),2) + sum(sum(YA(:,:,:),3),2);
    HistL  = - sum(ZB(:,:),2) + sum(sum(YB(:,:,:),3),2);
    
    VO = Gys*(Isr(:,k)+HistO);
    VL = Gyr*HistL;
    
    IO = Gy*VO - HistO;
    IL = Gy*VL - HistL;
    
    Vi(:,k) = VO;
    Vf(:,k) = VL;
    Ii(:,k) = IO;
    If(:,k) = IL;
    
end

toc

Iri = Yi*Vi;
Irf = Yr*Vf;

vt = (0:Dt:length(Vi(1,:))*Dt-(t0+4)*Dt)';
N = length(vt);
a1 = t1+1;
a2 = Nt+t1-3;


figure(1),plot(vt,Vi(:,a1:a2),':',vt,Vf(:,a1:a2))
ylabel('Amplitude in volts')
xlabel('Time in seconds')
legend('Sending end phase A','Sending end phase B','Sending end phase C',...
    'Receiving end phase A','Receiving end phase B','Receiving end phase C')
break


if Ns == 1
    figure(1),plot(vt,Vi(a1:a2),':k',vt,Vf(a1:a2),':k')
elseif Ns == 2
    figure(1),plot(vt(1:2:N),Vi(a1:2:a2),':m',vt(1:2:N),Vf(a1:2:a2),':m')
elseif Ns == 3
    figure(1),plot(vt(1:4:N),Vi(a1:4:a2),':b',vt(1:4:N),Vf(a1:4:a2),':b')
elseif Ns == 4
    figure(1),plot(vt(1:8:N),Vi(a1:8:a2),':r',vt(1:8:N),Vf(a1:8:a2),':r')
elseif Ns == 5
    figure(1),plot(vt(1:16:N),Vi(a1:16:a2),':g',vt(1:16:N),Vf(a1:16:a2),':g')
elseif Ns == 6;
    figure(1),plot(vt(1:32:N),Vi(a1:32:a2),':m',vt(1:32:N),Vf(a1:32:a2),':m')
elseif Ns == 7
    figure(1),plot(vt(1:64:N),Vi(a1:64:a2),':b',vt(1:64:N),Vf(a1:64:a2),':b')
else Ns == 8
    figure(1),plot(vt(1:128:N),Vi(a1:128:a2),':r',vt(1:128:N),Vf(a1:128:a2),':r')
end

