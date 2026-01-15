function spice_rion(Rgrod, Rsoil, fpath_grid, fname_grid)


RGname = [fname_grid,'_Rion'];

fid = fopen([fpath_grid, RGname,'.cir'],'w+');

E0 = 350e3;  % E = 350kV/m
Ig = 1/(2*pi)*E0*Rsoil/(Rgrod^2);

%% Write the nonlinear ground resistance
% Adding nonlinear ground resistance modeling soil ionization effect

fprintf(fid,'.SUBCKT  %s  1  PARAMS: Ig=%.2f  R0=%.4f \n', RGname, Ig, Rgrod);
fprintf(fid,'* Nonlinear Ground Resistance Used in Ground Grid\n');

fprintf(fid,'R_R_ROD    1  2  1u  TC=0,0 \n');
fprintf(fid,'V_I_SENSE  2  3  0V \n');
fprintf(fid,'H_I_SENSE  4  0  V_I_SENSE   1 \n');
fprintf(fid,'R_I_SENSE  4  0  1G   TC=0,0 \n');
fprintf(fid,'E_VAR  3  0 \n');
fprintf(fid,'+  VALUE={V(4)*{R0}/SQRT(1+ABS(V(4))/{Ig})} \n');
fprintf(fid,'.ENDS %s \n', RGname);


%% close the file after writing the spice file
fclose(fid);

