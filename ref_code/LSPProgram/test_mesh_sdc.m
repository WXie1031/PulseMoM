
clear


flag = 4;


%f0 = [1 50 100 200 500 1e3 2e3 5e3 10e3 20e3 50e3 100e3 200e3 500e3 1e6];
f0 = [1   1e3    10e3  100e3   1e6];
%f0 = [1 50 100 200 500 1e3 2e3 5e3 10e3 20e3 50e3 100e3 200e3];

len_tmp = 1;


Nf = length(f0);
w0 = 2*pi*f0;


% sdc
if flag == 1
    shape_cs2D = [2100; 2100; 2100;];
    pt_cs2D = [0  0; 0  -2; 0  2;]*1e-3;
    dim1_cs2D = [4.4; 1.2; 1.2;]*1e-3;
    dim2_cs2D = [3.9;   0;   0;]*1e-3;
    re_cs2D = dim1_cs2D;
    Rin_pul_cs2D = [14.85; 5.28; 5.28]*1e-3;
    Lin_pul_cs2D = zeros(length(shape_cs2D),1);
    
    Nc = size(pt_cs2D,1);
    mur = 1*ones(Nc,1);
    len = len_tmp*ones(Nc,1);
    
    pt_3d1 = [pt_cs2D, [0; 0; 0;]];
    pt_3d2 = [pt_cs2D, [0; 0; 0;]+len];
    dv1 = [0  0  1; 0  0  1; 0  0  1;];
    
end



if flag == 2
    % 2-3-sdc
    
    pt_cs2D = [     0.1000    0.0125 ;
        0.1000    0.0125     ;
        0.1000    0.0525     ;
        0.1000    0.0525     ;
        0.1000    0.0925      ;
        0.1000    0.0925     ;
        0.1400    0.0125     ;
        0.1400    0.0125     ;
        0.1400    0.0525      ;
        0.1400    0.0525     ;
        0.1400    0.0925   ;
        0.1400    0.0925    ;
        0.1800    0.0040   ;
        0.1780    0.0040   ;
        0.1820    0.0040    ;];
    
    Nc = size(pt_cs2D,1);
    
    dim1_cs2D = [0.0124;
        0.0040;
        0.0124;
        0.0040;
        0.0124;
        0.0040;
        0.0124;
        0.0040;
        0.0124;
        0.0040;
        0.0124;
        0.0040;
        0.0040;
        0.0013;
        0.0013;];
    dim2_cs2D = [    0.0116;
        0;
        0.0116;
        0;
        0.0116;
        0;
        0.0116;
        0;
        0.0116;
        0;
        0.0116;
        0;
        0.0034;
        0;
        0;];
    re_cs2D = dim1_cs2D;
    sig = [    58000000;
        58000000;
        58000000;
        58000000;
        58000000;
        58000000;
        58000000;
        58000000;
        58000000;
        58000000;
        58000000;
        58000000;
        58000000;
        58000000;
        58000000;];
    S = pi*(dim1_cs2D.^2-dim2_cs2D.^2);
    Rin_pul_cs2D = 1./(sig.*S);
    Lin_pul_cs2D = zeros(Nc,1);
    
    mur = 1*ones(Nc,1);
    len = len_tmp*ones(Nc,1);
    
    shape_cs2D = 2100*ones(Nc,1);
    
    pt_3d1 = [pt_cs2D, zeros(Nc,1)];
    pt_3d2 = [pt_cs2D, zeros(Nc,1)+len];
    dv1 = ones(Nc,1)*[0  0  1];
end


if flag == 3
    % 2-3-sdc
    
    pt_cs2D = [ 0.1000    0.0125    ;
        0.1000    0.0125         ;
        0.1375    0.0125         ;
        0.1375    0.0125         ;
        0.1750    0.0125         ;
        0.1750    0.0125         ;
        0.2125    0.0125         ;
        0.2125    0.0125         ;
        0.2500    0.0125         ;
        0.2500    0.0125         ;
        0.2875    0.0125         ;
        0.2875    0.0125         ;
        0.3150    0.0040        ;
        0.3130    0.0040       ;
        0.3170    0.0040      ;];
    
    Nc = size(pt_cs2D,1);
    
    dim1_cs2D = [0.0124;
        0.0040;
        0.0124;
        0.0040;
        0.0124;
        0.0040;
        0.0124;
        0.0040;
        0.0124;
        0.0040;
        0.0124;
        0.0040;
        0.0040;
        0.0013;
        0.0013;];
    dim2_cs2D = [    0.0116;
        0;
        0.0116;
        0;
        0.0116;
        0;
        0.0116;
        0;
        0.0116;
        0;
        0.0116;
        0;
        0.0034;
        0;
        0;];
    re_cs2D = dim1_cs2D;
    sig = [    58000000;
        58000000;
        58000000;
        58000000;
        58000000;
        58000000;
        58000000;
        58000000;
        58000000;
        58000000;
        58000000;
        58000000;
        58000000;
        58000000;
        58000000;];
    S = pi*(dim1_cs2D.^2-dim2_cs2D.^2);
    Rin_pul_cs2D = 1./(sig.*S);
    Lin_pul_cs2D = zeros(Nc,1);
    
    mur = 1*ones(Nc,1);
    len = len_tmp*ones(Nc,1);
    
    shape_cs2D = 2100*ones(Nc,1);
    
    pt_3d1 = [pt_cs2D, zeros(Nc,1)];
    pt_3d2 = [pt_cs2D, zeros(Nc,1)+len];
    dv1 = ones(Nc,1)*[0  0  1];
end




if flag == 4
    % Asysmmetical case
    
    pt_cs2D = [ 0	 0;...
        0	 5;...
        -5	 0;...
        0	-5;...
        5	 0;]*1e-3*2;
    
    %
%     pt_cs2D = [
%         0	 0;
%         5	 0;...
%         10	 0;...
%         15	 0;...
%         20	 0;]*1e-3*6;
% %     % %
%     pt_cs2D = [0	 0;
%         0   5;...
%         5	 5;...
%         0	-5;...
%         5	-5;...
%         %              5   0;
%         ]*1e-3*6;
    
    Nc = size(pt_cs2D,1);
    
    dim1_cs2D = 5*1e-3*ones(Nc,1);
    dim2_cs2D = zeros(Nc,1);
    %dim2_cs2D = 2*1e-3*ones(Nc,1)*4;
    re_cs2D = dim1_cs2D;
    sig = 5.8e7;
    S = pi*(dim1_cs2D.^2-dim2_cs2D.^2);
    Rin_pul_cs2D = 1./(sig*S);
    Lin_pul_cs2D = zeros(Nc,1);
    
    mur = 1*ones(Nc,1);
    len = len_tmp*ones(Nc,1);
    
    shape_cs2D = 2100*ones(Nc,1);
    
    pt_3d1 = [pt_cs2D, zeros(Nc,1)];
    pt_3d2 = [pt_cs2D, zeros(Nc,1)+len];
    dv1 = ones(Nc,1)*[0  0  1];
end

if flag == 5
    % paper 1 meshing
    dis = 20;
    pt_cs2D = [ 0 0 0;
         0 dis 0;
        -dis*cos(pi/6)  -dis*sin(pi/6) 0;
         dis*cos(pi/6)  -dis*sin(pi/6) 0;]*1e-3;
    
    Nc = size(pt_cs2D,1);
    
    dim1_cs2D = [0.982 9.5 9.5 9.5]'*1e-3;
    dim2_cs2D = [0 8.5 8.5 8.5]'*1e-3;
    %dim2_cs2D = 2*1e-3*ones(Nc,1)*4;
    re_cs2D = dim1_cs2D;

    sig_cu = 5.8e7;
    sig_al = 3.78e7;
    sig = [sig_cu sig_al sig_al sig_al]';
    
    S = pi*(dim1_cs2D.^2-dim2_cs2D.^2);
    Rin_pul_cs2D = 1./(sig.*S);
    Lin_pul_cs2D = zeros(Nc,1);
    
    mur = 1*ones(Nc,1);
    len = len_tmp*ones(Nc,1);
    
    shape_cs2D = 2100*ones(Nc,1);
    
    pt_3d1 = [pt_cs2D, zeros(Nc,1)];
    pt_3d2 = [pt_cs2D, zeros(Nc,1)+len];
    dv1 = ones(Nc,1)*[0  0  1];
end


S = pi*(dim1_cs2D.^2-dim2_cs2D.^2);
Sig_pul_cs2D = 1./(Rin_pul_cs2D.*S);


[Rmesh, Lmesh, Rms, Lms, RmMF, LmMF]= mesh2d_main_complete( ...
    shape_cs2D, pt_cs2D, dim1_cs2D, dim2_cs2D, Rin_pul_cs2D, Lin_pul_cs2D, mur, len_tmp, f0);

[ Rskin, Lskin] = para_self_multi_frq(shape_cs2D, dim1_cs2D, dim2_cs2D, ...
    len, Rin_pul_cs2D, Sig_pul_cs2D, mur*ones(1,Nf), f0);


L2d = [1.2493; 1.24928; 1.24921; 1.24895; 1.2472; 1.242499; 1.2331; 1.221; 1.21478; 1.2103; 1.2062; 1.2042; 1.20289;1.20249];

dfR = abs(Rms-Rskin)./Rms*100;
dfL = abs(Lms-Lskin)./Lms*100;

Zms = zeros(Nc,Nf);
Zskin = zeros(Nc,Nf);
for ik = 1:Nc
    Zms(ik,:) = Rms(ik,:)+1j*w0.*Lms(ik,:);
    Zskin(ik,:) = Rskin(ik,:)+1j*w0.*Lskin(ik,:);
end

dfZ = abs(abs(Zms)-abs(Zskin))./abs(Zms)*100;

format bank
Zms = Zms*1e3;
Zskin = Zskin*1e3;

Rms = Rms*1e3;
Rskin = Rskin*1e3;

Lms = Lms*1e6;
Lskin = Lskin*1e6;


figure(5);
hold on
for ik = 1:Nc
    pt_plt = [pt_cs2D(ik,:)-re_cs2D(ik) re_cs2D(ik)*2 re_cs2D(ik)*2];
    rectangle('Position',pt_plt,'Curvature',[1,1],  'FaceColor','r');
    pt_plt = [pt_cs2D(ik,:)-dim2_cs2D(ik) dim2_cs2D(ik)*2 dim2_cs2D(ik)*2];
    rectangle('Position',pt_plt,'Curvature',[1,1],  'FaceColor','b');
end
axis equal
hold off


