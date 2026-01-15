%
% Line Geometry
%
% 1 column -- number of conductor
% 2 column -- x position of each conduntor in meters
% 3 column -- y position of each coductor in meters
% 4 column -- radii of each conductor
% 5 column -- number of conductor in haz
% 6 column -- distance between haz conductors in meters
% 7 column -- resistivity of the aluminum
% 8 column -- relative permitivity
% 9 column -- line lenght in m

Geom = [1    0   20   0.03058/2   3   0.40   2.826e-8   1000   150e3
        2   10   20   0.03058/2   3   0.40   2.826e-8   1000   150e3
        3   20   20   0.03058/2   3   0.40   2.826e-8   1000   150e3 ];
             
lenght  = Geom(1,9);                 % Line lenght
Ncon    = Geom(max(Geom(:,1)),1);    % Number of conductors
Rsu     = 100;                       % Earth resistivity Ohm-m
Mu      = 4*pi*1E-7;                 % Henry's/meters
Eo      = (1/(36*pi))*1E-9;          % Farads/meters
Rhi     = 9.09E-7;                   % Ohm-m   resistivity of the iron.
Ral     = 2.61E-8;                   % Ohm-m   resistivity of the aluminum.
Rhg     = 2.71E-7;                   % Ohm-m   resistivity of the sky wires.
Ns      = 500;                       % Number of samples
f       = logspace(-2, 6, Ns);       % Frequence
w       = 2*pi*f;                    % Vector of frequencies in radian/sec.




