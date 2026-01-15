%
% Function to compute line parameters in frequency domain
%
% Inputs
%
%    Z   ==> Total impedance
%    Y   ==> Total admitance
%    Lo  ==> Line lenght
%    w   ==> Vector of frequencies in radian/sec.
%
% Outputs
%    Yc   ==> Characteristic admittance
%    Vm   ==> Modal velocities
%    Hmo  ==> Modal H
%
% How to call the function
%
%  function [Yc,Vm,Hmo] = ABYZLM(Z,Y,Lo,w)
%
%
function [Yc,Vm,Hmo] = ABYZLM(Z,Y,Lo,w)

[M, Lm] = eig(Y*Z);                   % Eigenvalues of YZ
Minv    = inv(M);                     % Inverse of eigenvector matrix
Yc      = inv(Z)*(M*sqrt(Lm)*Minv);   % Characteristic admittance
Gamma   = sqrt(diag(Lm));             % Gamma
Vm      = w./imag(Gamma);             % Modal Velocities
Hmo     = diag(expm(-sqrtm(Lm)*Lo));  % Modal H
