function exitflag = create_surge_generator(Uc, save_path_name)
% input Uc in kV
% x = inputdlg('Input Charging Voltage of Surge Generator (kV):',...
%     'Input Charging Voltage', [1 40]);
% Uc = str2double(x{:});

% opengl('save','software');

if Uc>0
    
    %Uc = 1.7e3;
    Uc = Uc*1e3;
    Ls = 5.6e-6;
    Cs = 10e-6;
    Re = 15;
    Rs1 = 2;
    Rs2 = 11;
    
    [pathstr, fname] = fileparts(save_path_name) ;
    
    %[FileName,PathName] = uiputfile({'*.cir', 'Circuit Files (*.cir)'});
    
    spice_surge_generator(Uc, Cs, Ls, Re, Rs1, Rs2, pathstr, fname);
    
    exitflag = 1; %succese
else
    exitflag = 0;
end

end


