function [Ls, Lin, Lext] = induct_agi_ac(wid, hig, sig, mur, len, f0)
%  Function:       induct_agi_ac
%  Description:    Calculate AC inductance of the L-shaped conductor using
%                  fitting method.
%
%  Calls:          
%
%  Input:          wid   --  width of conductors (N*1) (m)
%                  hig   --  thick of conductors (N*1) (m)
%                  sig   --  conductivity of conductors (N*1) (S/m)
%                  mur   --  Relative Permeability of the conductor
%                  len   --  length of conductors (N*1)
%                  f0    --  frequency
%  Output:         Ls    --  Ls vector (have multiplied length)
%  Others:
%  Author:         Chen Hongcai
%  Email :         hc.chen@live.com
%  Date:           2016-03-14

mu0 = 4*pi*1e-7;

modi_cof = 2.5;  % modification to internal inductance.

Dagi = gmd_agi_self(wid,hig);
Lr = induct_gmd(Dagi, len);

hw_ratio = max(hig./wid, wid./hig);

%% fit expression for the dc internal inductance of rectangle
% from - Addition to "DC Internal Inductance for a Conductor of Rectangular Cross Section"
% modification to fit L-shaped conductor
Lin_dc = 0.28*sqrt(hw_ratio) .* induct_bar_in_dc_appr(wid, hig, len);


%% fit expression for the AC inductance of rectangle
% from - Modeling of impedance of rectangular cross-section conductors
cof = 5.0*sqrt(mur);
w0 = 8./(mu0*sqrt(mur)*sig).*((wid+hig)./(wid.*hig)).^2;

Lext = Lr - modi_cof*Lin_dc;
%Lext = Lr - 50e-9*len;

% internal inductance has relation with mur
Lin = sqrt(mur).* Lin_dc./sqrt(1+cof*2*pi*f0./w0); %

Ls = Lext + Lin;

   

end
    
