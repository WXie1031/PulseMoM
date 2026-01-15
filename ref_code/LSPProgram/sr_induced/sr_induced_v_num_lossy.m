function [Uout,Er_T,Ez_T,Ha0_T] = sr_induced_v_num_lossy(pt_hit,h_ch,Ns_ch, ...
    pt_start,pt_end, i_sr,t_sr, soil_sig,soil_epr, flag_type)
%  Function:       sr_induced_v_num_lossy
%  Description:    Calculate induced voltage of all conductors using
%                  numerical integration of Uman's formula. 3 types of
%                  lighting current are programmed as introduced in "A new
%                  lightning stroke model based on antenna theory" Lossy
%                  ground is considered using the Rubinstein's formula as
%                  introduced in "Influence of a lossy ground on
%                  lightning-induced voltages on overhead lines"
%
%  Calls:          
%
%  Input:          pt_hit   --  lightning strike point in xy plane (1*2) (m)
%                  h_ch     --  height of lightning channel (m)
%                  Ns_ch    --  num of segments of the lightning channel
%                  pt_start --  start point of conductors (N*3) (m)
%                  pt_end   --  end point of conductors (N*3) (m)
%                  i_sr     --  lightning waveform
%                  t_sr     --  time sequeece (s)
%                  soil_sig --  resistivity of the soil
%                  soil_epr --  relative conductivity of the soil
%                  flag_type--  flag to choose the current type
%  Output:         Uout  --  induced U on conductors
%                  Er_T  --  induced Er field on conductors
%                  Ez_T  --  induced Ez field on conductors
%                  Ha0_T --  induced Ha field at z=0
%  Others:
%  Author:         Chen Hongcai
%  Email :         hc.chen@live.com
%  Date:           2017-01-17
% 

mu0 = 4*pi*1e-7;
ep0 = 8.85*1e-12;

%% 1. select the model of the lightning channel
if flag_type == 1  %% TL model
    vc = 3e8;
    v0 = 1.3e8;
elseif flag_type == 2  %% MTLL
    vc = 3e8;
    v0 = 1.3e8;
    H = 7e3;
else %% MTLE
    vc = 3e8;
    v0 = 1.3e8;
    lamda = 2e3;  % constant in MTLE -- decays exponentially with the height
end

%% 2. calculate the E and H field for every conductor
Nc = size(pt_start,1);
Nt = length(t_sr);

x_hit = pt_hit(:,1);
y_hit = pt_hit(:,2);

dz_ch = h_ch/Ns_ch;
z_ch = ((1:Ns_ch)-0.5)'*dz_ch; % mid point of the channel segment
z_ch_img = -z_ch; % mid point of the channel segment

dt = t_sr(2)-t_sr(1);

i_sr_div = zeros(Nt,1);
if size(i_sr,1)==1
    i_sr = i_sr';
end
%i_sr_int(1:Nt) = cumsum(i_sr)*dt;
% forward difference
i_sr_div(2:Nt) = diff(i_sr)/dt;
% center difference
%i_sr_div(2:Nt-1)=(i_sr(3:end)-i_sr(1:end-2))/(2*dt);
% distance between midpoint and channel segment in x-y plane
Rx = (pt_start(:,1)/2+pt_end(:,1)/2-x_hit);
Ry = (pt_start(:,2)/2+pt_end(:,2)/2-y_hit);
Rxy = sqrt( Rx.^2 + Ry.^2 ) ;
 
dx = pt_end(:,1)-pt_start(:,1);
dy = pt_end(:,2)-pt_start(:,2);
dz = pt_end(:,3)-pt_start(:,3);

Uout = zeros(Nt,Nc);
Er_T = zeros(Nt,Nc);
Ez_T = zeros(Nt,Nc);
Ha0_T = zeros(Nt,Nc);
for ik=1:Nc
    %ik
    z1 = pt_start(ik,3);
    z2 = pt_end(ik,3);
    
    % 1. calculate the E generate in the air
    %dEz1_air = zeros(Nt,Ns_ch);
    dEz2_air = zeros(Nt,Ns_ch);
    dEz3_air = zeros(Nt,Ns_ch);
    %dEr1_air = zeros(Nt,Ns_ch);
    dEr2_air = zeros(Nt,Ns_ch);
    dEr3_air = zeros(Nt,Ns_ch);
%     dEz_air = zeros(Nt,Ns_ch);
%     dEr_air = zeros(Nt,Ns_ch);
    Ha2_air = zeros(Nt,Ns_ch);
    Ha3_air = zeros(Nt,Ns_ch);

    Rxyz = zeros(1,Ns_ch);
    Rz = zeros(1,Ns_ch);

    for ig=1:Ns_ch

        Rz(ig) = (z1/2+z2/2 - z_ch(ig));
        Rxyz(ig) = sqrt( Rxy(ik)^2+Rz(ig)^2 );
        
        n_td_tmp = floor( (t_sr - (z_ch(ig)/v0 + Rxyz(ig)/vc) )/dt );
        id_t = n_td_tmp>0;
        
        if flag_type == 1  %% TL model
            cof_isr = dz_ch * 1/(4*pi*ep0);
        elseif flag_type == 2  %% MTLL
            cof_isr = dz_ch * 1/(4*pi*ep0) * (1 - z_ch(ig)/H);
        else %% MTLE
            cof_isr = dz_ch * 1/(4*pi*ep0) * exp(-z_ch(ig)/lamda);
        end
        % 1-static field   2-   3-scatted field
        %dEz_1_cof = cof_isr * ( 2*Rz(ig)^2-Rxy(ik)^2 )/(Rxyz(ig)^5);
        dEz2_cof = cof_isr * ( 2*Rz(ig)^2-Rxy(ik)^2)/(Rxyz(ig)^4)/vc;
        dEz3_cof = cof_isr * ( Rxy(ik)^2 )/(Rxyz(ig)^3)/(vc^2);
        
        %dEr_1_cof = cof_isr * ( 3*Rz(ig)*Rxy(ik) )/(Rxyz(ig)^5);
        dEr2_cof = cof_isr * ( 3*Rz(ig)*Rxy(ik) )/(Rxyz(ig)^4)/vc;
        dEr3_cof = cof_isr * ( Rz(ig)*Rxy(ik) )/(Rxyz(ig)^3)/(vc^2);
        
        %dEz1_air(id_t,ig) = dEz_1_cof * i_sr_int(n_td_tmp(id_t));
        dEz2_air(id_t,ig) = dEz2_cof * i_sr(n_td_tmp(id_t));
        dEz3_air(id_t,ig) = dEz3_cof * i_sr_div(n_td_tmp(id_t));
        
        %dEr1_air(id_t,ig) = dEr_1_cof * i_sr_int(n_td_tmp(id_t));
        dEr2_air(id_t,ig) = dEr2_cof * i_sr(n_td_tmp(id_t));
        dEr3_air(id_t,ig) = dEr3_cof * i_sr_div(n_td_tmp(id_t));
        
        %dEz_air(id_t,ig) = dEz1_air(id_t,ig) + dEz2_air(id_t,ig) - dEz3_air(id_t,ig);
        %dEr_air(id_t,ig) = dEr1_air(id_t,ig) + dEr2_air(id_t,ig) + dEr3_air(id_t,ig);
        
        % H field at z=0 level
        Rz0 = ( - z_ch(ig)); 
        Rxyz0 = sqrt( Rxy(ik)^2+Rz0^2 );
        
        n_td_tmp = floor( (t_sr - (z_ch(ig)/v0 + Rxyz0/vc) )/dt );
        id_t = n_td_tmp>0;
        
        Ha2_cof = cof_isr * ep0 * ( Rxy(ik) )/(Rxyz0^3);
        Ha3_cof = cof_isr * ep0 * ( Rxy(ik) )/(Rxyz0^2*vc);
        
        Ha2_air(id_t,ig) = Ha2_cof * i_sr(n_td_tmp(id_t));
        Ha3_air(id_t,ig) = Ha3_cof * i_sr_div(n_td_tmp(id_t));
    end
    
%     Ez_air = sum( dEz1_air + dEz2_air - dEz3_air, 2 );
%     Er_air = sum( dEr1_air + dEr2_air + dEr3_air, 2 );
    Ez_air = sum( dEz2_air - dEz3_air, 2 );
    Er_air = sum( dEr2_air + dEr3_air, 2 );
    
    Ha0_air = sum( Ha2_air + Ha3_air, 2 );

    
    % calculate imag effect
    %dEz1_img = zeros(Nt,Ns_ch);
    dEz2_img = zeros(Nt,Ns_ch);
    dEz3_img = zeros(Nt,Ns_ch);
    %dEr1_img = zeros(Nt,Ns_ch);
    dEr2_img = zeros(Nt,Ns_ch);
    dEr3_img = zeros(Nt,Ns_ch);
%     dEz_img=zeros(Nt,Ns_ch);
%     dEr_img=zeros(Nt,Ns_ch);
    
    Ha2_img = zeros(Nt,Ns_ch);
    Ha3_img = zeros(Nt,Ns_ch);
    
    Rxyz_img = zeros(1,Ns_ch);
    Rz_img = zeros(1,Ns_ch);
    
    for ig = 1:Ns_ch
        
        Rz_img(ig) = (z1/2+z2/2-z_ch_img(ig));
        Rxyz_img(ig) = sqrt( Rxy(ik)^2+Rz_img(ig)^2 );
        
        n_td_tmp = floor( (t_sr - (abs(z_ch_img(ig))/v0 + Rxyz_img(ig)/vc) )/dt );
        id_t = n_td_tmp>0;
        

        if flag_type == 1  %% TL model
            cof_isr = dz_ch * 1/(4*pi*ep0);
        elseif flag_type == 2  %% MTLL
            cof_isr = dz_ch * 1/(4*pi*ep0) * (1 + z_ch_img(ig)/H);
        else %% MTLE
            cof_isr = dz_ch * 1/(4*pi*ep0) * exp(-abs(z_ch_img(ig))/lamda);
        end
        
        %dEz1_img_cof = cof_isr * ( 2*Rz_img(ig)^2-Rxy(ik)^2 )/(Rxyz_img(ig)^5);
        dEz2_img_cof = cof_isr * ( 2*Rz_img(ig)^2-Rxy(ik)^2 )/(Rxyz_img(ig)^4)/vc;
        dEz3_img_cof = cof_isr * ( Rxy(ik)^2 )/(Rxyz_img(ig)^3)/(vc^2);
        
        %dEr1_img_cof = cof_isr * ( 3*Rz_img(ig)*Rxy(ik) )/(Rxyz_img(ig)^5);
        dEr2_img_cof = cof_isr * ( 3*Rz_img(ig)*Rxy(ik) )/(Rxyz_img(ig)^4)/vc;
        dEr3_img_cof = cof_isr * ( Rz_img(ig)*Rxy(ik) )/(Rxyz_img(ig)^3)/(vc^2);
        
        
        %dEz1_img(id_t,ig) = dEz1_img_cof * i_sr_int(n_td_tmp(id_t));
        dEz2_img(id_t,ig) = dEz2_img_cof * i_sr(n_td_tmp(id_t));
        dEz3_img(id_t,ig) = dEz3_img_cof * i_sr_div(n_td_tmp(id_t));
        
        %dEr1_img(id_t,ig) = dEr1_img_cof * i_sr_int(n_td_tmp(id_t));
        dEr2_img(id_t,ig) = dEr2_img_cof * i_sr(n_td_tmp(id_t));
        dEr3_img(id_t,ig) = dEr3_img_cof * i_sr_div(n_td_tmp(id_t));
        
        %dEz_img(id_t,ig) = dEz1_img(id_t,ig) + dEz2_img(id_t,ig) - dEz3_img(id_t,ig);       
        %dEr_img(id_t,ig) = dEr1_img(id_t,ig) + dEr2_img(id_t,ig) + dEr3_img(id_t,ig);
    
        % H field at z=0 level
        Rz0_img = ( - z_ch_img(ig) ); 
        Rxyz0_img = sqrt( Rxy(ik)^2+Rz0_img^2 );
        
        n_td_tmp = floor( (t_sr - (abs(z_ch_img(ig))/v0 + Rxyz0_img/vc) )/dt );
        id_t = n_td_tmp>0;
        
        Ha2_img_cof = cof_isr * ep0 * ( Rxy(ik) )/(Rxyz0_img^3);
        Ha3_img_cof = cof_isr * ep0 * ( Rxy(ik) )/(Rxyz0_img^2*vc);
        
        Ha2_img(id_t,ig) = Ha2_img_cof * i_sr(n_td_tmp(id_t));
        Ha3_img(id_t,ig) = Ha3_img_cof * i_sr_div(n_td_tmp(id_t));
    
    end

%     Ez_img = sum( dEz1_img + dEz2_img - dEz3_img, 2 );
%     Er_img = sum( dEr1_img + dEr2_img + dEr3_img, 2 );
    Ez_img = sum( dEz2_img - dEz3_img, 2 );
    Er_img = sum( dEr2_img + dEr3_img, 2 );    

    Ha0_img = sum( Ha2_img + Ha3_img, 2 );
    
    Er_T(1:Nt,ik) = (Er_air+Er_img);
    Ez_T(1:Nt,ik) = (Ez_air+Ez_img);
    Ha0_T(1:Nt,ik) = (Ha0_air+Ha0_img);
   
end

Er_T = Er_T';
Ez_T = Ez_T';
Ha0_T = Ha0_T';


%% 3. smooth the curves
fs = 1/dt;
f_filter = max(fs/10,200e3);
if f_filter<fs/2
    [b,a] = butter(1,f_filter/(fs/2),'low');
    
    for ik=1:Nc
        Er_T(ik,:) = filtfilt(b,a,Er_T(ik,:));
        Ez_T(ik,:) = filtfilt(b,a,Ez_T(ik,:));
        Ha0_T(ik,:) = filtfilt(b,a,Ha0_T(ik,:));
    end
end

%% 4. add the lossy ground usign convelution
Hcov_T = sr_lossy_h_conv(Ha0_T, t_sr, soil_sig,soil_epr, 1);
    
for ik = 1:Nc
    Uout(ik,1:Nt)= (Er_T(ik,1:Nt)-Hcov_T(ik,1:Nt)) .* (Rx(ik)/Rxy(ik)*(dx(ik)) ...
        + Ry(ik)/Rxy(ik)*(dy(ik))) + Ez_T(ik,1:Nt)*(dz(ik));
end



end

