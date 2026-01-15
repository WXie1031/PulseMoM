function [I] = GenerateWaveformCurrentShao(t,tau1,tau2,n,I01)

t = t-2e-4;
I = I01*exp((1/tau1)*t)./(1+exp(((1/tau1+1/tau2))*t));
I = I/max(I)*I01;

N = length(I);
I = [zeros(2000,1);I(1:N-2000)];



end


