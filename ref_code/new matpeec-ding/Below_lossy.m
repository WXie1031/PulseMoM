function [Er_below]= Below_lossy(Er_lossy0,d,erg,sigma_g,dt,Nt);

HR0=Er_lossy0;
ep0=8.85e-12;
u0=4*pi*1e-7;
Nt0=Nt;
vc=3e8;
Nd=9;    
w=[0.01 0.05 0.1 0.5 1e0 5e0 1e1 5e1 1e2 5e2 1e3 5e3 1e4 5e4 1e5 5e5 1e6 5e6 1e7];
con=1;
a11=length(w);
H_br2=zeros(1,1,a11);

for ii=1:a11
    kg(ii)=sqrt(j*w(ii)*u0*(sigma_g+j*w(ii)*ep0*erg));
    H_br2(ii)=exp(-kg(ii)*d);
end

% test1=zeros(a11,1);
% test1(1:a11)=H_in(1,1,1:a11);
[R0, L0, Rn, Ln, Zfit] = vecfit_kernel_Z_Ding(H_br2, w/2/pi, Nd);
% test11=zeros(a11,1);
% test11(1:a11)=Zfit(1,1,1:a11);
R0_1=R0-sum(Rn,3);
L0_1=L0;
R_1=zeros(1,Nd);
R_1(1:Nd)=Rn(1,1,1:Nd);
L_1=zeros(1,Nd);
L_1(1:Nd)=Rn(1,1,1:Nd);

[a00 Nt]=size(HR0);
t_ob=Nt*dt;
conv_2=10;
dt0=dt/conv_2;
Nt0=Nt;
Nt3=Nt;
dt0=dt/conv_2;

    x = dt:dt:t_ob;     
y = HR0(:,1:Nt); 
xi = dt0:dt0:t_ob; 
H_save2 = (interp1(x,y',xi,'spline'))';
H_all=H_save2;

% H_all=HR0;
Ntt=length(H_all);  
H_all_diff=zeros(a00,Ntt);
H_all_diff(:,1)=H_all(:,1)/dt0;
H_all_diff(:,2:Ntt)=(diff(H_all')/dt0)';

ee0=R0.*H_all;
eeL=L0.*H_all_diff;


t00=Ntt;
ee=zeros(Ntt,Nd);

for ii=1:t00
    for jj=1:Nd
        ee(ii,jj)=-Rn(1,1,jj)^2/Ln(1,1,jj)*exp(-Rn(1,1,jj)/Ln(1,1,jj)*(ii)*dt0);
    end
end
for jj=1:Nd
    ee_conv(:,:,jj)=dt0*conv2(H_all,ee(1:Ntt,jj)');
%     ee_one(:,jj)=dt0*conv(ones(Ntt,1),ee(1:Ntt,jj));
end

ee_conv_sum=sum(ee_conv,3);
ee_all=ee0(:,1:conv_2:Ntt)+eeL(:,1:conv_2:Ntt)+ee_conv_sum(:,1:conv_2:Ntt);
Er_below=ee_all;


% 
% ep0=8.85e-12;
% u0=4*pi*1e-7;
% conv2=1;
% dt0=dt/conv2;
% % ik=1;
% Nt0=Nt;
% Nd=9;
% R0=0;
% L0=0;
% Rn=0;
% Ln=0;
% Zfit=0;
% t_ob=Nt*dt;
% %     H_all=H_p2*u0;
% % E_all_in=H_p2*u0;
% %     H_all=Er_lossy;
% % E_all_in=Er_lossy;
% % Ntt=length(H_all);
% 
%     w=[1e0 5e0 1e1 5e1 1e2 5e2 1e3 5e3 1e4 5e4 1e5 5e5 1e6 5e6 1e7 5e7];
%     con=1;
%     
%     a11=length(w);
% 
% H_br2=zeros(1,1,a11);
% test1=0;
% for ii=1:a11
%     kg(ii)=sqrt(j*w(ii)*u0*(sigma_g+j*w(ii)*ep0*erg));
%     H_br2(ii)=exp(-kg(ii)*d);
% end
% 
% 
% [R0, L0, Rn, Ln, Zfit] = vecfit_kernel_Z_Ding(H_br2, w/2/pi, Nd);
% 
% R0_1=R0-sum(Rn,3);
% L0_1=L0;
% R_1=zeros(1,Nd);
% R_1(1:Nd)=Rn(1,1,1:Nd);
% L_1=zeros(1,Nd);
% L_1(1:Nd)=Rn(1,1,1:Nd);
% 
% [a00 Nt]=size(Er_lossy0);
% 
% for ik=1:a00;
% % H_all=Er_lossy0(ik,1:Nt);
% E_all_in=Er_lossy0(ik,1:Nt);
% 
% conv2=16;
% dt0=dt/conv2;
%     Nt0=Nt;
%     Nt3=Nt;
%     dt0=dt/conv2;
%     
%     x = dt:dt:t_ob; 
%     
%     y=0;
% y = Er_lossy0(ik,1:Nt); 
% xi = dt0:dt0:t_ob; 
% H_save2 = interp1(x,y,xi,'spline');
% H_all=H_save2;
% Ntt=length(H_all);
% 
% H_all_diff=0;
% for i=1:(Ntt-1)
%     H_all_diff(i+1)=(H_all(i+1)-H_all(i))/dt0;
% end
% H_all_diff(1)=H_all(1)/dt0;
% 
% t00=Ntt;
% 
% e1=zeros(1,t00);
% e2=zeros(1,t00);
% e3=zeros(1,t00);
% e4=zeros(1,t00);
% e5=zeros(1,t00);
% e6=zeros(1,t00);
% 
% ee=zeros(Ntt,Nd);
% 
% for ii=1:t00
%     for jj=1:Nd
%         ee(ii,jj)=-Rn(1,1,jj)^2/Ln(1,1,jj)*exp(-Rn(1,1,jj)/Ln(1,1,jj)*ii*dt0);
%     end
% end
% ee_conv=zeros(2*Ntt-1,jj);
% ee_one=zeros(2*Ntt-1,jj);
% for jj=1:Nd
%     ee_conv(:,jj)=dt0*conv(H_all(:),real(ee(1:Ntt,jj)));
%     ee_one(:,jj)=dt0*conv(ones(Ntt,1),real(ee(1:Ntt,jj)));
% end
% ee_conv_sum=sum(ee_conv,2);
% ee_one_sum=sum(ee_one,2);
% 
% 
% 
% ee0=(R0)*H_all;
% eeL=L0*H_all_diff;
% 
% % ee_all=ee0(1:conv2:Ntt)'+eeL(1:Nt0)+ee_conv_sum(1:conv2:Ntt)';
% ee_all=ee0(1:conv2:Ntt)+eeL(1:Nt0)+ee_conv_sum(1:conv2:Ntt)';
% 
% 
% Er_below(ik,1:Nt)=ee_all(1:con:Nt);
% end
% end