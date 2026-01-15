


Nf = 100;

frq = logspace(3,10,Nf);

% frq=1e9;
% Nf = length(frq);

epr_lyr = [ 1; 1; 5-1j*4e5; 1; 1];
mur_lyr = [1; 1; 1; 1; 1;];
sig_lyr = [0; 5.8e7; 0; 5.8e7; 0];
zbdy = [0; -0.25; -0.5; -0.75; ]*1e-3;
 
epr_lyr = [ 1; 1; 1; 1; 1;];
mur_lyr = [1; 1; 1; 1; 1;];
sig_lyr = [0; 11.8e6; 0; 11.8e6; 0];
zbdy = [0; -0.25;-2.75;-3; ]*1e-3;


T = zeros(Nf,1); 
for ik = 1:Nf
    f0 = frq(ik);
    w0= 2*pi*f0;

    T(ik) = multi_shield_se(zbdy, epr_lyr,mur_lyr,sig_lyr, w0);
end


SE = -20*log10(abs(T));

figure(3)

semilogx(frq,SE)
xlabel('Frequency (Hz)')
ylabel('Shielding Efficiency (dB)')
hold on



