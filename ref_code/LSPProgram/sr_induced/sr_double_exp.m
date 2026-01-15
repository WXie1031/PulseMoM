function [ist, tus, ts] = sr_double_exp(amp, tr, tf, Tmax, dt)
%  Function:       sr_double_exp
%  Description:    compute the double exponential function (for lightning source) 
%                  based on rise and fall time. Alpha and Beta parameters
%                  are calculated according time in sr_double_exp_para.
%
%  Calls:          sr_double_exp_para
%
%  Input:          Amp  --  amplitude of the lightning source
%                  tr   --  rise time of waveform (us)
%                  tf   --  fall time of waveform (us)
%                  Tmax --  the duration of the lightning (us)
%  Output:         ist  --  output of the source ( U or I )
%                  t    --  the time sequence (us)
%  Others:
%  Author:         Chen Hongcai
%  Email :         hc.chen@live.com
%  Date:           2014-08-16

Nt = ceil((Tmax)/dt);
tus = (0:Nt-1)*dt;

[amp, tau1, tau2] = sr_double_exp_para(amp, tr, tf);

ist = amp  * (exp(-tau1*tus)-exp(-tau2*tus));

ts = tus*1e-6;

end


