% SOL_M0    main program for solving J, Mx, My in a magnetic plate using
%           METHOD 0 (point-match)
%
% N, M      number of segements along x and y directions for the plate
% updated on June 8 2011
%
function IMP
w=0.01; dd=0.01; s=1;                             % 铁板的宽度和厚度
mu0=4e-7*pi;
f=50; mur=200; delt=5e+6;

M=40;                               % mesh no in y axis
N=40;                              % mesh no in x axis

TYPE_sol=1;                        % TYPE_sol = 0 (field) = 1 (impedance)

TYPE_mat=0;                        % TYPE_mat = 0 (conductive/magnetic)
if f==0             
    TYPE_mat=1;                    % TYPE_mat = 1 (magnetic)
elseif mur==1
    TYPE_mat=2;                    % TYPE_mat = 2 (conductive)
end

% (1)  source -------------------------------------------------------------
Is=[1;-1];
% Lxy=[ -0.05 -0.1;
%        0.05 -0.1];
Lxy=[ 0.5-0.05 -1;                % source location
      0.5+0.05 -1];
%--------------------------------------------------------------------------

% (2) geometrical info. of observations------------------------------------
N0=41;
p1=[-1 1];  p2=[1 1];
dx=(p2-p1)/N0;
for ik=1:N0+1
    P0(ik,1:2)=p1+dx*(ik-1);
end
%--------------------------------------------------------------------------
 
% (3) meshing -------------------------------------------------------------
u=-w/2+(0:N)*w/N;        du=u(2)-u(1);                     % x坐标数组
v=-dd/2+(0:M)*dd/M;      dv=v(2)-v(1);                     % y坐标数组
uu=0.5*(u(1:N)+u(2:N+1));
vv=0.5*(v(1:M)+v(2:M+1));
[X Y]=meshgrid(vv,uu);

% meshing for coef of V on two condcutors, G and G'
G0=zeros(2*N*M,2);
Gc=zeros(2,2);

G=ones(N*M,1);
G=[G G*0
   G*0 G];
Ga=G/(1i*f*mu0);
Gb=G'*(du*dv);
%--------------------------------------------------------------------------

for i=1:M
    idx=(1:N)+(i-1)*N;
    Sxy1(idx,1)=u(1:N);                  % source coordinates for coef. cal
    Sxy1(idx,3)=u(2:N+1);
    Sxy1(idx,2)=v(i);
    Sxy1(idx,4)=v(i+1);
    Oxy1(idx,1)=0.5*(u(1:N)+u(2:N+1));
    Oxy1(idx,2)=0.5*(v(i)+v(i+1));

    Sxy2(idx,1)=u(1:N)+s;                % source coordinates for coef. cal
    Sxy2(idx,3)=u(2:N+1)+s;
    Sxy2(idx,2)=v(i);
    Sxy2(idx,4)=v(i+1);
    Oxy2(idx,1)=0.5*(u(1:N)+u(2:N+1))+s;
    Oxy2(idx,2)=0.5*(v(i)+v(i+1));
end
Sxy=[Sxy1;Sxy2];                  % source coordinates for coef. cal
Oxy=[Oxy1;Oxy2];
    
ke=1i*f*delt*mu0;                  % ke =j*((2*pi)*f)*delt*mu0/(2*pi)
E=eye(2*N*M,2*N*M);
%--------------------------------------------------------------------------

% (4) source terms---------------------------------------------------------
if TYPE_sol==0                      % fuield solver
    [P1 P2 P3]=COEF_2D_SOUR_M0(Lxy,Is,Oxy);
    T1=-P1;         % -As
    T2=-P2;         % -Bsx
    T3=-P3;         % -Bsy
    T4=[0;0];        % total current in a plate = 0
else
    T1=zeros(2*N*M,1);
    T2=zeros(2*N*M,1);
    T3=zeros(2*N*M,1);
    T4=[1;-1];              % total current on the bars
end
%--------------------------------------------------------------------------

% (5) inductance term------------------------------------------------------
[Pa,Pb,Pc,Pdx,Pdy,Pe]=COEF_2D_CELL_M0(Sxy,Oxy,0);
% L=-Pe,  Qx=Pb,  Qy=-Pa
% Bcx=-Pb Pxx=pdx  Pxy=Pc
% Bcy=Pa  Pyx=Pc Pyy=pdy
km=0;
if mur~=1
    km=2*pi*mur/(mur-1);       % mur=200,km=mu0*mur/(mur-1)/(mu0/2*pi)
end
Z=E/ke-Pe;   Qx=Pb;          Qy=-Pa;
Bcx=-Pb;     Pxx=Pdx-E*km;    Pxy=Pc;
Bcy=Pa;      Pyx=Pc;         Pyy=Pdy-E*km; 
%--------------------------------------------------------------------------
 
% (6) build matrix equation------------------------------------------------
switch TYPE_mat
    case 2                          % conductivwe material
         G=[Z   Ga;
            Gb  Gc];
        T=[T1;T4];
        Q=G\T;
        V=Q(2*N*M+(1:2),1);
    case 0                          % conductive/mangetic
         G=[Z   Qx  Qy   Ga;
            Bcx Pxx Pxy  G0;
            Bcy Pyx Pyy  G0;
            Gb  G0' G0'  Gc];
        T=[T1;T2;T3;T4];
        
        Q=G\T;
        V=Q(6*N*M+(1:2),1);
    case 1                          % magnetic   
end

% (7) generate impedance --------------------------------------------------
V0=V(2)-V(1);
R=real(V0)
L=imag(V0)/(2*pi*f)
disp('OERSTED Result: R=5.8mho,L=15.45uH');

% (8) generate J/M density-- ----------------------------------------------
fid=1;
    switch TYPE_mat
        case 0
            for ik=1:M
                idx=N*(ik-1)+(1:N);
                idy=idx+2*N*M;
                idz=idy+2*N*M;
                Jc(1:N,ik)=Q(idx);
                Mx(1:N,ik)=Q(idy);
                My(1:N,ik)=Q(idz);
            end
            Jc0=Q(1:N*M);
            Mx0=Q((1:N*M)+2*N*M);
            My0=Q((1:N*M)+4*N*M);

            figure(fid);fid=fid+1;
            plot(uu,real(Jc(1:N,1)),'r',uu,imag(Jc(1:N,1)),'b',uu,...
                real(Jc(1:N,M)),'g',uu,imag(Jc(1:N,M)),'m');grid on;
            figure(fid);fid=fid+1;
            plot(uu,real(Mx(1:N,1)),'r',uu,imag(Mx(1:N,1)),'b',uu,...
                real(Mx(1:N,M)),'g',uu,imag(Mx(1:N,M)),'m'); grid on;
            figure(fid);fid=fid+1;
            plot(vv,real(My(1,1:M)),'r',vv,imag(My(1,1:M)),'b',vv,...
                real(My(N,1:M)),'g',vv,imag(My(N,1:M)),'m'); grid on;
            
            [X Y]=meshgrid(vv,uu);
            figure(fid);fid=fid+1;   meshc(X,Y,real(Jc));    grid on;
            title('real(Jc) (A/m2)');
            figure(fid);fid=fid+1;   meshc(X,Y,imag(Jc));    grid on ;
            title('imag(Jc) (A/m2)');
            figure(fid);fid=fid+1;   meshc(X,Y,real(Mx));    grid on ;
            title('real(Mx) (A/m)');
            figure(fid);fid=fid+1;   meshc(X,Y,imag(Mx));    grid on ;
            title('imag(Mx) (A/m)');        
%         [Bxc0 Byc0 Bxm0 Bym0]=BF0CAL(P0,P0c,Jc0,Mx0,My0);% field from plate
        case 2
            for ik=1:M
                idx=N*(ik-1)+(1:N);
                Jc(1:N,ik)=Q(idx);
            end
            Mx=0;My=0;
            figure(fid);fid=fid+1;
            plot(uu,real(Jc(1:N,1)),'r + -',uu,imag(Jc(1:N,1)),'g o -',uu,...
                real(Jc(1:N,M)),'b * -',uu,imag(Jc(1:N,M)),'k < -');grid on;
            figure(fid);fid=fid+1;    meshc(X,Y,real(Jc));    grid on;
            figure(fid);fid=fid+1;    meshc(X,Y,imag(Jc));    grid on ;
%             [Bxc0 Byc0 Bxm0 Bym0]=BF0CAL_NM(P0,P0c,Jc0);
        
%             figure(1);
%             subplot(3,1,3);
%             plot(uu,real(Jc(1:N,1)),'r',uu,imag(Jc(1:N,1)),'b');grid on;
%             xlabel('X(m)');ylabel('Jc (A)');      
        otherwise 
            disp('no such case');
    end
end        