

% Based on paper: "Vector Fitting-Based Calculation of Frequency-Dependent 
% Network Equivalents by Frequency Partitioning and Model-Order Reduction"
% IEEE Trans. on Power Delivery, Abner Ramirez, 2009.

% pol,res,d => (poles, residues, d-term) full system
% sys => state-space full system
% sysb => state-space balanced system
% sysr => state-space reduced system
% s => jw
% pol_new,res_new,d_new => (poles, residues, d-term) reduced system
A = diag(pol);  
B = ones(length(res),1); 
C = res;

sys = ss(A,B,C,D);
prescale(sys);
[sysb, g] = balreal(sys);
elim = find(g<5e-4);
sysr = modred(sysb, elim, 'dell');
[num, den] = tfdata(sysr, 'v');
[res_new, pol_new, d_new] = residue(num,den);
Fred(1,:) = freqresp(sysr, s);



