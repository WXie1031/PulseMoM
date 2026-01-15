function [ output_args ] = FigureShownEMField( matrixE, matrixH,  t )
%FIGURESHOWNEMFIELD Summary of this function goes here
%   Detailed explanation goes here


%% Showing the modeling result
figure('position',[20,20 600 400]);

% Ez

plot(t, - matrixE.matrixETotal(:,3), 'r','LineWidth',2); hold on;
plot(t, - matrixE.matrixEStatic(:,3), 'g','LineWidth',2); hold on;
plot(t, - matrixE.matrixEInduction(:,3), 'b','LineWidth',2); hold on;
plot(t, - matrixE.matrixERadiation(:,3),'Color',[1 0.5 0],'LineWidth',2); hold on;

legend('E_z total','E_z static','E_z induction','E_z radiation');

ylabel('Electric field / V/m');
xlabel('time');

grid on;
box  on;

% Hx
figure('position',[20,20 600 400]);

plot(t, - matrixH.matrixHTotal(:,1), 'k'); hold on;
plot(t, - matrixH.matrixHInduction(:,1), ':r'); hold on;
plot(t, - matrixH.matrixHRadiation(:,1), 'r'); hold on;

legend('H_x total','H_x induction','H_x radiation');

ylabel('Magnetic field / A/m');
xlabel('time');

grid on;
box  on;

% Hy
figure('position',[20,20 600 400]);

plot(t, - matrixH.matrixHTotal(:,2), 'k'); hold on;
plot(t, - matrixH.matrixHInduction(:,2), ':r'); hold on;
plot(t, - matrixH.matrixHRadiation(:,2), 'r'); hold on;

legend('H_y total','H_y induction','H_y radiation');

ylabel('Magnetic field / A/m');
xlabel('time');

grid on;
box  on;

end

