function [Er_p Ez_p] = E_Cal(pt_start,pt_end,flag_type,pt_hit,h_ch, ...
i_sr_type,Imax,tau1,tau2,k0,n,alpha1,alpha2,t_ob,dt,Ns_ch,a00,ve);
% 
% Ns_ch=h_ch;
% dt=4e-9;
% dt2=1e-9;
Nt=round(t_ob/dt);
% Nt2=(dt/dt2)*Nt;
[i_sr] = i_sr_select(Imax,tau1,tau2,k0,n,alpha1,alpha2,i_sr_type,t_ob,dt);
% [i_sr_H] = i_sr_select(Imax,tau1,tau2,k0,n,alpha1,alpha2,i_sr_type,t_ob,dt2);

figure(1);
plot(1:Nt,i_sr);

t_sr = dt*(1:(Nt));

ep0 = 8.85*1e-12;
% vcof=ve/vc;
if flag_type == 1  %% TL model
    vc = 3e8;
    vcof = ve/vc;
elseif flag_type == 2  %% MTLL
    vc = 3e8;
    vcof = ve/vc;
    H = 7e3;
else %% MTLE
    vc = 3e8;
    vcof = ve/vc;
    lamda = 2e3;  % constant in MTLE -- decays exponentially with the height
end

Nc = size(pt_start,1);
% Nt = length(t_sr);

x_hit = pt_hit(:,1);
y_hit = pt_hit(:,2);

dz_ch = h_ch/Ns_ch;
z_ch = ((1:Ns_ch)-0.5)'*dz_ch; % mid point of the channel segment
z_ch_img = -z_ch; % mid point of the channel segment


dt = t_sr(2)-t_sr(1);
i_sr_int = zeros(Nt,1);
i_sr_div = zeros(Nt,1);

if size(i_sr,1)==1
    i_sr = i_sr';
end
i_sr_int(1:Nt) = cumsum(i_sr)*dt;
i_sr_div(2:Nt) = diff(i_sr)/dt;

% distance between midpoint and channel segment in x-y plane
Rx = (pt_start(:,1)/2+pt_end(:,1)/2-x_hit);
Ry = (pt_start(:,2)/2+pt_end(:,2)/2-y_hit);
Rxy = sqrt( Rx.^2 + Ry.^2 ) ;
 
Uout = zeros(Nt,Nc);
Er_T = zeros(Nt,Nc);
Ez_T = zeros(Nt,Nc);
for ik=1:a00
    %ik
    x1 = pt_start(ik,1);
    y1 = pt_start(ik,2);
    z1 = pt_start(ik,3);
    
    x2 = pt_end(ik,1);
    y2 = pt_end(ik,2);
    z2 = pt_end(ik,3);
    
    % 1. calculate the E generate in the air
    dEz1_air = zeros(Nt,Ns_ch);
    dEz2_air = zeros(Nt,Ns_ch);
    dEz3_air = zeros(Nt,Ns_ch);
    dEr1_air = zeros(Nt,Ns_ch);
    dEr2_air = zeros(Nt,Ns_ch);
    dEr3_air = zeros(Nt,Ns_ch);
%     dEz_air = zeros(Nt,Ns_ch);
%     dEr_air = zeros(Nt,Ns_ch);

    Rxyz = zeros(1,Ns_ch);
    Rz = zeros(1,Ns_ch);

    for ig=1:Ns_ch

        Rxyz(ig) = sqrt( Rxy(ik)^2+(z1/2+z2/2 - z_ch(ig))^2 );
        
        n_td_tmp = floor( (t_sr - (z_ch(ig)/vc/vcof + Rxyz(ig)/vc) )/dt );
        id_t = n_td_tmp>0;        
    
        % propagate part
        Rz(ig) = (z1/2+z2/2 - z_ch(ig));
        
        if flag_type == 1  %% TL model
            cof_isr = 1/(4*pi*ep0);
        elseif flag_type == 2  %% MTLL
            cof_isr = 1/(4*pi*ep0) * (1 - z_ch(ig)/H);
            
        else %% MTLE
            cof_isr = 1/(4*pi*ep0) * exp(-z_ch(ig)/lamda);
        end
        
        dEz_1_cof = cof_isr * (2*Rz(ig)^2-Rxy(ik)^2)/(Rxyz(ig)^5);
        dEz_2_cof = cof_isr * (2*Rz(ig)^2-Rxy(ik)^2)/(Rxyz(ig)^4)/vc;
        dEz_3_cof = cof_isr * (Rxy(ik)^2)/(Rxyz(ig)^3)/(vc^2);
        
        dEr_1_cof = cof_isr * (3*Rz(ig)*Rxy(ik))/(Rxyz(ig)^5);
        dEr_2_cof = cof_isr * (3*Rz(ig)*Rxy(ik))/(Rxyz(ig)^4)/vc;
        dEr_3_cof = cof_isr * (Rz(ig)*Rxy(ik))/(Rxyz(ig)^3)/(vc^2); 
        
        dEz1_air(id_t,ig) = dEz_1_cof * i_sr_int(n_td_tmp(id_t));
        dEz2_air(id_t,ig) = dEz_2_cof * i_sr(n_td_tmp(id_t));
        dEz3_air(id_t,ig) = dEz_3_cof * i_sr_div(n_td_tmp(id_t));
        
        dEr1_air(id_t,ig) = dEr_1_cof * i_sr_int(n_td_tmp(id_t));
        dEr2_air(id_t,ig) = dEr_2_cof * i_sr(n_td_tmp(id_t));
        dEr3_air(id_t,ig) = dEr_3_cof * i_sr_div(n_td_tmp(id_t));
        
     end
    
    Ez_air = sum( dEz1_air + dEz2_air - dEz3_air, 2 );
    Er_air = sum( dEr1_air + dEr2_air + dEr3_air, 2 );
    
    % calculate imag effect
    dEz1_img = zeros(Nt,Ns_ch);
    dEz2_img = zeros(Nt,Ns_ch);
    dEz3_img = zeros(Nt,Ns_ch);
    dEr1_img = zeros(Nt,Ns_ch);
    dEr2_img = zeros(Nt,Ns_ch);
    dEr3_img = zeros(Nt,Ns_ch);
    
    Rxyz_img = zeros(1,Ns_ch);
    Rz_img = zeros(1,Ns_ch);
    
    for ig = 1:Ns_ch
        
        Rxyz_img(ig) = sqrt(Rxy(ik)^2+(z1/2+z2/2 - z_ch_img(ig))^2);
        
        n_td_tmp = floor( (t_sr - (abs(z_ch_img(ig))/vc/vcof + Rxyz_img(ig)/vc) )/dt );
        id_t = n_td_tmp>0;
        

        Rz_img(ig) = (z1/2+z2/2-z_ch_img(ig));

        if flag_type == 1  %% TL model
            cof_isr = 1/(4*pi*ep0);
        elseif flag_type == 2  %% MTLL
            cof_isr = 1/(4*pi*ep0) * (1 + z_ch_img(ig)/H);
        else %% MTLE
            cof_isr = 1/(4*pi*ep0) * exp(-abs(z_ch_img(ig))/lamda);
        end
        
        dEz1_img_cof = cof_isr*(2*Rz_img(ig)^2-Rxy(ik)^2)/Rxyz_img(ig)^5;
        dEz2_img_cof = cof_isr*(2*Rz_img(ig)^2-Rxy(ik)^2)/Rxyz_img(ig)^4/vc;
        dEz3_img_cof = cof_isr*(Rxy(ik)^2)/Rxyz_img(ig)^3/(vc^2);
        
        dEr1_img_cof = cof_isr*(3*Rz_img(ig)*Rxy(ik))/Rxyz_img(ig)^5;
        dEr2_img_cof = cof_isr*(3*Rz_img(ig)*Rxy(ik))/Rxyz_img(ig)^4/vc;
        dEr3_img_cof = cof_isr*(Rz_img(ig)*Rxy(ik))/Rxyz_img(ig)^3/(vc^2);        
       
        dEz1_img(id_t,ig) = dEz1_img_cof * i_sr_int(n_td_tmp(id_t));
        dEz2_img(id_t,ig) = dEz2_img_cof * i_sr(n_td_tmp(id_t));
        dEz3_img(id_t,ig) = dEz3_img_cof * i_sr_div(n_td_tmp(id_t));
        
        dEr1_img(id_t,ig) = dEr1_img_cof * i_sr_int(n_td_tmp(id_t));
        dEr2_img(id_t,ig) = dEr2_img_cof * i_sr(n_td_tmp(id_t));
        dEr3_img(id_t,ig) = dEr3_img_cof * i_sr_div(n_td_tmp(id_t));
        

    end

    Ez_img = sum( dEz1_img + dEz2_img - dEz3_img, 2 );
    Er_img = sum( dEr1_img + dEr2_img + dEr3_img, 2 );
    
    
    Er_T(1:Nt,ik) = dz_ch*(Er_air+Er_img);
    Ez_T(1:Nt,ik) = dz_ch*(Ez_air+Ez_img);
% 
%     Uout(1:Nt,ik)= Er_T(1:Nt,ik).*Rx(ik)/Rxy(ik)*abs(x1-x2) ...
%         + Er_T(1:Nt,ik)*Ry(ik)/Rxy(ik)*abs(y1-y2) + Ez_T(1:Nt,ik)*abs(z1-z2);
end
E_all_r2=Er_T;
Er_p=E_all_r2;
Ez_p=Ez_T;

% wp=2*1e7*dt;
% ws=2*3e7*dt;
% ap=1;
% as=50;
% [N Wc]=buttord(wp,ws,ap,as);
% [H W]=freqz(N,Wc);
% [B,A]=butter(N,Wc);
% E_all_r3=filter(B,A,E_all_r2);
% 
% % figure(2);
% % plot(dt:dt:t_ob,E_all_r3);
% 
% if ground_type==1 % perfect
%     Uout(1:Nt,ik)= E_all_r3(1:Nt,ik).*Rx(ik)/Rxy(ik)*abs(x1-x2) ...
%         + E_all_r3(1:Nt,ik)*Ry(ik)/Rxy(ik)*abs(y1-y2) + Ez_T(1:Nt,ik)*abs(z1-z2);
% elseif ground_type==2 % lossy
%     [H_save] = H_Cal(pt_hit,h_ch, pt_start,pt_end, i_sr_H,Nt2, dt2,flag_type);
%     Er_lossy=E_Cal_lossy(E_all_r3,H_save,dt,dt2,erg,sigma_g);
%         Uout(1:Nt,ik)= Er_lossy(1:Nt,ik).*Rx(ik)/Rxy(ik)*abs(x1-x2) ...
%         + Er_lossy(1:Nt,ik)*Ry(ik)/Rxy(ik)*abs(y1-y2) + Ez_T(1:Nt,ik)*abs(z1-z2);
% else
%     Uout=0;
% end
%     
%     Uout1(1:Nt,ik)= E_all_r3(1:Nt,ik).*Rx(ik)/Rxy(ik)*abs(x1-x2) ...
%         + E_all_r3(1:Nt,ik)*Ry(ik)/Rxy(ik)*abs(y1-y2) + Ez_T(1:Nt,ik)*abs(z1-z2);
% 
% wp2=2*f_low*dt;
% ws2=2*f_high*dt;
% ap2=1;
% as2=50;
% [N2 Wc2]=buttord(wp2,ws2,ap2,as2);
% [H2 W2]=freqz(N2,Wc2);
% [B2,A2]=butter(N2,Wc2);
% Uout2=filter(B2,A2,Uout);
% 
% % dt_out=4.4e-9;
% 
% xx = dt:dt:t_ob; 
% yy = Uout2; 
% xxi = dt_out:dt_out:t_ob; 
% Uout3 = interp1(xx,yy,xxi,'spline');
% Tout=dt_out:dt_out:t_ob;

end
