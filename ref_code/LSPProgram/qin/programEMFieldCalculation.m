%% The program of full-wave caluculation of lightning channel
%
% Here, every field is a vector in Cartesian coordinate system, i.e.:
% [ix,iy,iz] in a 3 column row vector; the column vector is refer the time
% sequence.ie[Ex(t1),Ey(t1),Ez(t1);
%             Ex(t2),Ey(t2),Ez(t2);
%             Ex(t3),Ey(t3),Ez(t3);
%             ...]
%
%% Created by QIN Zilong 2017.8.4@DukeU, version, 1.0.0
%% Modified by QIN Zilong 2017.8.22@DukeU, version, 1.0.1

clear all;
close all;

%% channel characteristics
structChannelProperties.vectorPointChannelCenter        = [50e3 ,0.0, 0.0];
structChannelProperties.vectroCurrentDirection          = [0.0, 0.0, -1.0]; %% normalize it
structChannelProperties.vectorCurrentFlowingDirection   = [0.0, 0.0, 1.0];
structChannelProperties.valueWholeLength                = 2000; % in m
structChannelProperties.valueScaleLength                = 20;
structChannelProperties.valueVelocity                   = 1.3e8;

%% Modeling parameter
structModelingParameter.valueTimeScale                  = 1e-8;
structModelingParameter.valueWaveFormLength             = 1e-3;

t = (0:structModelingParameter.valueTimeScale:structModelingParameter.valueWaveFormLength).';

%% Current parameter
structCurrentParameter.valueTau1                        = 0.3e-6;
structCurrentParameter.valueTau2                        = 2.3e-6;
structCurrentParameter.valueOrderN                      = 2;
structCurrentParameter.valuePeakCurrent                 = 22.7e3; %Ampere
structCurrentParameter.valueAttenuationFactor           = 2000;  %meter

% 
%% Antenna parameter, specify the position of your anttenna
structAntennaParameter.vectorPosition                         = [3300,0,0];

%% Generate the current waveform
structCurrentParameter.sequenceSourceCurrentWaveForm = GenerateWaveformCurrentShao(t,...
    structCurrentParameter.valueTau1 ,...
    structCurrentParameter.valueTau2,...
    structCurrentParameter.valueOrderN,...
    structCurrentParameter.valuePeakCurrent );

%% Invoke the calculation program

% Directed wave
[ matrixE_directed, matrixH_directed ] = CalculateEField( structChannelProperties,...
    structModelingParameter,...
    structCurrentParameter,...
    structAntennaParameter);

% Reflected wave from the ground/ the mirror PEC assumption
structAntennaParameter.vectorPosition(1,3) = -structAntennaParameter.vectorPosition(1,3);
[ matrixE_reflected, matrixH_reflected ] = CalculateEField( structChannelProperties,...
    structModelingParameter,...
    structCurrentParameter,...
    structAntennaParameter);

% superimposing directed and reflected wave
matrixE.matrixETotal     = matrixE_directed.matrixETotal + matrixE_reflected.matrixETotal;
matrixH.matrixHTotal     = matrixH_directed.matrixHTotal + matrixH_reflected.matrixHTotal;

matrixE.matrixEStatic    = matrixE_directed.matrixEStatic    + matrixE_reflected.matrixEStatic;
matrixE.matrixEInduction = matrixE_directed.matrixEInduction + matrixE_reflected.matrixEInduction;
matrixE.matrixERadiation = matrixE_directed.matrixERadiation + matrixE_reflected.matrixERadiation;

matrixH.matrixHInduction = matrixH_directed.matrixHInduction + matrixH_reflected.matrixHInduction;
matrixH.matrixHRadiation = matrixH_directed.matrixHRadiation + matrixH_reflected.matrixHRadiation;


%% Show the result
FigureShownEMField( matrixE, matrixH, t*1e6 );


