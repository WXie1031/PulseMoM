function write_source_txt(t, sr, fpath, fname)

% t--(s)
% sr--(A/V)

Nt = length(t);
Ns = length(sr);
N = min(Nt,Ns);

fid = fopen([fpath, '\', fname,'.txt'],'w+');

for ik = 1:N
    fprintf(fid,'%e  %e\n', t(ik), sr(ik));
end

fclose(fid);

end

