% GEOPLOT

function GEOPLOT(u,v,z,Qs)

figure(1);hold off;
% (0) source lines
Qs=[Qs;Qs(1,:)];
plot3(Qs(:,1),Qs(:,2),Qs(:,3),'r','linewidth',3);
hold on;

% (1) plate
Nx=length(u);
Ny=length(v);
Nz=length(z);
x1=u(1); x2=u(Nx);
y1=v(1); y2=v(Ny);
z1=z(1); z2=z(Nz);
X1=[x1 y1 z1;x1 y1 z2;x1 y2 z2;x1 y2 z1];      % x=x1，与x轴垂直的面
X2=[x2 y1 z1;x2 y2 z1;x2 y2 z2;x2 y1 z2];      % x=x2，与x轴垂直的面
Y1=[x1 y1 z1;x2 y1 z1;x2 y1 z2;x1 y1 z2];      % y=y1，与y轴垂直的面
Y2=[x1 y2 z1;x1 y2 z2;x2 y2 z2;x2 y2 z1];      % y=y2，与y轴垂直的面
Z1=[x1 y1 z1;x1 y2 z1;x2 y2 z1;x2 y1 z1];      % z=z1，与z轴垂直的面
Z2=[x1 y1 z2;x2 y1 z2;x2 y2 z2;x1 y2 z2];      % z=z2，与z轴垂直的面
 
plot3(X1,Y1,Z1,'linewidth',2);
hold on;
plot3(X2,Y2,Z2,'linewidth',2);
 
% (2) evaluation points (M0)
hold off;

end
            
            
