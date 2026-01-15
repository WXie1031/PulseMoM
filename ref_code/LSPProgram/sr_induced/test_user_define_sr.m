
[fopenname, popenname] = uigetfile( ...
{'*.xls; *.xlsx', 'Excel Format (*.xls,*.xlsx)'; ...
'*.csv; *.txt', 'Text Files (*.csv,*.txt)'; ...
'*.*',  'All Files (*.*)'},  'Open');

[fsavename, psavename] = uiputfile( ...
{'*.stl', 'All Files (*.*)'},  'Save');

open_path_name = [popenname, fopenname];
save_path_name = [psavename, fsavename];

fbf = 100e6;
sr_scale = 1;
data_type=1;

create_user_define_source(open_path_name, save_path_name, ...
    fbf, sr_scale, data_type, 0, 1);


