%
% Function to calculate the modal delay
%
% Input
%
%    H     ==> Modal H
%    f     ==> Vector of frequencies
%    long  ==> Line lenght
%    V     ==> Modal velocities
%    Ns    ==> Number of samples
%    Nc    ==> Number of conductors
%
% Outputs
%
%    taumin  ==> minimum delay of each mode
%
% Call the function
%
%    [taumin]=ModeDelay(H,f,long,V,Ns,Nc)
%
%
function [taumin]=ModeDelay(H,f,long,V,Ns,Nc)

w = 2*pi*f;  % Angular frequency rad/seg

% Minimal value Hm and position Hp of each propagation mode
[Hm,Hp]= min(abs(H(:,2:Ns)),[],2);

for k=1:Nc
    % Time delay
    Td(k)=long./V(k,Hp(k));
    % Minimal phase angle
    k1 = log(abs(H(k,Hp(k)+1))/abs(H(k,Hp(k)-1)));
    k2 = log((w(Hp(k)+1))/(w(Hp(k)-1)));
    phase1(k)= (pi/2)*k1/k2;
end

% Minimal phase delay in seconds
taumin   = Td + phase1./w(Hp);
