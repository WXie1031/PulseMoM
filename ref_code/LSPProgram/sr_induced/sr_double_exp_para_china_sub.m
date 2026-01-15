function y = sr_double_exp_para_china_sub(x, trise, tfall)
% according to GB/T 16927, however, equations are less than unknowns.   
% trise = (t90-t10)*1.25  -->  t90 = trise/1.25+t10
% k * (exp(-a*t10)-exp(-b*t10)) = 0.1
% k * (exp(-a*t90)-exp(-b*t90)) = 0.9
% t50 = tfall
% k * (exp(-a*t50)-exp(-b*t50)) = 0.5

% according to paper "标准雷电波形的频谱分析及其应用", equations are listed as
% t10 = 0.1*trise - t0;
% t90 = 0.9*trise - t0;
% k * (exp(-a*t10)-exp(-b*t10)) = 0.1
% k * (exp(-a*t90)-exp(-b*t90)) = 0.9
% t50 = tfall - t0;
% k * (exp(-a*t50)-exp(-b*t50)) = 0.9
% 
% k * (exp(-a*tmax)-exp(-b*tmax)) = 1
% a*exp(-a*tmax)-b*exp(-b*tmax) = 0

k = x(1);
a = 1/x(2);
b = 1/x(3);
tmax = x(4);
t0 = x(5);


F1 = k * ( exp(-a*(0.1*trise-t0)) - exp(-b*(0.1*trise-t0)) ) - 0.1; 
F2 = k * ( exp(-a*(0.9*trise-t0)) - exp(-b*(0.9*trise-t0)) ) - 0.9;
F3 = k * ( exp(-a*(tfall-t0)) - exp(-b*(tfall-t0)) ) - 0.5;
F4 = k * ( exp(-a*tmax) - exp(-b*tmax) ) - 1;
F5 = a*exp(-a*tmax) - b*exp(-b*tmax);

y= F1^2 + F2^2 + F3^2 + F4.^2 + F5.^2; 


end


