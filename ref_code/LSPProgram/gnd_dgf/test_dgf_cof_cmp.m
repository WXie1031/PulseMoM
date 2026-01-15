na=1; nb=1.5; 
n1=1.38; n2=2.45; 
n = [na,n1,n2,nb]; 
la0 = 550; 
r = n2r(n);
c = sqrt((r(1)^2*(1-r(2)*r(3))^2 - (r(2)-r(3))^2)/(4*r(2)*r(3)*(1-r(1)^2))); 
k2l2 = acos(c); G2 = (r(2)+r(3)*exp(-2*1i*k2l2))/(1 + r(2)*r(3)*exp(-2*1i*k2l2)); 
k1l1 = (angle(G2) - pi - angle(r(1)))/2; 
if k1l1 <0, 
    k1l1 = k1l1 + 2*pi;
end
L = [k1l1,k2l2]/2/pi;
la = linspace(400,700,101); 

% [Gamma,Z] = multidiel(n,L,lambda,theta,pol)
% n      = isotropic 1x(M+2), uniaxial 2x(M+2), or biaxial 3x(M+2), matrix of refractive indices
% L      = vector of optical lengths of layers, in units of lambda_0
% lambda = vector of free-space wavelengths at which to evaluate the reflection response
% theta  = incidence angle from left medium (in degrees)
% pol    = for 'tm' or 'te', parallel or perpendicular, p or s, polarizations
Ga = abs(multidiel(n, L, la/la0)).^2 * 100; 
Gb = abs(multidiel([na,n1,nb], 0.25, la/la0)).^2 * 100; 
Gc = abs(multidiel([na,sqrt(nb),nb], 0.25, la/la0)).^2 * 100;

plot(la, Ga, la, Gb, la, Gc); 