function create_heidler_source(sr_type, sr_amp, sr_tau1, sr_tau2, n, ...
    save_path_name, plot_flag)

% opengl('save','software');

Tmax = max(sr_tau2*1.5, 50);     % us
dt = min(1e-1,sr_tau1/500);  % us
[ist, t] = sr_heidler(sr_amp, sr_tau1, sr_tau2, n, Tmax, dt);

ist_max = max(ist);

ist = ist* sr_amp/ist_max;
sr_amp = sr_amp*sr_amp/ist_max;


%% plot I-t curve
if plot_flag>0

    [ist_max, id_max] = max(ist);
    id10 = knnsearch(ist(1:id_max)',0.1*ist_max);
    id90 = knnsearch(ist(1:id_max)',0.9*ist_max);
    id50 = knnsearch(ist(id90+1:end)',0.5*ist_max);
    id50 = id50+id90;
    

    close(gcf);
    set(gcf,'NumberTitle', 'off', 'Name', 'Heidler Source');

    
    if sr_amp>3e3
        plot(t, ist/1e3);
        ylabel('Current (kA)');
        
        text(t(id10),ist(id10)/1e3,['\leftarrow t_1_0_%=',num2str(t(id10)),' us']);
        text(t(id90),ist(id90)/1e3,['\leftarrow t_9_0_%=',num2str(t(id90)),' us']);
        text(t(id50),ist(id50)/1e3,[' \leftarrow t_5_0_%=',num2str(t(id50)),' us']);
    else
        plot(t, ist);
        ylabel('Current (A)');
        
        text(t(id10),ist(id10),['\leftarrow t_1_0_%=',num2str(t(id10)),' us']);
        text(t(id90),ist(id90),['\leftarrow t_9_0_%=',num2str(t(id90)),' us']);
        text(t(id50),ist(id50),[' \leftarrow t_5_0_%=',num2str(t(id50)),' us']);
    end
    xlabel('Time (us)');
    
    if sr_type == 0
        tr = (t(id90)-t(id10))*1.25;
    else
        tr = (t(id90)-t(id10))*1.67;
    end
    title( {'Heidler Source Waveform', ['t_r_i_s_e = ',num2str(tr),' us, '...
        't_h_a_l_f = ',num2str(t(id50)), ' us']});
    grid on
end


%% generate pspice file
[pathstr, fname] = fileparts(save_path_name) ;

spice_heidler_source(sr_type, sr_amp, sr_tau1, sr_tau2, n, ...
    pathstr, fname)


end

