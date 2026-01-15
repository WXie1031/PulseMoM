clc; close all; clear
% Axis limits
Hlim = 200; Mlim = 1.5e6;
% Begin parameters
Mmax = 1.4e6;
Href = 20;
Theta = 5;
w_1 = 6;
Htip = 170;
% End parameters
l = 60;
m_0 = mFinder(Mmax, Href, Theta, [0,0]);
Ha=linspace(0,Htip, l);
Hd=linspace(Htip,-Htip, 2*l);
Hvector(5*l-3) = 0;
Mvector(5*l-3) = 0;
% AIMC generation
delta = 1;
m = m_0;
for i=1:l
    M = delta*Mmax*((1-m)*2/pi.*atan(Theta...
        *(1-m).*(delta*Ha(i)/Href-1))+m);
    Hvector(i)=Ha(i);
    Mvector(i)=M;
end
% ILC descending branch generation
m_1 = mFinder(Mmax, Href, Theta, [-Htip,-M]);
m_2 = m_0;
DeltaH = 2*Htip;
delta=-1;
for i=1:2*l
    m = (m_2-m_1)*(1/2*cos(2*pi*(delta*Hd(i)...
        -DeltaH/2)/(2*DeltaH))+1/2)^(w_1)+m_1;
    M = delta*Mmax*((1-m)*2/pi.*atan(Theta*...
        (1-m).*(delta*Hd(i)/Href-1))+m);
    Hvector(i+l)=Hd(i);
    Mvector(i+l)=M;
end
% ILC ascending branch (mirrored copy of the
% ascending one)
Hvector=[Hvector(l:3*l), -Hd(1:2*l)];
Mvector=[Mvector(l:3*l), -Mvector(l+1:3*l)];
% Plot
plot(Hvector, Mvector)
xlabel('H [A/m]')
ylabel('M [A/m]')




