function [structElectricDipole] = GetPositionElectricDipole(structChannelProperties)

structChannelProperties.vectroCurrentDirection = structChannelProperties.vectroCurrentDirection/...
    norm(structChannelProperties.vectroCurrentDirection);
structChannelProperties.vectorCurrentFlowingDirection = structChannelProperties.vectorCurrentFlowingDirection/...
    norm(structChannelProperties.vectorCurrentFlowingDirection);


N_Segments = structChannelProperties.valueWholeLength/structChannelProperties.valueScaleLength;

for i=1:1:N_Segments
    valueLengthToCenter = (i-0.5 - (N_Segments)/2)*structChannelProperties.valueScaleLength;
    structElectricDipole{i}.vectorPosition = structChannelProperties.vectorPointChannelCenter +...
        valueLengthToCenter*structChannelProperties.vectorCurrentFlowingDirection;
    structElectricDipole{i}.valueLengthToSource = (i-0.5)*structChannelProperties.valueScaleLength;  
    structElectricDipole{i}.valueDelay = (i-0.5)*structChannelProperties.valueScaleLength/...
        structChannelProperties.valueVelocity;
end


%UNTITLED Summary of this function goes here
%   Detailed explanation goes here
end

