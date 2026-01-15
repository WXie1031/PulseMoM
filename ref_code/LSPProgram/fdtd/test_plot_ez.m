fpath = 'F:\Chen\Dropbox\Duke\FDTD_Project\duke_fdtd_h_rod\';
fname = [fpath, 'ez_out_h_5m_01.txt'];
[vout, t] = fdtd_ez2v(fname, 0.1);
figure(3)
hold on
plot(t,vout);

fname = [fpath, 'ez_out_h_5m_001.txt'];
[vout, t] = fdtd_ez2v(fname, 0.01);
figure(3)
hold on
plot(t,vout);

fname = [fpath, 'ez_out_h_5m_0005.txt'];
[vout, t] = fdtd_ez2v(fname, 0.005);
figure(3)
hold on
plot(t,vout);

fname = [fpath, 'ez_out_h_10m_01.txt'];
[vout, t] = fdtd_ez2v(fname, 0.1);
figure(3)
hold on
plot(t,vout);

fname = [fpath, 'ez_out_h_10m_001.txt'];
[vout, t] = fdtd_ez2v(fname, 0.01);
figure(3)
hold on
plot(t,vout);

fname = [fpath, 'ez_out_h_10m_0005.txt'];
[vout, t] = fdtd_ez2v(fname, 0.005);
figure(3)
hold on
plot(t,vout);

fname = [fpath, 'ez_out_h_15m_0005.txt'];
[vout, t] = fdtd_ez2v(fname, 0.005);
figure(3)
hold on
plot(t,vout);


fpath = 'F:\Chen\Dropbox\Duke\FDTD_Project\duke_fdtd_h_rod_thin_wire\';
fname = [fpath, 'eout_thin_wire_r01_001.txt'];
fname = [fpath, 'ez_h_thin_wire_peec_001.txt'];

[vout, t] = fdtd_ez2v(fname, 0.01);
figure(4)
hold on
plot(t,vout);


fpath = 'F:\Chen\Dropbox\Duke\FDTD_Project\duke_fdtd_4x4grid_thin_wire\';
fname = [fpath, 'ez_thin_wire_15m_001_tl.txt'];
[vout, t] = fdtd_ez2v(fname, 0.01);
figure(4)
hold on
plot(t,vout);

fpath = 'F:\Chen\Dropbox\Duke\FDTD_Project\duke_fdtd_4x4grid\';
fname = [fpath, 'ez_thin_wire_20m_001.txt'];
[vout, t] = fdtd_ez2v(fname, 0.01);
figure(4)
hold on
plot(t,vout);
