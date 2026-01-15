% SOL_M0    main program for solving J, Mx, My in a magnetic plate using
%           METHOD 0 (point-match)
%
% N, M      number of segements along x and y directions for the plate
% updated on June 8 2011
%
function SOL_M0
w=0.4; dd=0.002;                              % 铁板的宽度和厚度
mu0=4e-7*pi;
f=50; mur=200; delt=5e+6;
TYPE=0;                            % TYPE = 0 (conductive/magnetic)
if f==0             
    TYPE=1;                        % TYPE = 1 (magnetic)
elseif mur==1
    TYPE=2;                        % TYPE = 2 (conductive)
end

Is=[1;-1];
Lxy=[ -0.1 -0.1;
       0.1 -0.1];
% Lxy=[ 0.5-0.05 -1;                % source location
%       0.5+0.05 -1];

  % (0c) geometrical info. of observations
N0=41;
p1=[-1 1];  p2=[1 1];
dx=(p2-p1)/N0;
for ik=1:N0+1
    P0(ik,1:2)=p1+dx*(ik-1);
end
  
% u=-w/2+(0:N)*w/N;                             % x坐标数组
% v=-dd/2+(0:M)*dd/M;                           % y坐标数组
N1=101;
v=dd*([0 0.01 0.1 0.2 0.3 0.5 0.7 0.8 0.9 0.99 1]-0.5); M=length(v)-1;
% v=dd*([0 0.005 0.01 0.05 0.1 0.15 0.2 0.25 0.3 0.4 0.5 0.6 0.7 0.75 0.8 0.85 0.9 0.95 0.99 0.995 1]-0.5); M=length(v)-1;
u1=dd*[0 0.1 0.25 0.5 0.75 1.0 1.5 2.0 3.0 4.0 6.0 8.0]; N0=length(u1);
u2=-u1; u2=sort(u2);
u0=-w/2+u1(N0)+(1:N1-1)*(w-2*u1(N0))/N1;
u=[-w/2+u1 u0 w/2+u2]; N=length(u)-1;
uu=0.5*(u(1:N)+u(2:N+1));
vv=0.5*(v(1:M)+v(2:M+1));
[X Y]=meshgrid(vv,uu);

for i=1:M
    idx=(1:N)+(i-1)*N;
    Sxy(idx,1)=u(1:N);                  % source coordinates for coef. cal
    Sxy(idx,3)=u(2:N+1);
    Sxy(idx,2)=v(i);
    Sxy(idx,4)=v(i+1);
    Oxy(idx,1)=0.5*(u(1:N)+u(2:N+1));
    Oxy(idx,2)=0.5*(v(i)+v(i+1));
    P0c(idx,1)=Sxy(idx,1);             % source coordiantes for B field
    P0c(idx,2)=Sxy(idx,3);
    P0c(idx,3)=Sxy(idx,2);
    P0c(idx,4)=Sxy(idx,4);
end

% (1) source terms
[P1 P2 P3]=COEF_2D_SOUR_M0(Lxy,Is,Oxy);
T1=-P1;         % -As
T2=-P2;         % -Bsx
T3=-P3;         % -Bsy

figure(1);
subplot(3,1,1);
PP0=[u(1) u(N) v(1) v(M)];
plot(PP0(1:2),[1 1]*PP0(3),PP0(1:2),[1 1]*PP0(4),PP0(3:4),[1 1]*PP0(1),...
    PP0(3:4),[1 1]*PP0(2));grid on;hold on;
plot(Lxy(1,1),Lxy(1,2),'^r',Lxy(2,1),Lxy(2,2),'vr');hold off;
xlabel('X(m)');ylabel('Y(m)');
subplot(3,1,2);
PP1=P2(1:N);
PP2=P3(1:N);
plot(uu,real(PP1),uu,imag(PP1),uu,real(PP2),uu,imag(PP2));grid on;
xlabel('X(m)');ylabel('Bs (mG)');

ke=j*f*delt*mu0;                  % ke =j*((2*pi)*f)*delt*mu0/(2*pi)
E=eye(N*M,N*M);

[Pa,Pb,Pc,Pdx,Pdy,Pe]=COEF_2D_CELL_M0(Sxy,Oxy,0);
% L=-Pe,  Qx=Pb,  Qy=-Pa
% Bcx=-Pb Pxx=pdx  Pxy=Pc
% Bcy=Pa  Pyx=Pc Pyy=pdy
km=0;
if TYPE~=2
    km=2*pi*mur/(mur-1);       % mur=200,km=mu0*mur/(mur-1)/(mu0/2*pi)
end
L=E/ke-Pe;   Qx=Pb;          Qy=-Pa;
Bcx=-Pb;     Pxx=Pdx-E*km;    Pxy=Pc;
Bcy=Pa;      Pyx=Pc;         Pyy=Pdy-E*km; 
fid=2;

switch TYPE
    case 0

        G=[L,  Qx, Qy;
            Bcx,Pxx,Pxy;
            Bcy,Pyx,Pyy];
        T=[T1;T2;T3];
        Q=G\T;

        for ik=1:M
            idx=N*(ik-1)+(1:N);
            idy=idx+N*M;
            idz=idy+N*M;
            Jc(1:N,ik)=Q(idx);
            Mx(1:N,ik)=Q(idy);
            My(1:N,ik)=Q(idz);
        end
        Jc0=Q(1:N*M);
        Mx0=Q((1:N*M)+N*M);
        My0=Q((1:N*M)+2*N*M);

        figure(fid);fid=fid+1;
        plot(uu,real(Jc(1:N,1)),'r',uu,imag(Jc(1:N,1)),'b',uu,...
            real(Jc(1:N,M)),'g',uu,imag(Jc(1:N,M)),'m');grid on;
        figure(fid);fid=fid+1;
        plot(uu,real(Mx(1:N,1)),'r',uu,imag(Mx(1:N,1)),'b',uu,...
            real(Mx(1:N,M)),'g',uu,imag(Mx(1:N,M)),'m'); grid on;
        figure(fid);fid=fid+1;
        plot(vv,real(My(1,1:M)),'r',vv,imag(My(1,1:M)),'b');grid on;

        [X Y]=meshgrid(vv,uu);
        figure(fid);fid=fid+1;   meshc(X,Y,real(Jc));    grid on;
        title('real(Jc) (A/m2)');
        figure(fid);fid=fid+1;   meshc(X,Y,imag(Jc));    grid on ;
        title('imag(Jc) (A/m2)');
        figure(fid);fid=fid+1;   meshc(X,Y,real(Mx));    grid on ;
        title('real(Mx) (A/m)');
        figure(fid);fid=fid+1;   meshc(X,Y,imag(Mx));    grid on ;
        title('imag(Mx) (A/m)');
        
        [Bxc0 Byc0 Bxm0 Bym0]=BF0CAL(P0,P0c,Jc0,Mx0,My0);% field from plate
    case 1
        G=[ Pxx,Pxy;
            Pyx,Pyy];
        T=[T2;T3];
        Q=G\T;

        for ik=1:M
            idx=N*(ik-1)+(1:N);
            idy=idx+N*M;
            Mx(1:N,ik)=Q(idx);
            My(1:N,ik)=Q(idy);
        end
        Jc0=0;
        Mx0=Q(1:N*M);
        My0=Q((1:N*M)+N*M);

        figure(fid);fid=fid+1;
        plot(uu,real(Mx(1:N,1)),'r',uu,imag(Mx(1:N,1)),'b',...
            uu,real(Mx(1:N,M)),'g',uu,imag(Mx(1:N,M)),'m'); grid on;
        figure(fid);fid=fid+1;
        plot(vv,real(My(1,1:M)),'r',vv,imag(My(1,1:M)),'b');grid on;

        figure(1);
        subplot(3,1,3);
        plot(uu,real(Mx(1:N,1)),'r',uu,imag(Mx(1:N,1)),'b');grid on;
        xlabel('X(m)');ylabel('Mx (A)');

        [X Y]=meshgrid(vv,uu);
        figure(fid);fid=fid+1;   meshc(X,Y,real(Mx));    grid on ;
        figure(fid);fid=fid+1;   meshc(X,Y,imag(Mx));    grid on ;

        [Bxc0 Byc0 Bxm0 Bym0]=BF0CAL(P0,P0c,Jc0,Mx0,My0);
        Bxc0=0;Byc0=0;
    case 2
        Jc0=L\T1;
        for ik=1:M
            idx=N*(ik-1)+(1:N);
            Jc(1:N,ik)=Jc0(idx);
        end
        Mx=0;My=0;
        figure(fid);fid=fid+1;
        plot(uu,real(Jc(1:N,1)),'r + -',uu,imag(Jc(1:N,1)),'g o -',uu,...
            real(Jc(1:N,M)),'b * -',uu,imag(Jc(1:N,M)),'k < -');grid on;
        figure(fid);fid=fid+1;    meshc(X,Y,real(Jc));    grid on;
        figure(fid);fid=fid+1;    meshc(X,Y,imag(Jc));    grid on ;
        [Bxc0 Byc0 Bxm0 Bym0]=BF0CAL_NM(P0,P0c,Jc0);
        
        figure(1);
        subplot(3,1,3);
        plot(uu,real(Jc(1:N,1)),'r',uu,imag(Jc(1:N,1)),'b');grid on;
        xlabel('X(m)');ylabel('Jc (A)');      
    otherwise 
        disp('no such case');
end

% (4) generating b-field plots`
[Bxs Bys]=BFSCAL(P0,Lxy,Is);                % firld from the source
Bx=2*(Bxs+Bxc0+Bxm0);     % mG
By=2*(Bys+Byc0+Bym0);     % mG
Brms=sqrt(conj(Bx).*Bx+conj(By).*By);       % total field
Bx0=2*Bxs;
By0=2*Bys;
Brms0=sqrt(conj(Bx0).*Bx0+conj(By0).*By0);  % total field from the plate
dx=P0(:,1)-P0(1,1);
dy=P0(:,2)-P0(1,2);
D=sqrt(dx.*dx+dy.*dy);
figure(fid);fid=fid+1;
plot(D,abs(Bx),'r',D,abs(By),'b',D,abs(Brms),'g',D,abs(Bx0),'r-.',...
    D,abs(By0),'b-.',D,abs(Brms0),'g-.'); grid;
end        