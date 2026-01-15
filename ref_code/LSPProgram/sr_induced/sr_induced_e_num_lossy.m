function [Ur,Uz,Er_T,Ez_T,Ha0_T] = sr_induced_e_num_lossy(pt_hit,h_ch,Ns_ch, ...
    pt_start,pt_end,pt_nod, i_sr,t_sr, soil_sig,soil_epr, flag_type)
%  Function:       sr_induced_e_num_lossy
%  Description:    Calculate induced fields of all conductors using
%                  numerical integration of Uman's formula. 3 types of
%                  lighting current are programmed as introduced in "A new
%                  lightning stroke model based on antenna theory"
%                  Static field is included for field calculation. Lossy
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
%                  flag_type--  flag to choose the current type
%  Output:         Uout  --  induced U on conductors
%                  Er_T  --  induced Er field on conductors
%                  Ez_T  --  induced Ez field on conductors
%  Others:
%  Author:         Chen Hongcai
%  Email :         hc.chen@live.com
%  Date:           2017-01-17
% 

%mu0 = 4*pi*1e-7;
ep0 = 8.85*1e-12;

if flag_type == 1  %% TL model
    vc = 3e8;
    v0 = 1.1e8;
elseif flag_type == 2  %% MTLL
    vc = 3e8;
    v0 = 1.3e8;
    H = 7e3;
else %% MTLE
    vc = 3e8;
    v0 = 1.3e8;
    lamda = 2e3;  % constant in MTLE -- decays exponentially with the height
end

Nc = size(pt_start,1);
Nn = size(pt_nod,1);
Nt = length(t_sr);

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



Ur = zeros(Nc,Nt);
Uz = zeros(Nn,Nt);
Er_T = zeros(Nt,Nc);
Ez_T = zeros(Nt,Nc);
Ha0_T = zeros(Nt,Nc);

%% 1. calculate the induced voltage on the branch (Ex,Ey)
% distance between midpoint and channel segment in x-y plane
RxEr = (pt_start(:,1)/2+pt_end(:,1)/2-x_hit);
RyEr = (pt_start(:,2)/2+pt_end(:,2)/2-y_hit);
RxyEr = sqrt( RxEr.^2 + RyEr.^2 ) ;
for ik=1:Nc
    %ik
    z1 = pt_start(ik,3);
    z2 = pt_end(ik,3);
    
    % 1. calculate the E generate in the air
%     dEz1_air = zeros(Nt,Ns_ch);
%     dEz2_air = zeros(Nt,Ns_ch);
%     dEz3_air = zeros(Nt,Ns_ch);
    dEr1_air = zeros(Nt,Ns_ch);
    dEr2_air = zeros(Nt,Ns_ch);
    dEr3_air = zeros(Nt,Ns_ch);
%     dEz_air = zeros(Nt,Ns_ch);
%     dEr_air = zeros(Nt,Ns_ch);

    Ha2_air = zeros(Nt,Ns_ch);
    Ha3_air = zeros(Nt,Ns_ch);
    
    RxyzEr = zeros(1,Ns_ch);
    RzEr = zeros(1,Ns_ch);

    for ig=1:Ns_ch

        RzEr(ig) = (z1/2+z2/2 - z_ch(ig));
        RxyzEr(ig) = sqrt( RxyEr(ik)^2+RzEr(ig)^2 );
        
        n_td_tmp = floor( (t_sr - (z_ch(ig)/v0 + RxyzEr(ig)/vc) )/dt );
        id_t = n_td_tmp>0;
        
        if flag_type == 1  %% TL model
            cof_isr = dz_ch * 1/(4*pi*ep0);
        elseif flag_type == 2  %% MTLL
            cof_isr = dz_ch * 1/(4*pi*ep0) * (1 - z_ch(ig)/H);
        else %% MTLE
            cof_isr = dz_ch * 1/(4*pi*ep0) * exp(-z_ch(ig)/lamda);
        end
        
%         dEz_1_cof = cof_isr * ( 2*RzEr(ig)^2-RxyEr(ik)^2 )/(RxyzEr(ig)^5);
%         dEz_2_cof = cof_isr * ( 2*RzEr(ig)^2-RxyEr(ik)^2)/(RxyzEr(ig)^4)/vc;
%         dEz_3_cof = cof_isr * ( RxyEr(ik)^2 )/(RxyzEr(ig)^3)/(vc^2);
        
        dEr_1_cof = cof_isr * ( 3*RzEr(ig)*RxyEr(ik) )/(RxyzEr(ig)^5);
        dEr_2_cof = cof_isr * ( 3*RzEr(ig)*RxyEr(ik) )/(RxyzEr(ig)^4)/vc;
        dEr_3_cof = cof_isr * ( RzEr(ig)*RxyEr(ik) )/(RxyzEr(ig)^3)/(vc^2);

%         dEz1_air(id_t,ig) = dEz_1_cof * i_sr_int(n_td_tmp(id_t));
%         dEz2_air(id_t,ig) = dEz_2_cof * i_sr(n_td_tmp(id_t));
%         dEz3_air(id_t,ig) = dEz_3_cof * i_sr_div(n_td_tmp(id_t));
        
        dEr1_air(id_t,ig) = dEr_1_cof * i_sr_int(n_td_tmp(id_t));
        dEr2_air(id_t,ig) = dEr_2_cof * i_sr(n_td_tmp(id_t));
        dEr3_air(id_t,ig) = dEr_3_cof * i_sr_div(n_td_tmp(id_t));
        
        %dEz_air(id_t,ig) = dEz1_air(id_t,ig) + dEz2_air(id_t,ig) - dEz3_air(id_t,ig);
        %dEr_air(id_t,ig) = dEr1_air(id_t,ig) + dEr2_air(id_t,ig) + dEr3_air(id_t,ig);
    
        % H field at z=0 level
        Rz0 = (- z_ch(ig)); 
        Rxyz0 = sqrt( RxyEr(ik)^2+Rz0^2 );
        
        n_td_tmp = floor( (t_sr - (z_ch(ig)/v0 + Rxyz0/vc) )/dt );
        id_t = n_td_tmp>0;
        
        Ha2_cof = cof_isr * ep0 * ( RxyEr(ik) )/(Rxyz0^3);
        Ha3_cof = cof_isr * ep0 * ( RxyEr(ik) )/(Rxyz0^2*vc);
        
        Ha2_air(id_t,ig) = Ha2_cof * i_sr(n_td_tmp(id_t));
        Ha3_air(id_t,ig) = Ha3_cof * i_sr_div(n_td_tmp(id_t));
    end
    
%     Ez_air = sum( dEz1_air + dEz2_air - dEz3_air, 2 );
    Er_air = sum( dEr1_air + dEr2_air + dEr3_air, 2 );
    
    Ha0_air = sum( Ha2_air + Ha3_air, 2 );
    
    % calculate imag effect
%     dEz1_img = zeros(Nt,Ns_ch);
%     dEz2_img = zeros(Nt,Ns_ch);
%     dEz3_img = zeros(Nt,Ns_ch);
    dEr1_img = zeros(Nt,Ns_ch);
    dEr2_img = zeros(Nt,Ns_ch);
    dEr3_img = zeros(Nt,Ns_ch);
    
    Ha2_img = zeros(Nt,Ns_ch);
    Ha3_img = zeros(Nt,Ns_ch);
    
    RxyzEr_img = zeros(1,Ns_ch);
    RzEr_img = zeros(1,Ns_ch);
    
    for ig = 1:Ns_ch
        
        RzEr_img(ig) = (z1/2+z2/2-z_ch_img(ig));
        RxyzEr_img(ig) = sqrt( RxyEr(ik)^2+RzEr_img(ig)^2 );
        
        n_td_tmp = floor( (t_sr - (abs(z_ch_img(ig))/v0 + RxyzEr_img(ig)/vc) )/dt );
        id_t = n_td_tmp>0;
        

        if flag_type == 1  %% TL model
            cof_isr = dz_ch * 1/(4*pi*ep0);
        elseif flag_type == 2  %% MTLL
            cof_isr = dz_ch * 1/(4*pi*ep0) * (1 + z_ch_img(ig)/H);
        else %% MTLE
            cof_isr = dz_ch * 1/(4*pi*ep0) * exp(-abs(z_ch_img(ig))/lamda);
        end
        
%         dEz1_img_cof = cof_isr * ( 2*RzEr_img(ig)^2-RxyEr(ik)^2 )/(RxyzEr_img(ig)^5);
%         dEz2_img_cof = cof_isr * ( 2*RzEr_img(ig)^2-RxyEr(ik)^2 )/(RxyzEr_img(ig)^4)/vc;
%         dEz3_img_cof = cof_isr * ( RxyEr(ik)^2 )/(RxyzEr_img(ig)^3)/(vc^2);
        
        dEr1_img_cof = cof_isr * ( 3*RzEr_img(ig)*RxyEr(ik) )/(RxyzEr_img(ig)^5);
        dEr2_img_cof = cof_isr * ( 3*RzEr_img(ig)*RxyEr(ik) )/(RxyzEr_img(ig)^4)/vc;
        dEr3_img_cof = cof_isr * ( RzEr_img(ig)*RxyEr(ik) )/(RxyzEr_img(ig)^3)/(vc^2);
        
        
%         dEz1_img(id_t,ig) = dEz1_img_cof * i_sr_int(n_td_tmp(id_t));
%         dEz2_img(id_t,ig) = dEz2_img_cof * i_sr(n_td_tmp(id_t));
%         dEz3_img(id_t,ig) = dEz3_img_cof * i_sr_div(n_td_tmp(id_t));
        
        dEr1_img(id_t,ig) = dEr1_img_cof * i_sr_int(n_td_tmp(id_t));
        dEr2_img(id_t,ig) = dEr2_img_cof * i_sr(n_td_tmp(id_t));
        dEr3_img(id_t,ig) = dEr3_img_cof * i_sr_div(n_td_tmp(id_t));
        
        %dEz_img(id_t,ig) = dEz1_img(id_t,ig) + dEz2_img(id_t,ig) - dEz3_img(id_t,ig);       
        %dEr_img(id_t,ig) = dEr1_img(id_t,ig) + dEr2_img(id_t,ig) + dEr3_img(id_t,ig);
    
        % H field at z=0 level
        Rz0_img = (- z_ch_img(ig) ); 
        Rxyz0_img = sqrt( RxyEr(ik)^2+Rz0_img^2 );
        
        n_td_tmp = floor( (t_sr - (abs(z_ch_img(ig))/v0 + Rxyz0_img/vc) )/dt );
        id_t = n_td_tmp>0;
        
        Ha2_img_cof = cof_isr * ep0 * ( RxyEr(ik) )/(Rxyz0_img^3);
        Ha3_img_cof = cof_isr * ep0 * ( RxyEr(ik) )/(Rxyz0_img^2*vc);
        
        Ha2_img(id_t,ig) = Ha2_img_cof * i_sr(n_td_tmp(id_t));
        Ha3_img(id_t,ig) = Ha3_img_cof * i_sr_div(n_td_tmp(id_t));
    
    end

%     Ez_img = sum( dEz1_img + dEz2_img - dEz3_img, 2 );
    Er_img = sum( dEr1_img + dEr2_img + dEr3_img, 2 );

    Ha0_img = sum( Ha2_img + Ha3_img, 2 );
    
    Er_T(1:Nt,ik) = (Er_air+Er_img);
%     Ez_T(1:Nt,ik) = (Ez_air+Ez_img);
    Ha0_T(1:Nt,ik) = (Ha0_air+Ha0_img);
end



%% 2. calculate the induced voltage at the node (Ez)
RxEz = (pt_nod(:,1)-x_hit);
RyEz = (pt_nod(:,2)-y_hit);
RxyEz = sqrt( RxEz.^2 + RyEz.^2 ) ;
for ik=1:Nn
    %ik
    z0 = pt_nod(ik,3);
    
    % 1. calculate the E generate in the air
    dEz1_air = zeros(Nt,Ns_ch);
    dEz2_air = zeros(Nt,Ns_ch);
    dEz3_air = zeros(Nt,Ns_ch);
    
    RxyzEz = zeros(1,Ns_ch);
    RzEz = zeros(1,Ns_ch);

    for ig=1:Ns_ch

        RzEz(ig) = (z0 - z_ch(ig));
        RxyzEz(ig) = sqrt( RxyEz(ik)^2+RzEz(ig)^2 );
        
        n_td_tmp = floor( (t_sr - (z_ch(ig)/v0 + RxyzEz(ig)/vc) )/dt );
        id_t = n_td_tmp>0;
        
        if flag_type == 1  %% TL model
            cof_isr = dz_ch * 1/(4*pi*ep0);
        elseif flag_type == 2  %% MTLL
            cof_isr = dz_ch * 1/(4*pi*ep0) * (1 - z_ch(ig)/H);
        else %% MTLE
            cof_isr = dz_ch * 1/(4*pi*ep0) * exp(-z_ch(ig)/lamda);
        end
        
        dEz_1_cof = cof_isr * ( 2*RzEz(ig)^2-RxyEz(ik)^2 )/(RxyzEz(ig)^5);
        dEz_2_cof = cof_isr * ( 2*RzEz(ig)^2-RxyEz(ik)^2)/(RxyzEz(ig)^4)/vc;
        dEz_3_cof = cof_isr * ( RxyEz(ik)^2 )/(RxyzEz(ig)^3)/(vc^2);

        dEz1_air(id_t,ig) = dEz_1_cof * i_sr_int(n_td_tmp(id_t));
        dEz2_air(id_t,ig) = dEz_2_cof * i_sr(n_td_tmp(id_t));
        dEz3_air(id_t,ig) = dEz_3_cof * i_sr_div(n_td_tmp(id_t));
    end
    
    Ez_air = sum( dEz1_air + dEz2_air - dEz3_air, 2 );
    
    % calculate imag effect
    dEz1_img = zeros(Nt,Ns_ch);
    dEz2_img = zeros(Nt,Ns_ch);
    dEz3_img = zeros(Nt,Ns_ch);
    
    RxyzEz_img = zeros(1,Ns_ch);
    RzEz_img = zeros(1,Ns_ch);
    
    for ig = 1:Ns_ch
        
        RzEz_img(ig) = (z0 - z_ch_img(ig));
        RxyzEz_img(ig) = sqrt( RxyEz(ik)^2+RzEz_img(ig)^2 );
        
        n_td_tmp = floor( (t_sr - (abs(z_ch_img(ig))/v0 + RxyzEz_img(ig)/vc) )/dt );
        id_t = n_td_tmp>0;
        
        if flag_type == 1  %% TL model
            cof_isr = dz_ch * 1/(4*pi*ep0);
        elseif flag_type == 2  %% MTLL
            cof_isr = dz_ch * 1/(4*pi*ep0) * (1 + z_ch_img(ig)/H);
        else %% MTLE
            cof_isr = dz_ch * 1/(4*pi*ep0) * exp(-abs(z_ch_img(ig))/lamda);
        end
        
        dEz1_img_cof = cof_isr * ( 2*RzEz_img(ig)^2-RxyEz(ik)^2 )/(RxyzEz_img(ig)^5);
        dEz2_img_cof = cof_isr * ( 2*RzEz_img(ig)^2-RxyEz(ik)^2 )/(RxyzEz_img(ig)^4)/vc;
        dEz3_img_cof = cof_isr * ( RxyEz(ik)^2 )/(RxyzEz_img(ig)^3)/(vc^2);
        
        dEz1_img(id_t,ig) = dEz1_img_cof * i_sr_int(n_td_tmp(id_t));
        dEz2_img(id_t,ig) = dEz2_img_cof * i_sr(n_td_tmp(id_t));
        dEz3_img(id_t,ig) = dEz3_img_cof * i_sr_div(n_td_tmp(id_t));
    end

    Ez_img = sum( dEz1_img + dEz2_img - dEz3_img, 2 );
    Ez_T(1:Nt,ik) = (Ez_air+Ez_img);
end



Er_T = Er_T';
Ez_T = Ez_T';
Ha0_T = Ha0_T';

% smooth the curves
fs = 1/dt;
f_filter = max(fs/10,200e3);
if f_filter<fs/2
    [b,a] = butter(1,f_filter/(fs/2),'low');
    
    for ik=1:Nc
        Er_T(ik,:) = filtfilt(b,a,Er_T(ik,:));
        Ha0_T(ik,:) = filtfilt(b,a,Ha0_T(ik,:));
    end
    for ik=1:Nn
        Ez_T(ik,:) = filtfilt(b,a,Ez_T(ik,:));
    end
end

% add the lossy ground usign convelution 
Hcov_T = sr_lossy_h_conv(Ha0_T, t_sr, soil_sig,soil_epr, 4);

Er_T = Er_T - Hcov_T;


dxEr = pt_end(:,1)-pt_start(:,1);
dyEr = pt_end(:,2)-pt_start(:,2);
for ik = 1:Nc
    %     Uout(ik,1:Nt)= Er_T(ik,1:Nt) .* (Rx(ik)/Rxy(ik)*(dx(ik)) ...
    %         + Ry(ik)/Rxy(ik)*(dy(ik))) + Ez_T(ik,1:Nt)*(dz(ik));
    Ur(ik,1:Nt) = Er_T(ik,1:Nt) .* (RxEr(ik)/RxyEr(ik)*(dxEr(ik)) ...
        + RyEr(ik)/RxyEr(ik)*(dyEr(ik)));
end

for ik = 1:Nn
    Uz(ik,1:Nt) = -Ez_T(ik,1:Nt)*pt_nod(ik,3);
end



end

