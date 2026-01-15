% function [Tout Uout2] = main_Lightning_induced_Vol_Sr(pt_start, pt_end, a00, pt_hit, Imax, k0, tau1, tau2, n, alpha1, alpha2, ...
%                         i_sr_type, h_ch, t_ob, flag_type, erg, sigma_g, ground_type, dt_out,ve,Nc)
%%%%%%%%%%%%%%%%%%%%%%%%%%%  parameters
Ns_ch=h_ch*2;   %segments of channel
dt_E=8e-9;   %t_ob/dt_E should be integer
dt_H=2e-9;   %t_ob/dt_H should be integer
f_low=2e6;   % cut-of freq
f_high=2.5e7;  % cur_of freq
conv1=8;     % const in convolution
a00=Nc;       % number of conductors
%%%%%%%%%%%%%%%%%%% calculate E field above perfect ground
[Er_p Ez_p] = E_Cal(pt_start,pt_end,flag_type,pt_hit,h_ch, ...
i_sr_type,Imax,tau1,tau2,k0,n,alpha1,alpha2,t_ob,dt_E,Ns_ch,a00,ve);

%%%%%%%%%%%%%%%%%%% filter
[Er_p2]=LP_Filter0(Er_p,f_low,f_high,dt_E);
[Ez_p2]=LP_Filter0(Ez_p,f_low,f_high,dt_E);

%%%%%%%%%%%%%%%%%%% calculate E field above lossy ground
if ground_type==1 % perfect
    Er_out=Er_p2;

elseif ground_type==2 % lossy
    [H_p] =H_Cal(pt_start,pt_end,flag_type,pt_hit,h_ch, ...
i_sr_type,Imax,tau1,tau2,k0,n,alpha1,alpha2,t_ob,dt_H,Ns_ch,a00,ve);

    [H_p2]=LP_Filter0(H_p,f_low,f_high,dt_E);

    Er_lossy = E_Cal_lossy(Er_p2,H_p2,dt_E,dt_H,erg,sigma_g,conv1,a00);
    Er_out=LP_Filter0(Er_lossy,f_low,f_high,dt_E);

else
Er_out=0;
end
Ez_out=Ez_p2;

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% Voltage source calculate

[Uout]=U_Cal(Er_out,Ez_out,pt_start,pt_end,pt_hit,a00,dt_E,t_ob);

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% output at dt_out time interval
xx = dt_E:dt_E:t_ob; 
yy = Uout; 
xxi = dt_out:dt_out:t_ob; 
Uout2 = interp1(xx,yy,xxi,'spline');
Tout=dt_out:dt_out:t_ob;

figure(1);
plot((1:t_ob/dt_E).*dt_E,Er_out(:,1),(1:t_ob/dt_E)*dt_E,Er_p2(:,1),'-.')
grid on;
figure(2);
plot((1:t_ob/dt_E).*dt_E,Er_out(:,2),(1:t_ob/dt_E)*dt_E,Er_p2(:,2),'-.')
grid on;
if Nc>=3
figure(3);
plot((1:t_ob/dt_E).*dt_E,Er_out(:,3),(1:t_ob/dt_E)*dt_E,Er_p2(:,3),'-.');
grid on;
else
end

% end