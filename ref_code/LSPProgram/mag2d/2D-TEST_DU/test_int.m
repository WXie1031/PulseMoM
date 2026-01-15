Sxy=[0 0 0.01 0.002];
Oxy=[0.5*(Sxy(1)+Sxy(3)) 0.5*(Sxy(2)+Sxy(4))]+0.005*0;
r0=1e-6;

[Pa,Pb,Pc,Pdx,Pdy,Pe]=COEF_2D_CELL_M0(Sxy,Oxy,r0);
[Pa,Pb,Pc,Pdx,Pdy,Pe]

xf=Oxy(1);  yf=Oxy(2);
% fun = @(xs,ys) 0.5*log((xf-xs).*(xf-xs)+(yf-ys).*(yf-ys));
% fun = @(xs,ys) (xf-xs)./((xf-xs).*(xf-xs)+(yf-ys).*(yf-ys));
% fun = @(xs,ys) 2*(xf-xs).*(yf-ys)./((xf-xs).*(xf-xs)+(yf-ys).*(yf-ys)).^2;
fun = @(xs,ys) ((xf-xs).^2-(yf-ys).^2)./((xf-xs).*(xf-xs)+(yf-ys).*(yf-ys)).^2;

q = integral2(fun,Sxy(1),Sxy(3),Sxy(2),Sxy(4))
disp('end');
