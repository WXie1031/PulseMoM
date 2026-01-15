function [I] = GenerateWaveformCurrentGaussian(t,tau1,tau2,n,I01)

t = t-max(t)/2;

I1 = I01*exp(-(t).^2./(2*tau1^2));
I2 = I01*exp(-(t).^2./(2*tau2^2));

I(t<=0) = I1(t<=0);
I(t>0) = I2(t>0);

I = I/max(I)*I01;
I = I';
N = length(I);
I = [zeros(2000,1);I(1:N-2000)];



end


