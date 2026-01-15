fpath = 'F:\data_station\';
fpath = [];
fname = [fpath, 'hit2_no_e_m1_4.txt'];

% [vout, t] = fdtd_v(fname);
[vout, t] = fdtd_ez2v(fname, 0.05,4);
Npt = size(vout,2);
vout = vout'/10;
figure(1)
hold on
for ik = 2:2
plot(t,(vout(:,1)-vout(:,ik))/1e3);
end
ylabel('Potential Difference(kV)')
xlabel('Time(us)')

% fpath = 'F:\Chen\Dropbox\Duke\FDTD_Project\Station_grid_hit1\';
% fname = [fpath, 'v_m1_m4.txt'];

%[vout, t] = fdtd_v(fname);
figure(2)
hold on
for ik = 3:3
plot(t,(vout(:,1)-vout(:,ik))*0.75/1e3,'--');
end
ylabel('Potential Difference(kV)')
xlabel('Time(us)')

figure(3)
hold on
for ik = 4:4
plot(t,(vout(:,1)-vout(:,ik))/1e3,'--');
end
ylabel('Potential Difference(kV)')
xlabel('Time(us)')



