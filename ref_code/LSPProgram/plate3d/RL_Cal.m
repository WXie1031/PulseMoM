% RL_Cal        Calcaulte RC parameters for cell and sour, and return
%               according to preset codnctions
%
% Output (1) quit withour calcualtion (save other data)
%        (2) calcualte RL, then quit (save other data)
%            Case 1: RL at fixed freq, 
%            Case 2: RL at all freq. (fitting)
%            Case 3: R at all feq. for fitting
%        (3) calcualte RL, then cont (save other data)


function [Rx Ry Lx Ly Lsx Lsy INDEXout]=RL_Cal(Freq,Plat,Sour,INDEXin)
INDEXout=INDExin;

str='./RL_DATA';                    % fold storing RL data
fname=[str '/Rx_data.txt'];           
fexist=exist(fname,'file');

button1='Yes, calculate it';
if fexist==2                        % Exist data file, ask user to decide                              
    button1=questdlg('Would you like to recalculate it?',...
        'Found existing data file','Yes, calculate it',...
        'No, use the file','No, quit the program','No, use the file');
end

switch button1
    case 'No, use the file'             % (1) directly load data file 
         cd(str);
         Rx=dlmread('Rx_data.txt');
         Ry=dlmread('Ry_data.txt');
         Lx=dlmread('Lx_data.txt');
         Ly=dlmread('Ly_data.txt');
         Lsx=dlmread('Lsx_data.txt');
         Lsy=dlmread('Lsy_data.txt');  
         cd('../');
    case 'Yes, calculate it'           % (2)  load data file 
         [Rx Ry Lx Ly Lsx Lsy]=COEF_3D_PLAT(Freq,Plat,Sour);
         cd(str);
         dlmwrite('Rx_data.txt', Rx);
         dlmwrite('Ry_data.txt', Ry);
         dlmwrite('Lx_data.txt', Lx);
         dlmwrite('Ly_data.txt', Ly);
         dlmwrite('Lsx_data.txt', Lsx);
         dlmwrite('Lsy_data.txt', Lsy);
         save ('input_data.mat', 'Freq', 'Isx', 'Isy', 'BRNO', 'BFDS', 'BFDC'); 
         cd('../'); 
         
         if button0==0               % calculate RL only for vector fitting
             INDEXout=INDEXout+1;
         end
         button2=questdlg('Would you like to continue?','Completion of Calculation','Yes','No','Yes');
         switch button2
             case 'No'
                INDEXout=INDEXin+1;
             case 'Yes'
         end
    case 'No, quit the program'
         cd(str);
         save ('input_data.mat', 'Freq', 'Isx', 'Isy', 'BRNO', 'BFDS', 'BFDC');         
         cd('../');       
        return;
    otherwise
        disp('no such option in Freq_Solv');
end
