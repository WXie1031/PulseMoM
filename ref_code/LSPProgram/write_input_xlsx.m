function write_input_xlsx(bran_name, bran_type, point_start, point_end, ...
    nod_start, nod_start_type, nod_end, nod_end_type, ...
    dv, dim1_cs, dim2_cs, re, len, Rin_pul, Lin_pul, fpath, fname) 
   
% Nc = size(bran_type,1);
% 
% sht_data = cell(Nc,1);
% for ik = 1:Nc
% sht_data{1} = device_type;

sht_data(:,1) = bran_name;
sht_data(:,2) = bran_type;

sht_data(:,3:5) = point_start;
sht_data(:,6:8) = point_end;

sht_data(:,9) = nod_start;
sht_data(:,10) = nod_start_type;
sht_data(:,11) = nod_end;
sht_data(:,12) = nod_end_type;

sht_data(:,13:15) = dv;
sht_data(:,11) = dim1_cs;
sht_data(:,12) = dim2_cs;
sht_data(:,13) = re;
sht_data(:,14) = len;

sht_data(:,15) = Rin_pul;
sht_data(:,16) = Lin_pul;


xlswrite([fpath, fname], sht_data);
    
end



