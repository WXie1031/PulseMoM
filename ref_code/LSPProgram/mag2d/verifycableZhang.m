% number of conductor:
% layer1: 3
% layer2: 9
% layer3: 15
% layer4: 20
% total: 47
% number of iron wire: 48
% rshield: radius of shield layer
% rmag: radius of iron wire
%% 1.generate the path
length = 0.3;
path = 0:length/20:length;
Nseg = size(path,2);
len = path(2)-path(1);
frq = 1e4;
shp_id = ones(95,1);
rc = 0.95e-3;
rin = 0.05e-3;
rshield = 12e-3;
rmag = rshield*sin(3.75/360*2*pi);
rtotal = rc+rin;
theta1 = 0:2*pi/3:2*pi;
theta1(4)= [] ;
theta1 = theta1+pi/3;
theta2 = 0:2*pi/9:2*pi;
theta2(10) = [];
theta3 = 0:2*pi/15:2*pi;
theta3(16) = [];
theta4 = 0:2*pi/20:2*pi;
theta4(21) = [];
thetamag = 0:2*pi/48:2*pi;
thetamag(49) = [];
x1 = 2/sqrt(3)*rtotal*cos(theta1);
y1 = 2/sqrt(3)*rtotal*sin(theta1);
x2 = 3*rtotal*cos(theta2);
y2 = 3*rtotal*sin(theta2);
x3 = 5*rtotal*cos(theta3);
y3 = 5*rtotal*sin(theta3);
x4 = 7*rtotal*cos(theta4);
y4 = 7*rtotal*sin(theta4);
xmag = rshield*cos(thetamag);
ymag = rshield*sin(thetamag);
pt_2d = [x1,x2,x3,x4,xmag;y1,y2,y3,y4,ymag];
pt_2d = pt_2d';
figure(1);
alpha = 0:pi/40:2*pi;
for i = 1:47
    plot(pt_2d(i,1)+rc*cos(alpha),pt_2d(i,2)+rc*sin(alpha),'-')
    hold on
end
for i = 48:95
    plot(pt_2d(i,1)+rmag*cos(alpha),pt_2d(i,2)+rmag*sin(alpha),'-')
    hold on
end
%%
dim1 = [repmat(rc,47,1);repmat(rmag,48,1)];
dim2 = zeros(95,1);
sig = repmat(5.8e7,95,1);
mu = 10;
mur = [ones(47,1);mu*ones(48,1)];
epr = ones(95,1);
S = pi.*(dim1.^2-dim2.^2);
Rpul = 1./(sig.*S);
[~,~,Pmesh,~,~,RmeshMF,LmeshMF] = main_mesh2d_cmplt(shp_id, pt_2d, dim1,dim2, ...
    Rpul, sig,mur,epr, len, frq);
%%
% source points and field points
sourcepoint1 = [zeros(95,1),pt_2d];
sourcepoint2 = [len*ones(95,1),pt_2d];
dir_z1 = kron(path(2:Nseg-1)',ones(95,1));
dir_z2 = kron(path(3:Nseg)',ones(95,1));
dir_xy = repmat(pt_2d,Nseg-2,1);
pf1 = [dir_z1,dir_xy];
pf2 = [dir_z2,dir_xy];
Lm = zeros(95,(Nseg-2)*95);
r = [rc*ones(47,1);rmag*ones(48,1)];
[dv2,l2] = line_dv(pf1,pf2);
r2 = repmat(r,Nseg-2,1);
for ik=1:95
    ps1 = sourcepoint1(ik,:);
    ps2 = sourcepoint2(ik,:);
    [dv1, l1] = line_dv(ps1, ps2);
    r1 = r(ik);
    Lcol = cal_L_fila(ps1,ps2,dv1,l1,r1,pf1,pf2,dv2,l2,r2);
    Lm(ik,:)=Lcol;
end
