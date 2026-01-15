[fopenname, popenname] = uigetfile( ...
{'*.xls; *.xlsx', 'Excel Format (*.xls,*.xlsx)'; ...
'*.csv; *.txt', 'Text Files (*.csv,*.txt)'; ...
'*.*',  'All Files (*.*)'},  'Open');

[fsavename, psavename] = uiputfile( ...
{'*.cir', 'All Files (*.*)'},  'Save');

open_path_name = [popenname, fopenname];
save_path_name = [psavename, fsavename];

tmin = 0;
tmax = 1e-3;
dt = 1e-8;
sr_type = 0;

exitflag = optimize_input_sr(sr_type, open_path_name, save_path_name);



