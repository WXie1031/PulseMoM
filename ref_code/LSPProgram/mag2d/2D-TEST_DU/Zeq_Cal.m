% SOL_M0    main program for solving J, Mx, My in a magnetic plate using
%           METHOD 0 (point-match)
%
% N, M      number of segements along x and y directions for the plate
% updated on June 8 2011
%
function Zeq_Cal

w=40e-3; dd=4e-3; s=2;                             % 铁板的宽度和厚度
mu0=4e-7*pi;


f=1000; mur=20; delt=4.8828e+06;

M=80;
N=8;

u=-w/2+(0:N)*w/N;        du=u(2)-u(1);                     % x坐标数组
v=-dd/2+(0:M)*dd/M;      dv=v(2)-v(1);                     % y坐标数组
for i=1:M
    idx=(1:N)+(i-1)*N;
    Sxy1(idx,1)=u(1:N);                  % source coordinates for coef. cal
    Sxy1(idx,3)=u(2:N+1);
    Sxy1(idx,2)=v(i);
    Sxy1(idx,4)=v(i+1);
    Oxy1(idx,1)=0.5*(u(1:N)+u(2:N+1));
    Oxy1(idx,2)=0.5*(v(i)+v(i+1));

    Sxy2(idx,1)=u(1:N)+s;                  % source coordinates for coef. cal
    Sxy2(idx,3)=u(2:N+1)+s;
    Sxy2(idx,2)=v(i);
    Sxy2(idx,4)=v(i+1);
    Oxy2(idx,1)=0.5*(u(1:N)+u(2:N+1))+s;
    Oxy2(idx,2)=0.5*(v(i)+v(i+1));
end
Sxy=[Sxy1;Sxy2];                  % source coordinates for coef. cal
Oxy=[Oxy1;Oxy2];
    
ke=j*f*delt*mu0;                  % ke =j*((2*pi)*f)*delt*mu0/(2*pi)
E=eye(2*N*M,2*N*M);

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

n0=1:2*N*M;
n1=1:N*M;
n2=(N*M+1):2*N*M;

Z11=Z(n1,n1);
Z12=Z(n1,n2);
Z21=Z(n2,n1);
Z22=Z(n2,n2);
ZZ=Z11-Z12-Z21+Z22;

if mur==1                       % eq. impedance of al bars
        YY=inv(ZZ);
        Yeq=sum(sum(YY));
        Zeq=(1i*2*pi*f*2e-7)./(du*dv*Yeq);
else                            % eq. impedance of steel bars
        Q1=[Qx(n1,n0) Qy(n1,n0)];
        Q2=[Qx(n2,n0) Qy(n2,n0)];
        B1=[Bcx(n0,n1); Bcy(n0,n1)];
        B2=[Bcx(n0,n2); Bcy(n0,n2)];
        P=[Pxx,Pxy;
           Pyx,Pyy];
        P0=inv(P); 
        
        Z11=Z11-Q1*P0*B1;
        Z12=Z12-Q1*P0*B2;
        Z21=Z21-Q2*P0*B1;
        Z22=Z22-Q2*P0*B2;
        ZZ=Z11-Z12-Z21+Z22;

        YY=inv(ZZ);
        Yeq=sum(sum(YY));
        Zeq=(1i*2*pi*f*2e-7)./(du*dv*Yeq);
end

R=real(Zeq)*1e3/2
L=imag(Zeq)/(2*pi*f)*1e6/2
sdep = sqrt(2./(delt*2*pi*f*mur))
% disp('OERSTED Result: R=5.8mho,L=15.45uH');

end        