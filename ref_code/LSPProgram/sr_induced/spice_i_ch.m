function spice_i_ch(t, ich, fpath, fname)


%% writing stimulus file
fid = fopen([fpath,'\',fname,'.txt'],'w+');

Npt = length(t);
for ik = 1:Npt
    fprintf(fid,'%.6e  %.6e \n',t(ik),ich(ik));
end

fclose(fid);



end

