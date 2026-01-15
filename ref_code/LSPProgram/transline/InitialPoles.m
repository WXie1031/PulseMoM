%
% Function to set the initial poles
%
% Input
%
%    f     ==> Vector of frequencies
%    Npol  ==> Number of poles
%
% Outputs
%
%    Ps  ==> Column vector of the initial poles
%
% Call the function
%
%    [Ps]=InitialPoles(f,Npol)
%
%
function [Ps]=InitialPoles(f,Npol)

% Set the initial poles
even  = fix(Npol/2);     % Number of complex initial poles
p_odd = Npol/2 - even;   % Auxiliar variable to check if the initial poles are odd
disc  = p_odd ~= 0;      % Put  0 - even initial poles  &  1 - odd initial poles

% Set a real pole in case of disc == 1
if disc == 0    % Even initial poles
    pols = [];
else            % Odd initial poles
    pols = [(max(f)-min(f))/2];
end

% Set the complex initial poles
bet = linspace(min(f),max(f),even);
for n=1:length(bet)
  alf=-bet(n)*1e-2;
  pols=[pols (alf-j*bet(n)) (alf+j*bet(n)) ]; % OrIgInAl
end

Ps = pols.';  % Column vector of the initial poles
