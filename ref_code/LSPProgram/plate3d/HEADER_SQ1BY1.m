% header file which contains all basic info.for 3D transient analysis
%  (1) source
%  (2) plate (non-ferromangetic)
%  (3) plate connectionn (NA)
%  (4) infinitve plate (NA)
%  (5) meshing size
%  (6) others

function [Freq Plat Sour Fied]=HEADER_SQ1BY1(f)

% (0) initilization & material info.
if f==0                                     % dummy frequency value
    Freq=[1 10 50 100 200 500 1e3 2e3 5e3 1e4 2e4 5e4 1e5 2e5 5e5 1e6]; 
%     Freq=[1 1e5  1e6]; 
elseif f==-1
    Freq=-[1 10 50 100 200 500 1e3 2e3 5e3 1e4 2e4 5e4 1e5 2e5 5e5 1e6];     
else
    Freq=f;
end
NF=length(Freq);
% *************************************************************************

% (2) Plate structure
mu0=4*pi*1e-7;
Plat.name=1;
Plat.sigm=3.45e7;
Plat.alph=-(1+1i)*sqrt(pi*f*mu0*Plat.sigm);
Plat.NG=7;                                  % gaussian integratino order
Plat.cent=[0 0 0];                          % x0 y0 z0  
Plat.size=[1 1 0.002];                  % width depth height
Plat.num=[10 10];                           % # of node cell in x/y dir
% *************************************************************************

% (3) Source info
Sour.name=1;
Sour.xcur=[1;-1];                           % Isj (j=1...)
p1=[-0.1 -0.1 -0.2];
p2=[ 0.1 -0.1 -0.2];
p3=[-0.1  0.1 -0.2];
p4=[ 0.1  0.1 -0.2];
Sour.xcen=[p1   p2;  % source 2: start/end (x,y,z)
           p3   p4];
Sour.ycur=[-1;1];                           % Isj (j=1...)
Sour.ycen=[p1   p3;  % source 1: start/end (x,y,z)
           p2   p4]; % source 2: start/end (x,y,z)
Sour.xnum=length(Sour.xcur);
Sour.ynum=length(Sour.ycur);
NM=Plat.num(1)*Plat.num(2);
Sour.BNX=[1 1+NM 2+NM;
          2 3+NM 4+NM];
Sour.BNY=[1 1+NM 3+NM;
          2 2+NM 4+NM];

% *************************************************************************

% (4) Field line
Fied.num=6;
p1=[0   0 0.1];                     % [x1 y1 z1]
p2=[0.5 0 0.1];                     % [x2 y2 z2]
dir=[1];                            % dir
Fied.pts=[p1 p2 dir];               % [x1 y1 z1 x2 y2 z2 dir]
x=linspace(Fied.pts(1),Fied.pts(4),Fied.num);
y=linspace(Fied.pts(2),Fied.pts(5),Fied.num);
z=linspace(Fied.pts(3),Fied.pts(6),Fied.num);
d=sqrt((x-x(1)).*(x-x(1))+(y-y(1)).*(y-y(1))+(z-z(1)).*(z-z(1)));
Fied.line=[x; y; z; d]';                    % [x y z d]
% *************************************************************************


% *************************************************************************