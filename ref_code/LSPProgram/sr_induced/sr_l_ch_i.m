function iout = sr_l_ch_i(h_ch,Ns_ch, i_sr,t_sr, flag_type)
%  Function:       sr_l_ch_i
%  Description:    Calculate the current on the lightning channel
%
%  Calls:
%
%  Input:          h_ch     --  height of lightning channel (m)
%                  Ns_ch    --  num of segments of the lightning channel
%                  i_sr     --  lightning waveform
%                  t_sr     --  time sequeece (s)
%                  flag_type--  flag to choose the current type
%  Output:         iout  --  currents on segments of the lightning channel
%  Others:
%  Author:         Chen Hongcai
%  Email :         hc.chen@live.com
%  Date:           2017-05-01
%

ep0 = 8.85*1e-12;

if flag_type == 1  %% TL model
    v0 = 1.3e8;
elseif flag_type == 2  %% MTLL
    v0 = 1e8;
    H = 7e3;
else %% MTLE
    v0 = 1.3e8;
    lamda = 2e3;  % constant in MTLE -- decays exponentially with the height
end

Nt = length(t_sr);
dt = t_sr(2)-t_sr(1);

dz_ch = h_ch/Ns_ch;
z_ch = ((1:Ns_ch)-0.5)'*dz_ch; % mid point of the channel segment


if size(i_sr,1)==1
    i_sr = i_sr';
end

iout = zeros(Ns_ch,Nt);

for ig=1:Ns_ch
    
    n_td_tmp = floor( (t_sr - (z_ch(ig)/v0))/dt );
    id_t = n_td_tmp>0;
    
    
    if flag_type == 1  %% TL model
        cof_isr = 1;
    elseif flag_type == 2  %% MTLL
        cof_isr = (1 - z_ch(ig)/H);
    else %% MTLE
        cof_isr = exp(-z_ch(ig)/lamda);
    end
    
    iout(ig,id_t) = -cof_isr*i_sr(n_td_tmp(id_t));
    
end


% fs = 1/dt;
% f_filter = max(fs/20, 200e3);
% if f_filter<fs/2
%     [b,a] = butter(1,f_filter/(fs/2),'low');
%     
%     for ik=1:Ns_ch
%         iout(ik,:) = filtfilt(b,a,iout(ik,:));
%     end
% end

figure;
id_plt = floor(linspace(1,Ns_ch,5));

% plot3(z_ch(id_plt)*ones(1,Nt),t_sr*1e6,iout(id_plt,1:Nt)/1e3);
% ylabel('Time(us)')
% xlabel('Height(m)')
% zlabel('Current(kA)')
% axis ij

plot(t_sr*1e6,iout(id_plt,1:Nt)/1e3);
xlabel('Time(us)')
ylabel('Current(kA)')

end

