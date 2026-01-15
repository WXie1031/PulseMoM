%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% SR_DExp_sub sub program of SR_DExp. Calculation Alpha and Beta and other
%               parameters according rise and fall time.
% %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% INPUT %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% tr            rise time of waveform (us)
% tf            fall time of waveform (us)
% %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% OUTPUT %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% valores       parameters for double exponential function setting
% %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% revised by Chen Hongcai 2014.8
function [amp, tau1, tau2] = sr_double_exp_para_china(amp, trise, tfall)

% Unit(us)

[xresult, fval] = fminsearch(@(x) sr_double_exp_para_china_sub( ...
    x, trise, tfall),  [1, 1e3*1e-6, 1e5*1e-6, trise, -2], ...
    optimset('Display','final', ...
    'TolFun',1e-12,'TolX',1e-12,'MaxFunEvals',1e6,'MaxIter',1e6));


amp = amp * xresult(1);
tau1 = xresult(2)*1e6;
tau2 = xresult(3)*1e6;



figure;
Tmax = tfall*1.2;     % us
dt = 1e-2;
Nt = ceil((Tmax)/dt);
t = (0:Nt-1)*dt;
t = t*1e-6;
ist = amp  * (exp(-tau1*t)-exp(-tau2*t));


plot(t*1e6,ist);
grid on


end


