function spice_MOV_wct(C0, L0, A, rs, fpath, mov_name)


fid = fopen([fpath, '\', mov_name,'.cir'],'w+');

fprintf(fid,'.SUBCKT %s  1  2  \n',mov_name);
fprintf(fid,'************************************** \n');
fprintf(fid,'*MOV Model Optimized by Chen Hongcai * \n');
fprintf(fid,'*   Hong Kong Polytechic University  * \n');
fprintf(fid,'*         All Rights Reserved        * \n');
fprintf(fid,'************************************** \n');
fprintf(fid,'* Model generated on %s \n',datestr(now,1));
fprintf(fid,'* MODEL FORMAT: PSpice \n');
fprintf(fid,'* node:  1      2  \n');

if L0==0
     fprintf(fid,'LS 1 3 10nH \n');
elseif L0<=0.01 && L0>0
    fprintf(fid,'LS 1 3 %.4fpH \n', L0*1e3);
elseif L0>0.01
    fprintf(fid,'LS 1 3 %.4fuH \n', L0);
end

fprintf(fid,'D1 3 4 DIODE \n');
fprintf(fid,'D2 4 3 DIODE \n');
fprintf(fid,'.model DIODE D bv=0.24 rs=%.4fu \n', rs*1e6);
fprintf(fid,'B1 4 2 v=v(3,4)*%.4f \n', A);

if C0>=1000
    fprintf(fid,'CP 3 2 %.4fuF \n', C0/1e3);
elseif C0<1000 && C0>0
    fprintf(fid,'CP 3 2 %.4fpF \n', C0);
end

fprintf(fid,'.ENDS \n');

fclose(fid);

