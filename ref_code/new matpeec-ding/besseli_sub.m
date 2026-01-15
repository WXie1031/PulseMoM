function Isub = besseli_sub(nu,Z)
%  Function:       besseli_sub
%  Description:    Approximation formula for modified Bessel function of 
%                  first kind. Can be used for extremely  thin case. 
%                  More stable than Matlab bessel functions.
%
%  Calls:          
%
%  Input:          nu        --  order of bessel function
%                  Z         --  unknown
%  Output:         Isub   --  result
%  Others:
%  Author:         Chen Hongcai
%  Email :         hc.chen@live.com
%  Date:           2015-06-16

u = 4*nu.*nu;

% 5 order approximation
% Isub = exp(Z)./sqrt(2*pi.*Z).*( 1 - (u-1)./(8*Z) + (u-1).*(u-9)./(2*(8*Z).^2) ...
%     - (u-1).*(u-9).*(u-25)./(6*(8*Z).^3) + (u-1).*(u-9).*(u-25).*(u-49)./(24*(8*Z).^4) ...
% 	- (u-1).*(u-9).*(u-25).*(u-49).*(u-81)./(120*(8*Z).^5) );

% 1 order approximation
Isub = exp(Z)./sqrt(2*pi.*Z).*( 1 - (u-1)./(8*Z) ); 


end

