% NB_XY    Generating incidence matrix Ax and Ay for bran(I)-node(V) 
%
% KVL: Vr+Vl+Vs-Ui+Uj=0 (i->j) --> Ax and Ay;
% KCL: -Iout+Iin=0 --> Ax' and Ay' 
% | Zx    0  Ax  0 || Ix |   | -Vsx | 
% | 0    Zy  0  Ay || Iy | = | -Vsy |
% | Ax' Ay'  0   0 || Vc |   | Isc  |
% Ay = node_y(*,2)=[upper branch (-), bott branch (+)]
% 10 March 2016
%
function [Ax Ay Plat]=NB_XY(Plat)
Nx=Plat.num(1);                 % node cell # in x dir
Ny=Plat.num(2);                 % node cell # in y dir
nx=Nx-1;                        % x-cell cell # in x dir
ny=Ny-1;                        % y-cell cell # in y dir

nnx=nx*Ny;  % total Ix cells
nny=ny*Nx;  % total Iy cells
nn=Nx*Ny;   % total p cells

Plat.BNX=zeros(nnx,3);
Plat.BNY=zeros(nny,3);

% (1) X-cell and node numbering relationship
idx=[];idy1=[];idy2=[];
off_node=0; off_brax=0;
Ax=zeros(nnx,nn);                   % bran-node matrix
for j=1:Ny
    ix=(1:nx)+(j-1)*nx;             % branch no.
    idx=[idx ix];
    iy1=(1:Nx-1)+(j-1)*Nx;          % starting node
    idy1=[idy1 iy1];
    iy2=(2:Nx)+(j-1)*Nx;            % ending node
    idy2=[idy2 iy2];    
end
tp1=idx'+off_brax;
tp2=[idy1; idy2]'+off_node;
Plat.X2C=[tp1 tp2];

Plat.BNX(idx,1)=idx;
Plat.BNX(idx,2)=idy1;
Plat.BNX(idx,3)=idy2;

for ik=1:nnx
    Ax(ik,idy1(ik))=-1;
    Ax(ik,idy2(ik))=+1;
end
Ax=Ax';                             % node-bran matrix    

% (2) Y-cell and node numbering relationship
idx=[];idy1=[];idy2=[];
off_node=0; off_bray=0;
Ay=zeros(nny,nn);                   % bran-node matrix
for j=1:Ny-1
    ix=(1:Nx)+(j-1)*Nx;             % branch no.
    idx=[idx ix];
    iy1=(1:Nx)+(j-1)*(Nx);          % starting node
    idy1=[idy1 iy1];
    iy2=(1:Nx)+(j)*(Nx);            % ending node
    idy2=[idy2 iy2];
end
tp1=idx'+off_brax;
tp2=[idy1; idy2]'+off_node;
Plat.Y2C=[tp1 tp2];

Plat.BNY(idx,1)=idx;
Plat.BNY(idx,2)=idy1;
Plat.BNY(idx,3)=idy2;

for ik=1:nny
    Ay(ik,idy1(ik))=-1;
    Ay(ik,idy2(ik))=+1;
end
Ay=Ay';                             % node-bran matrix    

% (3) removing KCL equation for last node (ref node) to avoid signular problem
[n m]=size(Ax);
Ax(n,:)=[];
[n m]=size(Ay);
Ay(n,:)=[];
Plat.BNX(nnx,3)=0;
Plat.BNY(nny,3)=0;

Ax=sparse(Ax);
Ay=sparse(Ay);
end