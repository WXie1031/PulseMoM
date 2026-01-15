
% 
% [FileName,PathName] = uiputfile({'*.cir', 'Circuit Files (*.cir)'});
% save_path_name = [PathName, FileName];

save_path_name=[];


sr_type=0;
sr_amp=35;
sr_tau1 = 0.31;
sr_tau2 = 12;
td = 0.0;


sr_type=0;
sr_amp=9.7;
sr_tau1 = 9.5;
sr_tau2 = 85;
td = 0.0;


sr_type=0;
sr_amp=50e3;
sr_tau1 = 143.1;
sr_tau2 = 92.4e-3;
td = 0.0;

create_double_exp_source(sr_type, sr_amp, sr_tau1, sr_tau2, td,save_path_name,1)

optimize_double_exp_source(sr_type, sr_amp, sr_tau1, sr_tau2, td, ...
    save_path_name, 1)



