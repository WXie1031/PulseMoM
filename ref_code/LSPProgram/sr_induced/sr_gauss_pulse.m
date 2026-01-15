function [ist, ts] = sr_gauss_pulse(amp, tau, Tmax, dt, td)
%  Function:       sr_gauss_pulse
%  Description:    Guassian pulse used in lightning research.
%
%  Calls:          
%
%  Input:          amp  --  amplitude of the lightning source
%                  tau  --  the stoke duration in [s]. 
%                  Tmax --  ending time in [s].
%                  dt   --  time interval [s]
%  Output:         ist  --  output of the source ( U or I )
%                  ts   --  the time sequence (s)
%  Others:
%  Author:         Chen Hongcai
%  Email :         hc.chen@live.com
%  Date:           2018-10-16


Nt = ceil(Tmax/dt);
% Nt = ceil((Tmax-td)/dt);

ts = (0:Nt-1)*dt;
ist = amp * exp( -(ts-td).^2/(tau^2) );

% Ntd = Ntm-Nt;
% ts = [ts, (Nt-1)*dt+(1:Ntd)*dt];
% ist = [zeros(1,Ntd), ist];



end


