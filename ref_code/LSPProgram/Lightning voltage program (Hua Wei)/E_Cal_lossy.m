function [E_lossy] = E_Cal_lossy(E_all_r3,H_save,dt,dt2,erg,sigma_g,con2,a00);
for ik=1:a00
Nt0=length(H_save(:,ik));
Nt3=length(E_all_r3(:,ik)); 
% dt1=Nt3/Nt0;
t_ob=dt2*Nt0;

w=[0.01 0.05 0.1 0.5 1e0 5e0 1e1 5e1 1e2 5e2 1e3 5e3 1e4 5e4 1e5 5e5 1e6 5e6 1e7 5e7];
con=Nt0/Nt3;
% con2=8;
 Nd=9;
dt0=dt2/con2;
%  con=dt2/dt0;
x = dt2:dt2:t_ob; 
y = H_save(:,ik); 
xi = dt0:dt0:t_ob; 
H_save2 = interp1(x,y,xi,'spline');
dt=dt0;

H_all=H_save2;
E_all_in=E_all_r3;
Nt=length(H_all);
Nt_E=length(E_all_r3);
% dt=1e-9;
u0=4*pi*1e-7;
c=3e8;
e0=1/c^2/u0;
% sigma_g=0.001;
% erg=10;
% w=0;
w2=0;
% w0=0;
% Nd=9;
% tw0=1e8/1e3;

w0=[0.1 0.5 1e0 5e0 1e1 1e2 1e3 2e3 5e3 1e4 2e4 5e4 1e5 5e5 1e6 5e6 1e7 5e7];
tw0=length(w0);

% w=[0.01 0.05 0.1 0.5 1e0 5e0 1e1 1e2 1e3 2e3 5e3 1e4 2e4 5e4 1e5 5e5 1e6 5e6 1e7 5e7];
a00=length(w);
H_in=zeros(1,1,a00);
test1=0;
for ii=1:a00
    H_in(ii)=c*u0/sqrt(erg+sigma_g/(j*w(ii)*e0));
    test1(ii)=c*u0/sqrt(erg+sigma_g/(j*w(ii)*e0));
end
test0=0;
for ii=1:tw0
    H_in0(ii)=c*u0/sqrt(erg+sigma_g/(j*w0(ii)*e0));
    test0(ii)=(c*u0/sqrt(erg+sigma_g/(j*w0(ii)*e0)));
end

[R0, L0, Rn, Ln, Zfit] = vecfit_kernel_Z(H_in, w/2/pi, Nd);
test2=0;
test2(1:a00)=(Zfit(1,1,1:a00));


R0_1=R0-sum(Rn,3);
L0_1=L0;
R_1=zeros(1,Nd);
R_1(1:Nd)=Rn(1,1,1:Nd);
L_1=zeros(1,Nd);
L_1(1:Nd)=Rn(1,1,1:Nd);
w2=w0;
% tw0=length(w2);
Z_0=0;
Z_1=0;
for ii=1:tw0
    for jj=1:Nd
        Z_1(ii,jj)=(j*w2(ii)*Ln(1,1,jj)*Rn(1,1,jj)/(Rn(1,1,jj)+j*w2(ii)*Ln(1,1,jj)));
    end
end
for ii=1:tw0
    for jj=1:1
        Z_0(ii,jj)=R0-sum(Rn,3)+j*w2(ii)*L0;
    end
end

Zfit_1=Z_0+sum(Z_1,2);





H_all_diff=0;
for i=1:(Nt0-1)
    H_all_diff(i+1)=(H_all(i+1)-H_all(i))/dt;
end
H_all_diff(1)=H_all(1)/dt;






t00=Nt;

e1=zeros(1,t00);
e2=zeros(1,t00);
e3=zeros(1,t00);
e4=zeros(1,t00);
e5=zeros(1,t00);
e6=zeros(1,t00);

ee=zeros(Nt,Nd);

for ii=1:t00
    for jj=1:Nd
        ee(ii,jj)=-Rn(1,1,jj)^2/Ln(1,1,jj)*exp(-Rn(1,1,jj)/Ln(1,1,jj)*ii*dt);
    end
end
ee_conv=zeros(2*Nt-1,jj);
ee_one=zeros(2*Nt-1,jj);
for jj=1:Nd
    ee_conv(:,jj)=dt*conv(H_all,ee(1:Nt,jj));
    ee_one(:,jj)=dt*conv(ones(Nt,1),ee(1:Nt,jj));
end
ee_conv_sum=sum(ee_conv,2);
ee_one_sum=sum(ee_one,2);



ee0=(R0)*H_all;
eeL=L0*H_all_diff;

ee_all=ee0(1:con2:Nt)+eeL(1:Nt0)+ee_conv_sum(1:con2:Nt)';


E_lossy(1:Nt_E,ik)=E_all_in(1:Nt_E,ik)-ee_all(1:con:Nt0)';
% figure(2);
% % N0=1;con
% plot((1:Nt_E).*dt,E_lossy(1:Nt_E,ik),(1:Nt_E).*dt,E_all_r3(1:Nt_E),'-.')
% R0-sum(Rn,3)
% figure(2);
% plot(ee_all)
% plot(sum(Rn,3)*ones(Nt,1)+ee_one_sum(1:Nt))
% plot(Rn(1,1,1)*ones(Nt,1)+ee_one(1:Nt,1))
% plot((1:20:3000)*dt,E_lossy(8:20:(3000+7)),(1:20:3000)*dt,E_all_r(8:20:(3000+7)),'-.')
grid




end
% figure(2);
% N0=1;con
% plot((1:Nt_E).*dt,E_lossy(1:Nt_E,ik),(1:Nt_E).*dt,E_all_r3(1:Nt_E),'-.')
end
% ratio=E_save./H_save;
 






