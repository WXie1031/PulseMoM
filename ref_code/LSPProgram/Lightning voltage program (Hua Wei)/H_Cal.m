function [H_p]  =H_Cal(pt_start,pt_end,flag_type,pt_hit,h_ch, ...
i_sr_type,Imax,tau1,tau2,k0,n,alpha1,alpha2,t_ob,dt_H,Ns_ch,Nc,ve);
[i_sr] = i_sr_select(Imax,tau1,tau2,k0,n,alpha1,alpha2,i_sr_type,t_ob,dt_H);

Nt=round(t_ob/dt_H);
t_sr = dt_H*(1:(Nt));
ep0 = 8.85*1e-12;
% vcof=ve/vc;
if flag_type == 1  %% TL model
    vc = 3e8;
    vcof = ve/vc;
elseif flag_type == 2  %% MTLL   decays with the height
    vc = 3e8;
    vcof = ve/vc;
    H = 7e3;
else %% MTLE
    vc = 3e8;
    vcof = ve/vc;
    lamda = 2e3;  % constant in MTLE -- decays exponentially with the height
end

Nc = size(pt_start,1);


x_hit = pt_hit(:,1);
y_hit = pt_hit(:,2);

dz_ch = h_ch/Ns_ch;
z_ch = ((1:Ns_ch)-0.5)'*dz_ch; % mid point of the channel segment
z_ch_img = -z_ch; % mid point of the channel segment



i_sr_int = zeros(Nt,1);
i_sr_div = zeros(Nt,1);

if size(i_sr,1)==1
    i_sr = i_sr';
end
i_sr_int(1:Nt) = cumsum(i_sr)*dt_H;
i_sr_div(2:Nt) = diff(i_sr)/dt_H;

% distance between midpoint and channel segment in x-y plane
Rx = (pt_start(:,1)/2+pt_end(:,1)/2-x_hit);
Ry = (pt_start(:,2)/2+pt_end(:,2)/2-y_hit);
Rxy = sqrt( Rx.^2 + Ry.^2 ) ;
 
Uout = zeros(Nt,Nc);
Er_T = zeros(Nt,Nc);
Ez_T = zeros(Nt,Nc);
for ik=1:Nc
    %ik
    x1 = pt_start(ik,1);
    y1 = pt_start(ik,2);
%     z1 = pt_start(ik,3);
    z1 = 0;
    
    x2 = pt_end(ik,1);
    y2 = pt_end(ik,2);
    z2 = 0;
%     z2 = pt_end(ik,3);
    
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
        
        n_td_tmp = floor( (t_sr - (z_ch(ig)/vc/vcof + Rxyz(ig)/vc) )/dt_H );
        id_t = n_td_tmp>0;
        
        % delay part
%         dEz_air(id_td,ig)=0;
%         dEr_air(id_td,ig)=0;
        
        % propagate part
        Rz(ig) = (z1/2+z2/2 - z_ch(ig));
        
        if flag_type == 1  %% TL model
            cof_isr = 1/(4*pi*ep0);
        elseif flag_type == 2  %% MTLL
            cof_isr = 1/(4*pi*ep0) * (1 - z_ch(ig)/H);
            
        else %% MTLE
            cof_isr = 1/(4*pi*ep0) * exp(-z_ch(ig)/lamda);
        end
        
        dEz_1_cof = 0*cof_isr * (2*Rz(ig)^2-Rxy(ik)^2)/(Rxyz(ig)^5);
        dEz_2_cof = 0*cof_isr * (2*Rz(ig)^2-Rxy(ik)^2)/(Rxyz(ig)^4)/vc;
        dEz_3_cof = 0*cof_isr * (Rxy(ik)^2)/(Rxyz(ig)^3)/(vc^2);
        
%         dEr_1_cof = cof_isr * (3*Rz(ig)*Rxy(ik))/(Rxyz(ig)^5);
%         dEr_2_cof = cof_isr * (3*Rz(ig)*Rxy(ik))/(Rxyz(ig)^4)/vc;
%         dEr_3_cof = cof_isr * (Rz(ig)*Rxy(ik))/(Rxyz(ig)^3)/(vc^2);

        dEr_1_cof = 0*cof_isr * (3*Rz(ig)*Rxy(ik))/(Rxyz(ig)^5);
        dEr_2_cof = cof_isr * (Rxy(ik))/(Rxyz(ig)^3);
        dEr_3_cof = cof_isr * (Rxy(ik))/(Rxyz(ig)^2)/(vc);
       
        
%         dEz1_air(id_t,ig) = dEz_1_cof * i_sr_int(n_td_tmp(id_t));
%         dEz2_air(id_t,ig) = dEz_2_cof * i_sr(n_td_tmp(id_t));
%         dEz3_air(id_t,ig) = dEz_3_cof * i_sr_div(n_td_tmp(id_t));
%         
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
        
        n_td_tmp = floor( (t_sr - (abs(z_ch_img(ig))/vc/vcof + Rxyz_img(ig)/vc) )/dt_H );
        id_t = n_td_tmp>0;
        

        Rz_img(ig) = (z1/2+z2/2-z_ch_img(ig));

        if flag_type == 1  %% TL model
            cof_isr = 1/(4*pi*ep0);
        elseif flag_type == 2  %% MTLL
            cof_isr = 1/(4*pi*ep0) * (1 + z_ch_img(ig)/H);
        else %% MTLE
            cof_isr = 1/(4*pi*ep0) * exp(-abs(z_ch_img(ig))/lamda);
        end
        
        dEz1_img_cof = 0*cof_isr*(2*Rz_img(ig)^2-Rxy(ik)^2)/Rxyz_img(ig)^5;
        dEz2_img_cof = 0*cof_isr*(2*Rz_img(ig)^2-Rxy(ik)^2)/Rxyz_img(ig)^4/vc;
        dEz3_img_cof = 0*cof_isr*(Rxy(ik)^2)/Rxyz_img(ig)^3/(vc^2);
        
%         dEr1_img_cof = 0*cof_isr*(3*Rz_img(ig)*Rxy(ik))/Rxyz_img(ig)^5;
%         dEr2_img_cof = cof_isr*(3*Rz_img(ig)*Rxy(ik))/Rxyz_img(ig)^4/vc;
%         dEr3_img_cof = cof_isr*(Rz_img(ig)*Rxy(ik))/Rxyz_img(ig)^3/(vc^2);
        
        dEr1_img_cof = 0*cof_isr * (3*Rz(ig)*Rxy(ik))/(Rxyz(ig)^5);
        dEr2_img_cof = cof_isr * (Rxy(ik))/(Rxyz_img(ig)^3);
        dEr3_img_cof = cof_isr * (Rxy(ik))/(Rxyz_img(ig)^2)/(vc);
        
        
%         dEz1_img(id_t,ig) = dEz1_img_cof * i_sr_int(n_td_tmp(id_t));
%         dEz2_img(id_t,ig) = dEz2_img_cof * i_sr(n_td_tmp(id_t));
%         dEz3_img(id_t,ig) = dEz3_img_cof * i_sr_div(n_td_tmp(id_t));
%         
        dEr1_img(id_t,ig) = dEr1_img_cof * i_sr_int(n_td_tmp(id_t));
        dEr2_img(id_t,ig) = dEr2_img_cof * i_sr(n_td_tmp(id_t));
        dEr3_img(id_t,ig) = dEr3_img_cof * i_sr_div(n_td_tmp(id_t));
        

    end

    Ez_img = sum( dEz1_img + dEz2_img - dEz3_img, 2 );
    Er_img = sum( dEr1_img + dEr2_img + dEr3_img, 2 );
    
    
    Er_T(1:Nt,ik) = dz_ch*(Er_air+Er_img);
    Ez_T(1:Nt,ik) = dz_ch*(Ez_air+Ez_img);

%     Uout(1:Nt,ik)= Er_T(1:Nt,ik).*Rx(ik)/Rxy(ik)*abs(x1-x2) ...
%         + Er_T(1:Nt,ik)*Ry(ik)/Rxy(ik)*abs(y1-y2) + Ez_T(1:Nt,ik)*abs(z1-z2);
end
H_all_2=ep0*Er_T;
H_p=H_all_2;
% d0=1;
% H_all_3=H_all_2(d0:d0:Nt);
% H_all_diff2=0;
% for i=1:(Nt/d0-1)
%     H_all_diff2(i+1)=(H_all_3(i+1)-H_all_3(i))/dt_H;
% end
% % plot(H_all_diff2)
% % plot(H_save)
% % plot(conv(H_all_diff2,ones(1000,1)))
% wp_H=2*1e7*dt_H;
% ws_H=2*3e7*dt_H;
% ap_H=1;
% as_H=50;
% [N_H Wc_H]=buttord(wp_H,ws_H,ap_H,as_H);
% [H_H W_H]=freqz(N_H,Wc_H);
% 
% [B_H,A_H]=butter(N_H,Wc_H);
% H_save=filter(B_H,A_H,H_all_3);


end

