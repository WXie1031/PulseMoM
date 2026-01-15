function [ist, tus, ts] = sr_heidler(amp, tau1, tau2, n, Tmax, dt, td)
%  Function:       sr_heidler
%  Description:    Lightning Source Generation using HEIDLER Function.
%
%  Calls:          
%
%  Input:          Amp  --  amplitude of the lightning source
%                  Tf   --  the front duration in [s]. Intercal between t=0 to the time
%                           of the function peak.
%                  tau  --  the stoke duration in [s]. Interval between t=0 and the
%                           point on the tail whrer the function amplitude has fallen 
%                           to 37% of its peak value.
%                  n    --  factor influencing the rate of rise of the function.
%                           Increased n increases the maximum steepnes.
%                  Tmax --  ending time in [us].
%                  dt   --  time interval [us]
%  Output:         ist  --  output of the source ( U or I )
%                  t    --  the time sequence (us)
%  Others:
%  Author:         Chen Hongcai
%  Email :         hc.chen@live.com
%  Date:           2013-12-16

if nargin<7
    td=0;
end


Ntm = ceil(Tmax/dt);
Nt = ceil((Tmax-td)/dt);

tus = (0:Nt-1)*dt;
ist = amp*(tus/tau1).^n./(1+(tus./tau1).^n).*exp(-tus./tau2);

Ntd = Ntm-Nt;
tus = [tus, (Nt-1)*dt+(1:Ntd)*dt];
ist = [zeros(1,Ntd), ist];

ts = tus*1e-6;

end


