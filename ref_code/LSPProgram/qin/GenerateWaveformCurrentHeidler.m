function [I] = GenerateWaveformCurrentHeidler(t,tau1,tau2,n,I01)

yita1 = exp(-tau1/tau2*(n*tau2/tau1)^(1/n));
I     = I01*exp(-t/tau2)/yita1.*(t/tau1).^n./((t/tau1).^n+1);
I = I/max(I)*I01;

N = length(I);
I = [zeros(2000,1);I(1:N-2000)];



end


