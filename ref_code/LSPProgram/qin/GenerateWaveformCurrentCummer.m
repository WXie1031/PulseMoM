function [I] = GenerateWaveformCurrentCummer(t,tau1,tau2,n,I01,v,L)

t = t;

I = I01*v/n*(exp(-tau1*t)-exp(-tau2*t)).*(1-exp(-n*t))/L;
N = length(I);
%I = [zeros(1,1);I(1:N-1)];

end


