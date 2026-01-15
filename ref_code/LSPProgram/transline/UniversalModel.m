%
% Single phase Universal Model performed with the trapezoidal rule
%
%       Reseach group
%
%          Dr. J. L. Naredo
%          Dr. J. A. Gutiérrez-Robles
%
%                                  Copyright july 2010

clear all
clc

% m file to set the line data
LineData

% Parameters in per unit length
[Zg,Zt,Zc,Yg,ZL,YL] = LineParameters(Mu,Eo,Rsu,Geom,Ncon,Ns,w);

% Modal Parameters
for k=1:Ns
   [Yc(:,:,k),Vm(k,:),Hm(k,:)] = ABYZLM(ZL(:,:,k),YL(:,:,k),lenght,w(k));
end

% Admittance fit
[YcPoles,YcResidues,YcConstant,YcProportional] = YcFit(Yc,f,Ns,Ncon);

% Hk fit
[HkPoles,HkResidues,HkConstant,HkProportional,md] = HkFitTrace(Hm,Vm,ZL,YL,f,Ns,lenght,Ncon);

% m file to execute the loop of the simulation
SimulationLoop


