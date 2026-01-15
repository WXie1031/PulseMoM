clear
clc
% m file to set line data
LineData
% Per unit length parameters
[Zg,Zt,Zc,Yg,ZL,YL] = LineParameters(Mu,Eo,Rsu,Geom,Ncon,Ns,w);
% Modal Parameters
for k=1:Ns
[Yc(:,:,k),Vm(k,:),Hm(k,:)] = ABYZLM(ZL(:,:,k),YL(:,:,k),lenght,w(k));
end
% Characteristic Admittance Fitting
[YcPoles,YcResidues,YcConstant,YcProportional] = YcFit(Yc,f,Ns,Ncon);
% Hk fit
[HkPoles,HkResidues,HkConstant,HkProportional,md] = HkFit(Hm,Vm,ZL,YL,f,Ns,lenght,Ncon);
% m file to execute simulation loop.
SimulationLoop


