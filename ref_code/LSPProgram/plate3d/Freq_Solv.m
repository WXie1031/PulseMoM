% Transient Shielding Solver 
%   (a) Freq-domain solver with
%       (1) PEEC method (modelling plate cells with R / L obtained with
%           the method of R=We/(IxI*), and L=Wm/(IxI*)
%       (2) circuit analysis approach to find current distribution and
%           field around t he plate
%   (b) Time-domain solver with
%       (1) PEEC method (modelling plate cells with R / L obtained with
%           the method of R=We/(IxI*), and L=Wm/(IxI*)
%       (2) convlution method (obtaining impulse response of freq-dep
%           R and freq-indep L)
%       (3) circuit analysis software (pspice) to find cell current, then
%           mangetic field around the plate by using Kc*Ic+Ks*Is
%
% input:    f = 0   (R and L at all frequencies), 
%             =-1   (R at all frequencies)
%             >0    (R  and L at a fixed f)
% output:   Rx and Rx: resistance of all cells
%           Lx and Ly: inductance of all cells
%           Lsx & Lsy: inductance of all cells from Is to cell Ic
%           Lmx & Lmy: inductance of all cells from cell Ic to field-out
%
% HEADER    contains all basic info. of plates, sources, field lines
% NB_XY     returns bran-node incidence Ax and Ay
% COEF_3D_PLAT  returns Rx Ry Lx Ly Lsx Lsy 
% | Zx    0  Ax  0 || Ix |   | -Vsx | --> Ix=-Zx'*Ax*U-Zx'*Zsx*Isx
% | 0    Zy  0  Ay || Iy | = | -Vsy | --> Iy=-Zy'*Ay*U-Zy'*Zsy*Isy
% | Axt Ayt  0   0 || Uo |   | Isc  | --> 
% Uo=-Zeq(Axt*Zx'*Zsx*Isx+Ayt*Zy'*Zsy*Isy)
% Zeq=Ax'*Zx'*Ax+Ay'*Zy'*Ay
% Bx=        +Ksxy*Isy         +Kcxy*Icy
% By=Ksyx*Isx         +Kcyx*Icx
% Bz=Kszx*Isx+Kszy*Isy+Kczx*Icx+Kczy*Icy
% 
% f=1e3; [Freq Rx Ry Lx Ly Lsx Lsy Icx Icy Isx Isy BRNO BFDS BFDC]=Freq_Solv(f);
% March 2016

function [Freq Rx Ry Lx Ly Lsx Lsy Icx Icy Isx Isy BRNO BFDS BFDC]=Freq_Solv(f)
t1=clock

Rx=0; Ry=0; Lx=0; Ly=0; Lsx=0; Lsy=0; Icx=0; Icy=0; BFDS=0; BFDC=0;

% (1) getting basic information
[Freq Plat Sour Fied]=HEADER_SQ1BY1(f);

% (2) Pre-process
% order: x->y for Ix,Iy, Vn
Plat=MESH_3D_PLAT(Plat);   % meshing
[Ax,Ay,Plat]=NB_XY(Plat);                 % Ix/Iy-V matrix    
%--------------------------------------------------------------------------
% output bran-node relationship for x-cell y-cell SX-cell and SY-cell
BRNO.SBNX=Sour.BNX;
BRNO.SBNY=Sour.BNY;
BRNO.PBNX=Plat.BNX;
BRNO.PBNY=Plat.BNY;
Isx=Sour.xcur;
Isy=Sour.ycur;
Isy=Sour.ycur;
%--------------------------------------------------------------------------

% (3) Find out source/cell field coef
[BFDS Isx Isy]=H_CAL_SOUR(Fied,Sour);           % source field
BFDC=H_CAL_PLAT(Fied,Plat);                     % palte field

% (4) Find out impedance matrix
str='./RL10_DATA';
fname=[str '/Rx_data.txt'];                % fname=[MDIR,'\',ModelFile,'Gain']; 
fexist=exist(fname,'file');
button1='Yes, calculate it';
if fexist==2                        % Exist data file, ask user to decide                              
    button1=questdlg('Would you like to recalculate it?',...
        'Found Existing RL Files','Yes, calculate it',...
        'No, use the file','No, quit the program','No, use the file');
end
switch button1
    case 'No, use the file'
         % (1) directly load gain file witnin WaveFrGainUI
         cd(str);
         Rx=dlmread('Rx_data.txt');
         Ry=dlmread('Ry_data.txt');
         Lx=dlmread('Lx_data.txt');
         Ly=dlmread('Ly_data.txt');
         Lsx=dlmread('Lsx_data.txt');
         Lsy=dlmread('Lsy_data.txt');  
         cd('../');
    case 'Yes, calculate it'
         [Rx Ry Lx Ly Lsx Lsy]=COEF_3D_PLAT(Freq,Plat,Sour);
         cd(str);
         if f<=0               % calculate RL only for vector fitting
             fitRx=Rx;
             fitRy=Ry;
             fitLx=Lx;
             fitLy=Ly;
             save ('fitting_data.mat', 'Freq', 'fitRx', 'fitRy', 'fitLx', 'fitLy'); 
             cd('../'); 
             return;
         end
         
         dlmwrite('Rx_data.txt', Rx);
         dlmwrite('Ry_data.txt', Ry);
         dlmwrite('Lx_data.txt', Lx);
         dlmwrite('Ly_data.txt', Ly);
         dlmwrite('Lsx_data.txt', Lsx);
         dlmwrite('Lsy_data.txt', Lsy);       
         save ('input_data.mat', 'Freq', 'Isx', 'Isy', 'BRNO', 'BFDS', 'BFDC'); 
         cd('../'); 
         
         button2=questdlg('Would you like to continue calculating I(A) and B (mG)?','Completion of Parameter Calculation','Yes','No','Yes');
         switch button2
             case 'No'
                return;
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
t0=clock      
% (4) solving for current at a fixed frequency f
omega=2*pi*f;
Zx=Rx+1i*omega*Lx;    Yx=inv(Zx);   clear Zx
Zy=Ry+1i*omega*Ly;    Yy=inv(Zy);   clear Zy
Zsx=1i*omega*Lsx;
Zsy=1i*omega*Lsy;

tmpx=Ax*Yx;                             
tmpy=Ay*Yy;   
I0=-(tmpx*Zsx*Sour.xcur+tmpy*Zsy*Sour.ycur);
Y0=(tmpx*Ax'+tmpy*Ay');
U0=Y0\I0;               %inv(Y0)*I0;
clear tmpx tmpy I0 Y0;

Icx=-(Yx*Ax'*U0+Yx*Zsx*Sour.xcur);   
Icy=-(Yy*Ay'*U0+Yy*Zsy*Sour.ycur);   
clear Ax Ay U0;

Jcx=Icx./Plat.XCEL(:,7);   % line density
Jcy=Icy./Plat.YCEL(:,7);   % line density

% (5) Calcualte field
% [BFDS Isx Isy]=H_CAL_SOUR(Fied,Sour);           % source field
Bxs=BFDS.Bxy*Isy;                  BFDS.Bxs=Bxs;
Bys=BFDS.Byx*Isx;                  BFDS.Bys=Bys;
Bzs=(BFDS.Bzx*Isx+BFDS.Bzy*Isy);   BFDS.Bzs=Bzs;

% BFDC=H_CAL_PLAT(Fied,Plat);                     % palte field
BFDC.Bxc=BFDC.Bxy*Icy;                  
BFDC.Byc=BFDC.Byx*Icx;
BFDC.Bzc=(BFDC.Bzx*Icx+BFDC.Bzy*Icy);

Bx=BFDC.Bxc+BFDS.Bxs;       BFDS.Bxt=Bx;
By=BFDC.Byc+BFDS.Bys;       BFDS.Byt=By;
Bz=BFDC.Bzc+BFDS.Bzs;       BFDS.Bzt=Bz;

B0=sqrt(Bx.*conj(Bx)+By.*conj(By)+Bz.*conj(Bz));
Bs=sqrt(Bxs.*conj(Bxs)+Bys.*conj(Bys)+Bzs.*conj(Bzs));
SE=B0./Bs;

% (6) output complete solution data
cd(str);
save ('output_data.mat', 'Freq', 'Icx', 'Icy', 'Isx', 'Isy', 'BRNO', 'BFDS', 'BFDC'); 
cd('../');       

% (7) plotting (b)
% (7a) plotting cuerrent density
% figure(1);
% Jx=Ix./Plat.XCEL(:,7);   % line density
% Jy=Iy./Plat.YCEL(:,7);   % line density
% x0=Plat.XCEL(:,6);
% y0=Plat.YCEL(:,7);
% s1='real of J';    s2='imag of J';
% subplot(2,1,1);plot(x0,real(Jx),'r-o',x0,imag(Jx),'b-x');ylabel('Jx (A/m)');xlabel('dis (m)');legend(s1,s2);
% subplot(2,1,2);plot(y0,real(Jy),'r-o',y0,imag(Jy),'b-x');ylabel('Jy (A/m)');xlabel('dis (m)');legend(s1,s2);

% (7b) plotting magnetic field
dir=Fied.pts(7);
ax=Fied.line(:,dir);
figure(2);
subplot(3,1,1);plot(ax,abs(Bx));ylabel('Bx (mG)');xlabel('dis (m)');grid;
subplot(3,1,2);plot(ax,abs(By));ylabel('By (mG)');xlabel('dis (m)');grid;
subplot(3,1,3);plot(ax,abs(Bz));ylabel('Bz (mG)');xlabel('dis (m)');grid;

figure(3);
s1='field with shielding'; s2='field without shielding';
subplot(2,1,1);plot(ax,B0,'r',ax,Bs,'b');ylabel('B (mG)');xlabel('dis (m)');
grid;title('Magnetic Flux Density)');legend(s1,s2);
subplot(2,1,2);plot(ax,SE,'r');xlabel('dis (m)');ylabel('SE');
grid;title('Shielding Effectiveness');

t2=clock-t1
end
