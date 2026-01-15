
depth = 1;
len = 0.5;

pt_start = [0 0 -depth];
pt_end = [len 0  -depth];

dv_grid = [1 0  0];

re = [5e-3];

sig_soil = 1/150;
Rsoil=1/sig_soil;
epr_soil = 10;


R0=grid_rod_resis_h([], pt_start,pt_end, dv_grid, re, len, Rsoil)

% R1 = Rsoil/pi/len*(log(2*len/sqrt(2*re*depth))-1)
% 
% %R2 =  Rsoil/pi/len*(log(len^2/(2*re*depth))-0.61)
% 
% R3 =  Rsoil/pi/len/2*(log(len^2/(2*re*depth)))  % relative good
% 
% %R4 =  Rsoil/pi/len*(log(2*len/re)-1)
% 
% R5 =  Rsoil/2/pi/len*(log(len^2/(2*re*depth))+0.48)  % relative best
% 
% R6 =  Rsoil/2/pi/len*(log(len/re)+log(len/(2*depth)))  % same as R3

rv = re*linspace(1,0.2e6,1000);
for ik = 1:length(rv)
R0(ik) = grid_rod_resis_h([], pt_start,pt_end, dv_grid, rv(ik), len, Rsoil);
end
figure;
plot(rv,R0./rv);

