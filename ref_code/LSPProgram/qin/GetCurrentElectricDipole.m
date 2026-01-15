function structElectricDipole = GetCurrentElectricDipole(structElectricDipole, structModelingParameter, structChannelProperties, structCurrentParameter)
%UNTITLED Summary of this function goes here
%   Detailed explanation goes here
N_Segment = length(structElectricDipole);



sequenceSourceCurrentWaveForm = structCurrentParameter.sequenceSourceCurrentWaveForm;
N_Sequence = length(sequenceSourceCurrentWaveForm);

%CAC   = zeros(N_Sequence,1);

if N_Sequence==1
    
    structCurrentParameter.valueAttenuationFactor = 1e20;
    
end


for i = 1:1:N_Segment
    ShiftPointsCounts = round(structElectricDipole{i}.valueDelay/structModelingParameter.valueTimeScale);
    structElectricDipole{i}.sequenceCurrent = [zeros(ShiftPointsCounts,1);sequenceSourceCurrentWaveForm(1:length(sequenceSourceCurrentWaveForm)-ShiftPointsCounts)]...
        *exp(- structElectricDipole{i}.valueLengthToSource/structCurrentParameter.valueAttenuationFactor);
 
    structElectricDipole{i}.sequenceCurrentDerivative(1)              = (structElectricDipole{i}.sequenceCurrent(2) - structElectricDipole{i}.sequenceCurrent(1))/structModelingParameter.valueTimeScale;
    structElectricDipole{i}.sequenceCurrentDerivative(N_Sequence)     = (structElectricDipole{i}.sequenceCurrent(N_Sequence) - structElectricDipole{i}.sequenceCurrent(N_Sequence-1))/structModelingParameter.valueTimeScale;
    structElectricDipole{i}.sequenceCurrentDerivative(2:N_Sequence-1) = (structElectricDipole{i}.sequenceCurrent(3:N_Sequence)-structElectricDipole{i}.sequenceCurrent(1:N_Sequence-2))/2/structModelingParameter.valueTimeScale;
    structElectricDipole{i}.sequenceCurrentDerivative = structElectricDipole{i}.sequenceCurrentDerivative.';
    
  %  CAC =CAC+structElectricDipole{i}.sequenceCurrentDerivative;
    structElectricDipole{i}.sequenceCurrentCumulative = cumsum(structElectricDipole{i}.sequenceCurrent)*structModelingParameter.valueTimeScale;
end

% ttt-1;
end

