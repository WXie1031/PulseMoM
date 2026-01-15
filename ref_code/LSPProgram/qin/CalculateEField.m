function [ matrixE, matrixH ] = CalculateEField( structChannelProperties,...
    structModelingParameter,...
    structCurrentParameter,...
    structAntennaParameter)

structElectricDipole = GetPositionElectricDipole(structChannelProperties);
structElectricDipole = GetCurrentElectricDipole(structElectricDipole, structModelingParameter, structChannelProperties, structCurrentParameter);

Epsilon0      = 8.854187817e-12; %F/m
Mu0           = 4*pi*1e-7;
c             = 1/((Epsilon0*Mu0)^(1/2));

t = 0:structModelingParameter.valueTimeScale:structModelingParameter.valueWaveFormLength;
N_Segment = length(structElectricDipole);
N_Sequence = length(t);

matrixE.matrixETotal     = zeros(N_Sequence,3);
matrixE.matrixEStatic    = zeros(N_Sequence,3);
matrixE.matrixEInduction = zeros(N_Sequence,3);
matrixE.matrixERadiation = zeros(N_Sequence,3);

matrixH.matrixHTotal     = zeros(N_Sequence,3);
matrixH.matrixHInduction = zeros(N_Sequence,3);
matrixH.matrixHRadiation = zeros(N_Sequence,3);

%% v/c parameter for traveling current pulses
valueVOverC = structChannelProperties.valueVelocity/c;

%%
for i = 1:1:N_Segment
    
    structElectricDipole{i}.vectorDirectionR = structAntennaParameter.vectorPosition  - structElectricDipole{i}.vectorPosition;
    structElectricDipole{i}.vectorDirectionR = structElectricDipole{i}.vectorDirectionR/norm(structElectricDipole{i}.vectorDirectionR);
    
    structElectricDipole{i}.vectorDirectionFai =  cross(structElectricDipole{i}.vectorDirectionR, structChannelProperties.vectroCurrentDirection);
    structElectricDipole{i}.vectorDirectionFai = structElectricDipole{i}.vectorDirectionFai/norm(structElectricDipole{i}.vectorDirectionFai);
    
    structElectricDipole{i}.vectorDirectionTheta =  cross(structElectricDipole{i}.vectorDirectionR, structElectricDipole{i}.vectorDirectionFai);
    structElectricDipole{i}.vectorDirectionTheta = structElectricDipole{i}.vectorDirectionTheta/norm(structElectricDipole{i}.vectorDirectionTheta);
        
    structElectricDipole{i}.valueSinTheta = norm(cross(structChannelProperties.vectroCurrentDirection,structElectricDipole{i}.vectorDirectionR))/...
        norm(structElectricDipole{i}.vectorDirectionR)/norm(structChannelProperties.vectroCurrentDirection);
    
    structElectricDipole{i}.valueCosTheta = norm(dot(structChannelProperties.vectroCurrentDirection,structElectricDipole{i}.vectorDirectionR))/...
        norm(structElectricDipole{i}.vectorDirectionR)/norm(structChannelProperties.vectroCurrentDirection);
    
    structElectricDipole{i}.valueR = norm( structAntennaParameter.vectorPosition  - structElectricDipole{i}.vectorPosition);
    
    % E field
    structElectricDipole{i}.matrixEStatic = structChannelProperties.valueScaleLength/(4*pi*Epsilon0)*...
        (repmat(2*structElectricDipole{i}.sequenceCurrentCumulative*structElectricDipole{i}.valueCosTheta/(structElectricDipole{i}.valueR^3),1,3).*repmat(structElectricDipole{i}.vectorDirectionR,N_Sequence,1)+...
        repmat(1*structElectricDipole{i}.sequenceCurrentCumulative*structElectricDipole{i}.valueSinTheta/(structElectricDipole{i}.valueR^3),1,3).*repmat(structElectricDipole{i}.vectorDirectionTheta,N_Sequence,1));
    
    structElectricDipole{i}.matrixEInduction = structChannelProperties.valueScaleLength/(4*pi*Epsilon0)*...
        (repmat(2*structElectricDipole{i}.sequenceCurrent*structElectricDipole{i}.valueCosTheta/(c*structElectricDipole{i}.valueR^2),1,3).*repmat(structElectricDipole{i}.vectorDirectionR,N_Sequence,1)+...
        repmat(1*structElectricDipole{i}.sequenceCurrent*structElectricDipole{i}.valueSinTheta/(c*structElectricDipole{i}.valueR^2),1,3).*repmat(structElectricDipole{i}.vectorDirectionTheta,N_Sequence,1));
    
    structElectricDipole{i}.matrixERadiation = structChannelProperties.valueScaleLength/(4*pi*Epsilon0)*.../(1-valueVOverC*structElectricDipole{i}.valueCosTheta)*...
        (repmat(1*structElectricDipole{i}.sequenceCurrentDerivative*structElectricDipole{i}.valueSinTheta/(c*c*structElectricDipole{i}.valueR),1,3).*repmat(structElectricDipole{i}.vectorDirectionTheta,N_Sequence,1));
    
    % H field
    structElectricDipole{i}.matrixHInduction = structChannelProperties.valueScaleLength/(4*pi)*...
        (repmat(1*structElectricDipole{i}.sequenceCurrent*structElectricDipole{i}.valueSinTheta/(structElectricDipole{i}.valueR^2),1,3).*repmat(structElectricDipole{i}.vectorDirectionFai,N_Sequence,1));
    
    structElectricDipole{i}.matrixHRadiation = structChannelProperties.valueScaleLength/(4*pi)*...
        (repmat(1*structElectricDipole{i}.sequenceCurrentDerivative*structElectricDipole{i}.valueSinTheta/(c*structElectricDipole{i}.valueR),1,3).*repmat(structElectricDipole{i}.vectorDirectionFai,N_Sequence,1));
    
    % the retard
    ShiftPointsCounts = round(structElectricDipole{i}.valueR/c/structModelingParameter.valueTimeScale);
    
    % field space integration
    structElectricDipole{i}.matrixEStatic = [zeros(ShiftPointsCounts,3);structElectricDipole{i}.matrixEStatic(1:N_Sequence-ShiftPointsCounts,:)];
    structElectricDipole{i}.matrixEInduction = [zeros(ShiftPointsCounts,3);structElectricDipole{i}.matrixEInduction(1:N_Sequence-ShiftPointsCounts,:)];
    structElectricDipole{i}.matrixERadiation = [zeros(ShiftPointsCounts,3);structElectricDipole{i}.matrixERadiation(1:N_Sequence-ShiftPointsCounts,:)];
    structElectricDipole{i}.matrixHInduction = [zeros(ShiftPointsCounts,3);structElectricDipole{i}.matrixHInduction(1:N_Sequence-ShiftPointsCounts,:)];
    structElectricDipole{i}.matrixHRadiation = [zeros(ShiftPointsCounts,3);structElectricDipole{i}.matrixHRadiation(1:N_Sequence-ShiftPointsCounts,:)];
        
    structElectricDipole{i}.matrixETotal = structElectricDipole{i}.matrixEStatic+...
        structElectricDipole{i}.matrixEInduction+...
        structElectricDipole{i}.matrixERadiation;
    
    structElectricDipole{i}.matrixHTotal = structElectricDipole{i}.matrixHInduction+...
        structElectricDipole{i}.matrixHRadiation;
    
    matrixE.matrixETotal = matrixE.matrixETotal+structElectricDipole{i}.matrixETotal;
    matrixH.matrixHTotal = matrixH.matrixHTotal+structElectricDipole{i}.matrixHTotal;
       
    matrixE.matrixEStatic    = matrixE.matrixEStatic    + structElectricDipole{i}.matrixEStatic;
    matrixE.matrixEInduction = matrixE.matrixEInduction + structElectricDipole{i}.matrixEInduction;
    matrixE.matrixERadiation = matrixE.matrixERadiation + structElectricDipole{i}.matrixERadiation;
    
    matrixH.matrixHInduction = matrixH.matrixHInduction + structElectricDipole{i}.matrixHInduction;
    matrixH.matrixHRadiation = matrixH.matrixHRadiation + structElectricDipole{i}.matrixHRadiation;   

end

end

