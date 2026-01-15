


ep0=8.85e-12;
u0=4*pi*1e-7;
Nt0=Nt;
vc=3e8;
Nd=9; 
    w=[0.01 0.05 0.1 0.5 1e0 5e0 1e1 5e1 1e2 5e2 1e3 5e3 1e4 5e4 1e5 5e5 1e6 5e6 1e7];
    con=1;
    
    a11=length(w);
H_in=zeros(1,1,a11);

for ii=1:a11
    H_in(ii)=vc*u0/sqrt(erg+sigma_g/(j*w(ii)*ep0));
end

test1=zeros(a11,1);

test1(1:a11)=H_in(1,1,1:a11);

[R0, L0, Rn, Ln, Zfit] = vecfit_kernel_Z_Ding(H_in, w/2/pi, Nd);

test11=zeros(a11,1);
test11(1:a11)=Zfit(1,1,1:a11);

R0_1=R0-sum(Rn,3);
L0_1=L0;
R_1=zeros(1,Nd);
R_1(1:Nd)=Rn(1,1,1:Nd);
L_1=zeros(1,Nd);
L_1(1:Nd)=Rn(1,1,1:Nd);

E_all_in=0;
[a00 Nt]=size(HR0);
for ik=1:a00

    
E_all_in(1,1:Nt)=ER(ik,1:Nt);
t_ob=Nt*dt;
conv2=1;
dt0=dt/conv2;
    Nt0=Nt;
    Nt3=Nt;
    dt0=dt/conv2;
    
    x = dt:dt:t_ob; 
    
    y=0;
y = HR0(ik,1:Nt); 
xi = dt0:dt0:t_ob; 
H_save2 = interp1(x,y,xi,'spline');
H_all=H_save2;


Ntt=length(H_all);  
    
    H_all_diff=0;
for i=1:(Ntt-1)
    H_all_diff(i+1)=(H_all(i+1)-H_all(i))/dt0;
end
H_all_diff(1)=H_all(1)/dt0;

t00=Ntt;

ee=zeros(Ntt,Nd);

for ii=1:t00
    for jj=1:Nd
        ee(ii,jj)=-Rn(1,1,jj)^2/Ln(1,1,jj)*exp(-Rn(1,1,jj)/Ln(1,1,jj)*(ii)*dt0);
    end
end
ee_conv=zeros(2*Ntt-1,jj);
ee_one=zeros(2*Ntt-1,jj);
for jj=1:Nd
    ee_conv(:,jj)=dt0*conv(H_all,ee(1:Ntt,jj));
    ee_one(:,jj)=dt0*conv(ones(Ntt,1),ee(1:Ntt,jj));
end
ee_conv_sum=sum(ee_conv,2);
ee_one_sum=sum(ee_one,2);



ee0=(R0)*H_all;
eeL=L0*H_all_diff;

ee_all=ee0(1:conv2:Ntt)+eeL(1:conv2:Ntt)+ee_conv_sum(1:conv2:Ntt)';


Er_lossy(ik,1:Nt)=E_all_in(1:Nt)+ee_all(1:con:Nt);
end
