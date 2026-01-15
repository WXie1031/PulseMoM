function [Uout0]=LP_Filter0(Uout,f_low,f_high,dt)
% wp2=2*f_low*dt;
% ws2=2*f_high*dt;
if dt>1e-6
dt=dt/100;
elseif dt>1e-7
    dt=dt/10
end

wp2=2*f_low*dt;
ws2=2*f_high*dt;
ap2=1;
as2=50;
[N2 Wc2]=buttord(wp2,ws2,ap2,as2);
[H2 W2]=freqz(N2,Wc2);
[B2,A2]=butter(N2,Wc2);
Uout0=filter(B2,A2,Uout);
end