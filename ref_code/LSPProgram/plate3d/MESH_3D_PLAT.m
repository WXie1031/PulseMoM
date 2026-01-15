% MESH_3D_PLAT   Generating coordinates of cells for M1 zones
% 
% Ordering: lower -> upper (x -> y)
%           left  -> right
%            1  2  3  4  5  6  7  8
% Plat.NODE=[x1,x2,y1,y2 x0 y0    Ndex]         % Node  cell coordinates
% Plat.XCEL=[x1,x2,y1,y2 x0 y0 dy Bxdex]        % X-dir cell coordinates
% Plat.YCEL=[x1,x2,y1,y2 x0 y0 dx Bydex]        % Y-dir cell coordinates
% Plat.Z=[z1 z2]                % Z coordinates of plate (one row)
% Plat.NCuv=[x;y]               % ref pts along u a nd v for meshing
% 15 April 2016
%
function Plat=MESH_3D_PLAT(Plat)
nx=Plat.num(1);         % node cell # in x dir
ny=Plat.num(2);         % node cell # in y dir
wth=Plat.size(1);       % widthof the plate
dth=Plat.size(2);       % depth of the plate
hht=Plat.size(3);       % height of the plate

x0=0.5*linspace(-wth,wth,nx+1)+Plat.cent(1); 
y0=0.5*linspace(-dth,dth,ny+1)+Plat.cent(2); 
Plat.Z=0.5*[-hht hht]+Plat.cent(3);
Plat.NC_U=x0;
Plat.NC_V=y0;

% (1) create node cell coordinates
x=repmat([x0(1:nx);x0(2:nx+1)]',ny,1);
y=[];
for ik=1:ny
    tmp=[y0(ik) y0(ik+1)];
    y=[y; repmat(tmp,nx,1)];
end
% Plat.NODE=[x1 x2 y1 y2 x0 y0 index]
Plat.NODE=[x y 0.5*(x(:,1)+x(:,2)) 0.5*(y(:,1)+y(:,2)) (1:nx*ny)'*0 (1:nx*ny)'];                        

% (2) create x dir cell coordinates
x=[];
y=[];
for ik=1:ny
    tmp1=0.5*(x0(1:nx-1)+x0(2:nx));
    tmp2=0.5*(x0(2:nx)+x0(3:nx+1));
    x=[x; [tmp1; tmp2]'];
    tmp=[y0(ik) y0(ik+1)];
    y=[y; repmat(tmp,nx-1,1)];
end
tmp=[x y 0.5*(x(:,1)+x(:,2)) 0.5*(y(:,1)+y(:,2))];   % x-curret cell
Plat.XCEL=[tmp abs(y(:,2)-y(:,1)) (1:nx*ny-ny)'];

% (3) create y dir cell coordinates
x=[];
y=[];
for ik=1:ny-1
    tmp=[x0(1:nx);x0(2:nx+1)];
    x=[x; tmp'];
    tmp=0.5*[y0(ik)+y0(ik+1) y0(ik+1)+y0(ik+2)];
    y=[y; repmat(tmp,nx,1)];
end
tmp=[x y 0.5*(x(:,1)+x(:,2)) 0.5*(y(:,1)+y(:,2))];   % y-curret cell
Plat.YCEL=[tmp abs(x(:,2)-x(:,1)) (1:nx*ny-nx)'];

% (4) bran and node relationship

end