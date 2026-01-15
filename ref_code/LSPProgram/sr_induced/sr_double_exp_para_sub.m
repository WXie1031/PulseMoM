function y = sr_double_exp_para_sub(x, t1, t2)

k=x(1);
a=-x(2);
b=-x(3);
t01=x(4);
t09=x(5);

F1 = 1- ( k * ((a/b) ^ (a / (b-a))) * (b - a) / b);

F2 = exp(a*t2)-exp(b*t2) - (0.5 * ((a/b) ^ (a / (b - a)) ) * (b - a) / b);

%F3 = (t1/1.67) - (t09 - t01); % for voltage
F3 = (t1/1.25) - (t09 - t01); % for voltage

F4 = (0.9) - ( k * ( exp(a*t09) - exp(b*t09) ) );
F5 = (0.1) - ( k * ( exp(a*t01) - exp(b*t01) ) );

y= F1^2 + F2^2 + F3^2 + F4^2 + F5^2; 


