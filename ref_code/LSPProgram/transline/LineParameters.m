%
% Function to compute the line parameters as function of the frequency
%
% Inputs
%
%    Mu   = 4*pi*1E-7;                ==> Henry's/meters
%    Eo   = (1/(36*pi))*1E-9;         ==> Farads/meters
%    Rsu  = 100;                      ==> Earth resistivity Ohm-m
%    Geom                             ==> Line data geometry
%    Ncon = Geom(max(Geom(:,1)),1);   ==> Number of conductors
%    Ns   = 500;                      ==> Number of samples
%    w    = 2*pi*f;                   ==> Vector of frequencies in radian/sec.
%
% Outputs
%
%    Zg  ==> Geometric impedance
%    Zt  ==> Earth impedance
%    Zc  ==> Conductor impedance
%    Yg  ==> Geometric capacitance
%    ZT  ==> Total impedance
%    YT  ==> Total admitance
%
% The function is implemened as follows
%
%   [Zg,Zt,Zc,Yg,ZT,YT]=LineParameters(Mu,Eo,Rsu,Geom,Ncon,Ns,w)
%

function [Zg,Zt,Zc,Yg,ZT,YT]=LineParameters(Mu,Eo,Rsu,Geom,Ncon,Ns,w)

[Dij,dij,hij]=Height(Geom);  % Function to compute the distances between conductor

Zg  = zeros(Ncon,Ncon,Ns);   % Inicialize geometrical impedance matrix in zeros.
Zt  = zeros(Ncon,Ncon,Ns);   % Inicialize earth impedance matrix in zeros.
Zc  = zeros(Ncon,Ncon,Ns);   % Inicialize conductors impedance matrix in zeros.
Yg  = zeros(Ncon,Ncon,Ns);   % Inicialize geometrical admitance matrix in zeros.
Zcd = zeros(Ncon,Ns);        % Initializa the impedance of direct current
Zaf = zeros(Ncon,Ns);        % Initializa the impedance of high frequency

P       = (1./sqrt(1i*w*Mu/Rsu));  % Complex deep
Pmatrix = log(Dij./dij);          % Matrix of potentials
Pinv    = inv(Pmatrix);           % Inverse of Pmatrix

% Routine to compute the matrices in full frequency range
for kl = 1:Ns
    
    % Geometrical impedance
    Zg(:,:,kl) = (1i*w(kl)*Mu/(2*pi))*Pmatrix;
    
    % Earth impedance
    for km = 1:Ncon
        for kn = 1:Ncon
            if km == kn
                Zt(km,km,kl) = (1i*w(kl)*Mu/(2*pi))*log(1+P(kl)./(0.5*hij(km,km)));
            else
                numerador   = hij(km,kn)^2 + 4*P(kl)*hij(km,kn) + 4*P(kl)^2 + dij(km,kn)^2;
                denominador = hij(km,kn)^2 + dij(km,kn)^2;
                Zt(km,kn,kl) = (1i*w(kl)*Mu/(4*pi))*log(numerador/denominador);
            end
        end
    end
    
    % Geometrical admittance
    Yg(:,:,kl) = (1i*w(kl)*2*pi*Eo)*Pinv;
end

% Conductor impedance
for kd = 1:Ncon;
    Rcon = Geom(kd,4);             % Each conductor radii in m.
    Nhaz = Geom(kd,5);             % Number of conductor in haz
    Rpha = Geom(kd,7);             % Resistivity of each conductor in Ohm-m.
    % DC conductor impedance.
    Zcd(kd,:)  = (1/Nhaz)*Rpha./(pi.*Rcon.^2);      
    % High frequency conductor impedance.
    Zaf(kd,:)  = (1/Nhaz)*(1+1i).*(1./(2.*pi.*Rcon)) .* sqrt(0.5.*w.*Mu.*Rpha);
    % Conductor impedance
    Zc(kd,kd,:) = sqrt(Zcd(kd,:).^2 + Zaf(kd,:).^2);  
end


% Outputs
ZT = Zg + Zt + Zc ;  % Total impedance
YT = Yg ;            % Total admittance


end

