function create_double_exp_source(sr_type, sr_amp, sr_tau1, sr_tau2, td, ...
    save_path_name, plot_flag)
% sr_tau1 - us
% sr_tau2 - us
% td - us

% opengl('save','software');

Tmax = max(max(sr_tau1*1.5, sr_tau2*1.5),50);     % us
dt = min(1e-1,sr_tau2/200);  % us
Nt = ceil((Tmax)/dt);
tus = (0:Nt-1)*dt;  % us


if td>0
    Ntd = ceil(td/dt);
    ist = zeros(1,Ntd+Nt);

    ist(Ntd+1:end) = sr_amp  * (exp(-1/sr_tau2*tus)-exp(-1/sr_tau1*tus));
    tus = (0:Ntd+Nt-1)*dt;
else
    ist = sr_amp  * (exp(-1/sr_tau2*tus)-exp(-1/sr_tau1*tus));
end

[ist_max, id_max] = max(abs(ist));

ist_sign = sign(ist(id_max));

ist = ist_sign*ist* sr_amp/ist_max;
sr_amp = sr_amp * sr_amp/ist_max;

%% plot I-t curve
if plot_flag>0

   % [ist_max, id_max] = max(ist);
    
    id10 = knnsearch(ist(1:id_max)',0.1*ist_max);
    id90 = knnsearch(ist(1:id_max)',0.9*ist_max);
    id50 = knnsearch(ist(id90+1:end)',0.5*ist_max);
    id50 = id50+id90;
    
    
    close(gcf);
    set(gcf,'NumberTitle', 'off', 'Name', 'Double Exponential Source');
    clf(gcf);
    
    if sr_amp>3e3
        plot(tus, ist/1e3);
        ylabel('Current (kA)');
        text(tus(id10),ist(id10)/1e3,['\leftarrow t_1_0_%=',num2str(tus(id10)),' us']);
        text(tus(id90),ist(id90)/1e3,['\leftarrow t_9_0_%=',num2str(tus(id90)),' us']);
        text(tus(id50),ist(id50)/1e3,[' \leftarrow t_5_0_%=',num2str(tus(id50)),' us']);
        if td>0
            text(tus(Ntd),0.028*ist_max/1e3,[' \leftarrow td=',num2str(tus(Ntd+1)),' us']);
        end
    else
        plot(tus, ist);
        ylabel('Current (A)');
        text(tus(id10),ist(id10),['\leftarrow t_1_0_%=',num2str(tus(id10)),' us']);
        text(tus(id90),ist(id90),['\leftarrow t_9_0_%=',num2str(tus(id90)),' us']);
        text(tus(id50),ist(id50),[' \leftarrow t_5_0_%=',num2str(tus(id50)),' us']);
        if td>0
            text(tus(Ntd),0.028*ist_max,[' \leftarrow td=',num2str(tus(Ntd+1)),' us']);
        end
    end
    xlabel('Time (us)');
    
    if sr_type == 0
        tr = (tus(id90)-tus(id10))*1.25;
    else
        tr = (tus(id90)-tus(id10))*1.67;
    end
    title( {'Double Exponential Source Waveform', ['t_r_i_s_e = ',num2str(tr),' us, '...
        't_h_a_l_f = ',num2str(tus(id50)), ' us']});
    grid on
end

sr_tau1 = sr_tau1*1e-6;% transform into [s] 
sr_tau2 = sr_tau2*1e-6;% transform into [s] 

td = td*1e-6;
sr_amp = ist_sign*sr_amp;
[pathstr, fname] = fileparts(save_path_name);
spice_double_exp_source(sr_type, sr_amp, sr_tau2, sr_tau1, td, pathstr, fname);


end


