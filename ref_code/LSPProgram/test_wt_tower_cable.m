
shp_id = [1100; 2100; 2100;];
pt_2d = [0  0;
    0   1940;
    0   1000]*1e-3;
dim1 = [2000; 10; 10]*1e-3;
dim2 = [1995; 0;  0]*1e-3;
re = dim1;

Nc = size(pt_2d,1);

sig = 5.8e7*ones(Nc,1);
mur = 1*ones(Nc,1);
epr = 1*ones(Nc,1);
len = 5;

S = pi*(dim1.^2-dim2.^2);
Rpul = 1./(sig.*S);


pt_start = [pt_2d zeros(Nc,1)];
pt_end = [pt_2d len*ones(Nc,1)];


% mesh2d model
f0 = 50e3;
flag_p = 1;
[Rmesh, Lmesh, Pmesh]= main_mesh2d_cmplt(shp_id, pt_2d, dim1, dim2, ...
    Rpul, sig,mur,epr, len, f0, flag_p);
Cmesh = inv(full(Pmesh));
C0 = -Cmesh(1,2);


% filament model
[dv, len] = line_dv(pt_start, pt_end);
flag_p = 1;
[Rmtx,Lmtx,Pmtx] = para_main_fila_rlp(pt_start,pt_end,dv,re,len, ...
    pt_start,pt_end,dv,re,len, flag_p);
Lmtx = full(Lmtx);
Cmtx = inv(full(Pmtx));
C1 = -Cmtx(1,2);

% chinese paper model
ep0 = 8.85*1e-12;
D0 = dim1(1) - sqrt(sum( (pt_2d(1,:)-pt_2d(2,:)).^2 ));
C2 = 2*pi*ep0*epr(1)*len(1)/( log(len(1)/D0+sqrt((len(1)/D0)^2-1)) );


% chongqing paper model
C3 = 2*pi*ep0*epr(1)*len(1)/acosh( (dim1(1)^2+dim1(2)^2-D0^2)/(2*dim1(1)*dim1(2)) );


%% conclusion
% WT tower cannot be calculated in meshing mode






