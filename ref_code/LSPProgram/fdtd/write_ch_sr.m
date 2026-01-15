function write_ch_sr(fpath, fname, pt_name, sr_name)

% open file
fid = fopen( [fpath,fname], 'rb' ); % target file
if( fid == -1 )
    return;
end;

tline = fgetl(fid);
cnt = 1;
cnt_in_sr = 0;
while ischar(tline)
    if regexp(tline, [pt_name, ['(1,%3d)',cnt]])
        cnt = cnt+1;
        cnt_in_sr = 1;
    end
    
    tline = fgetl(fid);
    if cnt_in_sr>0 && cnt_in_sr<16
        cnt_in_sr = cnt_in_sr+1;
    elseif cnt_in_sr==16
        fprint(fid,'                            // Filename   %s',...
            [fpath,sr_name,['%4d.txt',cnt]]);
        cnt_in_sr = 0;
    end
    
end
    
fclose(fid);


fid = fopen([fpath, fname,'.txt'],'w+');
fprintf(fid,'* Add induced voltage files. \n');
for ik = 1:Nbran
    fprintf(fid,'.STMLIB "../../../%s.stl" \n', deblank(Uname(ik,:)));
end

fclose(fid);



