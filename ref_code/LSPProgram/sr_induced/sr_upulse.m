function y = sr_upulse(t,td,tr,tf)
% upulse.m - generates trapezoidal, rectangular, triangular pulses, or a unit-step
%
% Usage: y = upulse(t,td,tr,tf)     (trapezoidal pulse)
%        y = upulse(t,0, tr,tf)     (triangular pulse)
%        y = upulse(t,td,tr)        (equal rise and fall times, equivalent to tf=tr)
%        y = upulse(t,td)           (rectangular pulse, equivalent to tr=tf=0)
%        y = upulse(t)              (unit step, equivalent to td=tr=tf=0)
%
% t  = any vector of time instants
% td = duration of flat part
% tr = rise time
% tf = fall time
%
% y = trapezoidal pulse of unit amplitude
%
% Notes: if tr=tf=0, it reduces to a rectangular pulse
%        if td=0, but tr~=0 or tf~=0, it generates a triangular pulse
%
% see also USTEP, which generates a unit-step or a rising step.

% S. J. Orfanidis - 1999 - www.ece.rutgers.edu/~orfanidi/ewa


if nargin==0, help upulse; return; end
if nargin==3, tf=tr; end                                % equal rise and fall times
if nargin==2, tr=0; tf=0; end                           % rectangular pulse
if nargin==1, td=0; tr=0; tf=0; end                     % unit step

if tr ~= 0
    y = t/tr .* p(t, tr, tr, tf) + p(t-tr, td, tr, tf);         % rising and flat parts
else
    y = p(t,td, tr, tf);                                        % zero rise or fall times
end

if tf ~= 0
    y = y + (tr+td+tf-t)/tf .* p(t-tr-td, tf, tr, tf);          % falling part
end
  
%---------------------------------------------------------------------------

function y=p(t,td,tr,tf)                                % rectangular pulse

if td==0 && tr==0 && tf==0
    y = ustep(t);                                       
else
    y = ustep(t) - ustep(t-td);
end


%---------------------------------------------------------------------------
% ustep.m - unit-step or rising unit-step function
%
% Usage: y = ustep(t)           (unit step)
%        y = ustep(t,tr)        (rising unit step, with rise time tr)
%
% t  = any vector 
% tr = rise time (tr=0 corresponds to a unit step)
%
% y = (t/tr)*[u(t)-u(t-tr)] + u(t-tr)
%
%
% see also UPULSE

% S. J. Orfanidis - 1999 - www.ece.rutgers.edu/~orfanidi/ewa

function y = ustep(t,tr)

if nargin==0, help ustep; return; end
if nargin==1, tr=0; end

y = zeros(size(t));

if tr==0
    y(t>=0) = 1; 
else
    y = (t/tr) .* (ustep(t) - ustep(t-tr)) + ustep(t-tr);
end



